#include "App.h"
#include "VoltageMonitor.h"
#include "TemperatureSensor.h"
#include "Radio.h"
#include "Security.h"
#include "PacketEncoder.h"
#include "MeasurementData.h"
#include "PacketId.h"

#include "stm32l0xx_hal.h"
#include "main.h"

#include <string.h>

// it is number of 30s blocks between retransmission.
// 10 means that first retransmission is at 10th iteration, then 20th iteration, (at 30th iteration new data are send)
#define RETRANSMISSION_TIME_DELTA_BETWEEN 10
#define RETRANSMISSION_COUNT 3

extern RTC_HandleTypeDef hrtc;

static PacketId packetId;

void SystemClock_Config(void);

void App_SleepLong() {
	HAL_PWREx_EnableUltraLowPower();
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// wake up
	SystemClock_Config();
}

void App_SleepShortTicks(int ticks) {
	HAL_ResumeTick();
	HAL_Delay(ticks);
	HAL_SuspendTick();
}

void App_SleepShortMs(int ms) {
	HAL_ResumeTick();
	HAL_Delay(ms);
	HAL_SuspendTick();
}

static void App_Transmit(MeasurementData* data, int retransmissionNumber) {
	int transmitStatus;

	uint8_t message[20];
	PacketEncoder_EncodeNewDataPacket(message, sizeof(message), data, &packetId, retransmissionNumber);

	HAL_GPIO_WritePin(DBG1_GPIO_Port, DBG1_Pin, 1);
	// HAL_GPIO_WritePin(HRVST_DIS_SW_GPIO_Port, HRVST_DIS_SW_Pin, 1);

	transmitStatus = Radio_TransmitMessage(message, sizeof(message));
	if (transmitStatus) {
		__BKPT(0);
	}

	// HAL_GPIO_WritePin(HRVST_DIS_SW_GPIO_Port, HRVST_DIS_SW_Pin, 0);
	HAL_GPIO_WritePin(DBG1_GPIO_Port, DBG1_Pin, 0);
}

void App_Run() {
	int securityStatus;
	int temperatureStatus;
	int voltageStatus;

	MeasurementData sendData;
	// default value is set above (+1) RETRANSMISSION_COUNT prevent "retransmitting" uniinitialized data at power up
	int retransmissionNum = RETRANSMISSION_COUNT + 1;

	MeasurementData workingData;

	// HAL_SuspendTick()
	// HAL_SuspendTick() is called inside App_SleepShortMs(2);

	// HAL_GPIO_WritePin(DBG_RADIO_CSB_GPIO_Port, DBG_RADIO_CSB_Pin, 0);

	// wait for chips powerup
	App_SleepShortMs(2);

	securityStatus = Security_Init();
	if (securityStatus) {
		while (1) {
			App_SleepShortMs(10);
			NVIC_SystemReset();
		}
	}

	Radio_Init();

	__HAL_RCC_GPIOH_CLK_DISABLE();

	PacketId_Init(&packetId);

	while (1) {
		MeasurementData_Reset(&workingData);

		for (int i = 0; i < 30; i++) {
			temperatureStatus = TemperatureSensor_BeginMeasurement();

			VoltageMonitor_Voltages voltages;
			voltageStatus = VoltageMonitor_GetVoltages(&voltages);

			int16_t temperature;
			if (temperatureStatus == 0) {
				temperatureStatus = TemperatureSensor_GetTemperature(&temperature);
			}

			if (voltageStatus == 0) {
				if (i >= 10 && i <= 15 && !workingData.isVoltage10Loaded) {
					workingData.voltage10 = voltages.batteryVoltage;
					workingData.isVoltage10Loaded = 1;
				} else if (i >= 20 && i <= 25 && !workingData.isVoltage20Loaded) {
					workingData.voltage20 = voltages.batteryVoltage;
					workingData.isVoltage20Loaded = 1;
				} else if (i > 25) {
					workingData.voltage30 = voltages.batteryVoltage;
					workingData.isVoltage30Loaded = 1;
				}
			}

			if (temperatureStatus == 0) {
				workingData.temperatureSum += (int32_t)temperature;
				workingData.temperaturesCount++;
				if (temperature < workingData.temperatureMin) {
					workingData.temperatureMin = temperature;
				}
				if (temperature > workingData.temperatureMax) {
					workingData.temperatureMax = temperature;
				}
			}

			int is_voltage_good = voltageStatus == 0 && voltages.batteryVoltage > 3000;
			int is_rf_budget = retransmissionNum < RETRANSMISSION_COUNT;
			int is_time_to_retransmit = (i != 0) && (i % RETRANSMISSION_TIME_DELTA_BETWEEN == RETRANSMISSION_TIME_DELTA_BETWEEN - 1);
			if (is_time_to_retransmit && is_rf_budget && is_voltage_good) {
				App_Transmit(&sendData, retransmissionNum++);
			}

			// in last iteration we do not sleep because we need to transmit data before sleep
			if (i != 29) {
				App_SleepLong();
			}
		}

		MeasurementData_Copy(&sendData, &workingData);
		PacketId_Increment(&packetId);
		retransmissionNum = 0;
		App_Transmit(&sendData, retransmissionNum++);

		App_SleepLong();
	}

}
