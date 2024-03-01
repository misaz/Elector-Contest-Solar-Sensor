/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_DMA_Init(void);
void MX_I2C2_Init(void);
void MX_RTC_Init(void);
void MX_ADC_Init(void);
void MX_SPI1_Init(void);
void MX_TIM6_Init(void);
void MX_RNG_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UNUSED11_Pin GPIO_PIN_13
#define UNUSED11_GPIO_Port GPIOC
#define XTAL_16M_UNUSED1_Pin GPIO_PIN_0
#define XTAL_16M_UNUSED1_GPIO_Port GPIOH
#define XTAL_16M_UNUSED2_Pin GPIO_PIN_1
#define XTAL_16M_UNUSED2_GPIO_Port GPIOH
#define UNUSED0_Pin GPIO_PIN_0
#define UNUSED0_GPIO_Port GPIOA
#define UNUSED1_Pin GPIO_PIN_1
#define UNUSED1_GPIO_Port GPIOA
#define UNUSED2_Pin GPIO_PIN_2
#define UNUSED2_GPIO_Port GPIOA
#define VBATT_ADC_EN_Pin GPIO_PIN_3
#define VBATT_ADC_EN_GPIO_Port GPIOA
#define UNUSED3_Pin GPIO_PIN_4
#define UNUSED3_GPIO_Port GPIOA
#define UNUSED4_Pin GPIO_PIN_5
#define UNUSED4_GPIO_Port GPIOA
#define VBATT_ADC_IN_Pin GPIO_PIN_6
#define VBATT_ADC_IN_GPIO_Port GPIOA
#define UNUSED5_Pin GPIO_PIN_7
#define UNUSED5_GPIO_Port GPIOA
#define UNUSED8_Pin GPIO_PIN_0
#define UNUSED8_GPIO_Port GPIOB
#define HRVST_PGOOD_Pin GPIO_PIN_1
#define HRVST_PGOOD_GPIO_Port GPIOB
#define UNUSED9_Pin GPIO_PIN_2
#define UNUSED9_GPIO_Port GPIOB
#define HRVST_LLD_Pin GPIO_PIN_10
#define HRVST_LLD_GPIO_Port GPIOB
#define TEMP_GPIO1_Pin GPIO_PIN_11
#define TEMP_GPIO1_GPIO_Port GPIOB
#define TEMP_GPIO0_Pin GPIO_PIN_12
#define TEMP_GPIO0_GPIO_Port GPIOB
#define UNUSED11_GND_Pin GPIO_PIN_15
#define UNUSED11_GND_GPIO_Port GPIOB
#define TEMP_GND_Pin GPIO_PIN_8
#define TEMP_GND_GPIO_Port GPIOA
#define DBG1_Pin GPIO_PIN_9
#define DBG1_GPIO_Port GPIOA
#define DBG2_Pin GPIO_PIN_10
#define DBG2_GPIO_Port GPIOA
#define UNUSED6_GND_Pin GPIO_PIN_11
#define UNUSED6_GND_GPIO_Port GPIOA
#define UNUSED7_GND_Pin GPIO_PIN_12
#define UNUSED7_GND_GPIO_Port GPIOA
#define RADIO_CSB_Pin GPIO_PIN_15
#define RADIO_CSB_GPIO_Port GPIOA
#define RADIO_SCLK_Pin GPIO_PIN_3
#define RADIO_SCLK_GPIO_Port GPIOB
#define RADIO_MISO_Pin GPIO_PIN_4
#define RADIO_MISO_GPIO_Port GPIOB
#define RADIO_MOSI_Pin GPIO_PIN_5
#define RADIO_MOSI_GPIO_Port GPIOB
#define HRVST_DIS_SW_Pin GPIO_PIN_6
#define HRVST_DIS_SW_GPIO_Port GPIOB
#define UNUSED10_GND_Pin GPIO_PIN_7
#define UNUSED10_GND_GPIO_Port GPIOB
#define HRVST_REG_D0_Pin GPIO_PIN_8
#define HRVST_REG_D0_GPIO_Port GPIOB
#define HRVST_REG_D1_Pin GPIO_PIN_9
#define HRVST_REG_D1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
