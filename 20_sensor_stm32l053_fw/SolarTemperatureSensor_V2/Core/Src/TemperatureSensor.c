#include "TemperatureSensor.h"
#include "App.h"

#include "stm32l0xx_hal.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c2;

static int TemperatureSensor_PowerOn() {
	// set device address (GPIO0, GPIO1) and power on temperature sensor
	HAL_GPIO_WritePin(TEMP_GPIO0_GPIO_Port, TEMP_GPIO0_Pin, 1);
	HAL_GPIO_WritePin(TEMP_GPIO1_GPIO_Port, TEMP_GPIO1_Pin, 1);
	HAL_GPIO_WritePin(TEMP_GND_GPIO_Port, TEMP_GND_Pin, 0);

	// wait for sensor powerup
	App_SleepShortMs(1);

	// Init I2C Driver
	__HAL_RCC_I2C2_CLK_ENABLE();
	MX_I2C2_Init();

	return 0;
}

static int TemperatureSensor_PowerDown() {
	HAL_StatusTypeDef hStatus;

	hStatus = HAL_I2C_DeInit(&hi2c2);
	// error ignored
	(void)hStatus;

	// shutdown I2C peripheral
	__HAL_RCC_I2C2_FORCE_RESET();
	__HAL_RCC_I2C2_RELEASE_RESET();
	__HAL_RCC_I2C2_CLK_DISABLE();

	// power off the sensor
	HAL_GPIO_WritePin(TEMP_GPIO0_GPIO_Port, TEMP_GPIO0_Pin, 1);
	HAL_GPIO_WritePin(TEMP_GPIO1_GPIO_Port, TEMP_GPIO1_Pin, 1);
	HAL_GPIO_WritePin(TEMP_GND_GPIO_Port, TEMP_GND_Pin, 1);

	return 0;
}

int TemperatureSensor_BeginMeasurement() {
	int iStatus;
	HAL_StatusTypeDef hStatus;

	iStatus = TemperatureSensor_PowerOn();
	if (iStatus) {
		TemperatureSensor_PowerDown();
		return iStatus;
	}

	// read part ID
	/*
	uint8_t partId = 0;
	hStatus = HAL_I2C_Mem_Read(&hi2c2, 0xA6, 0xFF, 1, &partId, sizeof(partId), 100);
	if (hStatus) {
		TemperatureSensor_PowerDown();
		return 1;
	}
	*/

	// trigger conversion
	uint8_t tempSetup = 0xC1;
	hStatus = HAL_I2C_Mem_Write(&hi2c2, 0xA6, 0x14, 1, &tempSetup, sizeof(tempSetup), 100);
	if (hStatus) {
		TemperatureSensor_PowerDown();
		return 1;
	}

	return 0;
}


int TemperatureSensor_GetTemperature(int16_t* temperatureRaw) {
	HAL_StatusTypeDef hStatus;

	uint8_t fifoCount = 0;
	uint8_t retries = 25;
	do {
		hStatus = HAL_I2C_Mem_Read(&hi2c2, 0xA6, 0x07, 1, &fifoCount, sizeof(fifoCount), 100);
		if (hStatus) {
			TemperatureSensor_PowerDown();
			return 1;
		}

		if (fifoCount != 1) {
			App_SleepShortMs(1);
		}
	} while (fifoCount != 1 && retries--);

	if (fifoCount != 1) {
		TemperatureSensor_PowerDown();
		return 1;
	}

	// read data
	uint8_t temperatureBytes[2];
	hStatus = HAL_I2C_Mem_Read(&hi2c2, 0xA6, 0x08, 1, temperatureBytes, sizeof(temperatureBytes), 100);
	if (hStatus) {
		TemperatureSensor_PowerDown();
		return 1;
	}

	uint16_t temperatureUint = (((uint16_t) (temperatureBytes[0])) << 8) | (uint16_t) (temperatureBytes[1]);

	*temperatureRaw = (int16_t)temperatureUint;

	TemperatureSensor_PowerDown();

	return 0;
}

