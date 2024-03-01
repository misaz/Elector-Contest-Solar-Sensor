#include "VoltageMonitor.h"
#include "main.h"

#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_adc.h"
#include "stm32l0xx_ll_gpio.h"

extern ADC_HandleTypeDef hadc;

void MX_ADC_Init(void);

#define OVERSAMPLING_NUM 7

static void VoltageMonitor_EnableRdiv() {
	LL_GPIO_ResetOutputPin(VBATT_ADC_EN_GPIO_Port, VBATT_ADC_EN_Pin);
	LL_GPIO_SetPinMode(VBATT_ADC_EN_GPIO_Port, VBATT_ADC_EN_Pin, LL_GPIO_MODE_OUTPUT);
}

static void VoltageMonitor_DisableRdiv() {
	LL_GPIO_SetPinMode(VBATT_ADC_EN_GPIO_Port, VBATT_ADC_EN_Pin, LL_GPIO_MODE_ANALOG);
}

static int VoltageMonitor_AdcInit() {
	HAL_StatusTypeDef hStatus;

	VoltageMonitor_EnableRdiv();

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	hStatus = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (hStatus) {
		return 1;
	}

	__HAL_RCC_ADC1_CLK_ENABLE();

	hadc.Instance = ADC1;
	hadc.Init.OversamplingMode = ENABLE;
	hadc.Init.Oversample.Ratio = ADC_OVERSAMPLING_RATIO_16;
	hadc.Init.Oversample.RightBitShift = ADC_RIGHTBITSHIFT_4;
	hadc.Init.Oversample.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
	hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
	hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.DMAContinuousRequests = DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
	hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc.Init.LowPowerAutoWait = DISABLE;
	hadc.Init.LowPowerFrequencyMode = ENABLE;
	hadc.Init.LowPowerAutoPowerOff = DISABLE;

	hStatus = HAL_ADC_Init(&hadc);
	if (hStatus) {
		return 1;
	}

	return 0;
}

static int VoltageMonitor_AdcDeinit() {
	HAL_StatusTypeDef hStatus;

	hStatus = HAL_ADC_DeInit(&hadc);
	// error ignored

	// reset peripheral before disabling clock. Reduces power consumption from 14 uA to 1 uA
	__HAL_RCC_ADC1_FORCE_RESET();
	__HAL_RCC_ADC1_RELEASE_RESET();

	__HAL_RCC_ADC1_CLK_DISABLE();

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	hStatus = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (hStatus) {
		return 1;
	}

	VoltageMonitor_DisableRdiv();

	return 0;
}

static int VoltageMonitor_RunAdcConversion(uint32_t channel, uint32_t *value) {
	HAL_StatusTypeDef hStatus;
	ADC_ChannelConfTypeDef sConfig;
	sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
	sConfig.Channel = channel;

	// Enable Channel
	hStatus = HAL_ADC_ConfigChannel(&hadc, &sConfig);
	if (hStatus) {
		HAL_ADC_Stop(&hadc);
		return 200;
	}

	uint16_t sum = 0;

	for (int i = 0; i < OVERSAMPLING_NUM; i++) {
		hStatus = HAL_ADC_Start(&hadc);
		if (hStatus) {
			HAL_ADC_Stop(&hadc);
			return 201;
		}

		hStatus = HAL_ADC_PollForConversion(&hadc, 100);
		if (hStatus) {
			HAL_ADC_Stop(&hadc);
			return 202;
		}

		if (i > 0) {
			sum += HAL_ADC_GetValue(&hadc);
		}
	}

	hStatus = HAL_ADC_Stop(&hadc);
	if (hStatus) {
		HAL_ADC_Stop(&hadc);
		return 203;
	}

	*value = sum / (OVERSAMPLING_NUM - 1);

	return 0;
}

int VoltageMonitor_GetVoltages(VoltageMonitor_Voltages *voltages) {
	int status;

	status = VoltageMonitor_AdcInit();
	if (status) {
		return status;
	}

	uint32_t vrefInt = 0;
	uint32_t ch6 = 0;

	status = VoltageMonitor_RunAdcConversion(ADC_CHANNEL_VREFINT, &vrefInt);
	if (status) {
		return status;
	}

	status = VoltageMonitor_RunAdcConversion(ADC_CHANNEL_6, &ch6);
	if (status) {
		return status;
	}

	status = VoltageMonitor_AdcDeinit();
	if (status) {
		return status;
	}

	uint32_t vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(vrefInt, LL_ADC_RESOLUTION_12B);

	voltages->batteryVoltage = __LL_ADC_CALC_DATA_TO_VOLTAGE(vref, ch6, LL_ADC_RESOLUTION_12B) * (11300 + 22000) / 11300;

	return 0;

}

