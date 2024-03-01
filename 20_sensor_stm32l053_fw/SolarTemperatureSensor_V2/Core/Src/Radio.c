#include "Radio.h"
#include "App.h"

#include "main.h"
#include "stm32l0xx_hal.h"

extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim6;
extern DMA_HandleTypeDef hdma_tim6_up;

#define GPIO_MOSI_BRSS_SET RADIO_MOSI_Pin
#define GPIO_MOSI_BRSS_RESET (RADIO_MOSI_Pin << 16)

#define GPIO_CSB_BRSS_SET RADIO_CSB_Pin
#define GPIO_CSB_BRSS_RESET (RADIO_CSB_Pin << 16)

#define ABUSED_TIMER_OUT_SET (TIM_CCER_CC2E | TIM_CCER_CC2P)
#define ABUSED_TIMER_OUT_RESET (TIM_CCER_CC2E)

static void Radio_ReInitGpio() {
	// configure MAX41460 input pins to driven value (reduces MAX41460 power consumption) before first use
	GPIO_InitTypeDef gpio;
	gpio.Alternate = 0;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_LOW;

	gpio.Pin = RADIO_MOSI_Pin;
	HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 0);
	HAL_GPIO_Init(RADIO_MOSI_GPIO_Port, &gpio);

	gpio.Pin = RADIO_SCLK_Pin;
	HAL_GPIO_WritePin(RADIO_SCLK_GPIO_Port, RADIO_SCLK_Pin, 0);
	HAL_GPIO_Init(RADIO_SCLK_GPIO_Port, &gpio);
}

void Radio_Init() {
	Radio_ReInitGpio();

	// cycle MAX41460 DATA pin according to "Power-On Programming" section in MAX41460 datasheet rev. 2
	HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 0);
	App_SleepShortMs(1);
	HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 1);
	App_SleepShortMs(1);
	HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 0);
	App_SleepShortMs(1);
	HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 1);
	App_SleepShortMs(1);
	HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 0);
}


static void Radio_PowerOn() {
	// Init SPI Driver
	__HAL_RCC_SPI1_CLK_ENABLE();
	MX_SPI1_Init();

	// init DMA Driver (DMA must be enabled before MX_TIM6_Init call; this call configures it)
	__HAL_RCC_DMA1_CLK_ENABLE();

	// init TIM6 (data timing) Driver
	__HAL_RCC_TIM6_CLK_ENABLE();
	MX_TIM6_Init();

	// init TIM22 (abused output) Driver
	__HAL_RCC_TIM22_CLK_ENABLE();
}

static void Radio_PowerOff() {
	HAL_StatusTypeDef hStatus;

	HAL_GPIO_WritePin(RADIO_CSB_GPIO_Port, RADIO_CSB_Pin, 1);

	hStatus = HAL_SPI_DeInit(&hspi1);
	// error ignored
	(void) hStatus;

	// shutdown SPI peripheral
	__HAL_RCC_SPI1_FORCE_RESET();
	__HAL_RCC_SPI1_RELEASE_RESET();
	__HAL_RCC_SPI1_CLK_DISABLE();

	hStatus = HAL_TIM_Base_DeInit(&htim6);
	// error ignored
	(void) hStatus;

	// shutdown SPI peripheral
	__HAL_RCC_TIM6_FORCE_RESET();
	__HAL_RCC_TIM6_RELEASE_RESET();
	__HAL_RCC_TIM6_CLK_DISABLE();

	// shutdown DMA peripheral
	__HAL_RCC_DMA1_FORCE_RESET();
	__HAL_RCC_DMA1_RELEASE_RESET();
	__HAL_RCC_DMA1_CLK_DISABLE();

	// shutdown TIM22 (abused output) peripheral
	__HAL_RCC_TIM22_FORCE_RESET();
	__HAL_RCC_TIM22_RELEASE_RESET();
	__HAL_RCC_TIM22_CLK_DISABLE();

	// reinitialize GPIOs
	Radio_ReInitGpio();
}

#define MAX41460_CsbSet() \
	RADIO_CSB_GPIO_Port->BSRR = GPIO_CSB_BRSS_SET;

#define MAX41460_CsbReset() \
	RADIO_CSB_GPIO_Port->BSRR = GPIO_CSB_BRSS_RESET;

#define MAX41460_WriteByte(data) \
	SPI1->DR = data; \
	while ((SPI1->SR & SPI_SR_TXE) == 0) {}

#define MAX41460_WriteRegExitCs(addr, val, csbClr, csbSet) \
	MAX41460_CsbReset(); \
	MAX41460_WriteByte(addr); \
	MAX41460_WriteByte(val); \
	MAX41460_CsbSet();

#define MAX41460_WriteReg(addr, val, csbClr) \
	MAX41460_CsbReset(); \
	MAX41460_WriteByte(addr); \
	MAX41460_WriteByte(val);


#define Radio_Delay15Us __NOP(); __NOP(); __NOP

#define Radio_Delay125Us \
	__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
	__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
	__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
	__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
	__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); \
	__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();  \
	__NOP

/*
static uint32_t MAX41460_GetMosiGpioModeUpdateValue(uint32_t newMode) {
	int RADIO_MOSI_PinNum = -1;
	for (int i = 0; i < 16; i++) {
		if ((1 << i) == RADIO_MOSI_Pin) {
			RADIO_MOSI_PinNum = i;
		}
	}

	return (RADIO_MOSI_GPIO_Port->MODER & (~(0x3 << (RADIO_MOSI_PinNum * 2)))) | (newMode << (RADIO_MOSI_PinNum * 2));
}
*/
static uint32_t MAX41460_GetMosiGpioAfUpdateValue(volatile uint32_t** reg, uint32_t newAf) {
	int RADIO_MOSI_PinNum = -1;
	for (int i = 0; i < 16; i++) {
		if ((1 << i) == RADIO_MOSI_Pin) {
			RADIO_MOSI_PinNum = i;
		}
	}
	if (RADIO_MOSI_PinNum > 7) {
		*reg = RADIO_MOSI_GPIO_Port->AFR + 1;
	} else {
		*reg = RADIO_MOSI_GPIO_Port->AFR + 0;
	}

	RADIO_MOSI_PinNum &= 0x7;

	return (**reg & (~(0xF << (RADIO_MOSI_PinNum * 4)))) | (newAf << (RADIO_MOSI_PinNum * 4));
}

static void MAX41460_InitialProgramming() {
	// forward GPIO preparation. Switching GPIO mode take one register write (no read) later
	// HAL_GPIO_WritePin(RADIO_MOSI_GPIO_Port, RADIO_MOSI_Pin, 0);
	// uint32_t mosiModeGpioValue = MAX41460_GetMosiGpioModeUpdateValue(MODE_OUTPUT);

	volatile uint32_t* mosiAfTimChangeReg;
	uint32_t mosiAfTimChangeVal = MAX41460_GetMosiGpioAfUpdateValue(&mosiAfTimChangeReg, GPIO_AF4_TIM22);

	SPI1->CR1 |= SPI_CR1_SPE;

	// transmit dummy byte with CS=1 to stabilize clock low
	MAX41460_WriteByte(0);

	// assert MAX41460 reset
	Radio_Delay15Us();
	MAX41460_CsbReset();
	Radio_Delay125Us();
	Radio_Delay125Us();
	MAX41460_WriteByte(MAX41460_REG_CFG8);
	MAX41460_WriteByte(0x01); /* MAX41460_REG_CFG8 */
	MAX41460_CsbSet();
	Radio_Delay125Us();

	// deassert MAX41460 reset
	MAX41460_CsbReset();
	Radio_Delay125Us();
	Radio_Delay125Us();
	MAX41460_WriteByte(MAX41460_REG_CFG8);
	MAX41460_WriteByte(0x00); /* MAX41460_REG_CFG8 */
	MAX41460_CsbSet();
	Radio_Delay125Us();

	// initial programming
	MAX41460_CsbReset();
	Radio_Delay125Us();
	Radio_Delay125Us();

	MAX41460_WriteByte(MAX41460_REG_CFG1); // write bit + register address
	MAX41460_WriteByte(0x91); /* MAX41460_REG_CFG1 */
	MAX41460_WriteByte(0x81); /* MAX41460_REG_CFG2 */
	MAX41460_WriteByte(0x03); /* MAX41460_REG_CFG3 */
	MAX41460_WriteByte(0x00); /* MAX41460_REG_CFG4 */
	MAX41460_WriteByte(0x00); /* MAX41460_REG_CFG5 */
	MAX41460_WriteByte(0x04); /* MAX41460_REG_SHDN */
	MAX41460_WriteByte(0x87); /* MAX41460_REG_PA1  */
	MAX41460_WriteByte(0x80); /* MAX41460_REG_PA2  */
	MAX41460_WriteByte(0x60); /* MAX41460_REG_PLL1 */
	MAX41460_WriteByte(0x00); /* MAX41460_REG_PLL2 */
	MAX41460_WriteByte(0x00); /* MAX41460_REG_CFG6 */

	// FSK 868.40 MHz (in CZE 1% Duty Cycle Limit)
	MAX41460_WriteByte(0x36); /* MAX41460_REG_PLL3 */
	MAX41460_WriteByte(0x46); /* MAX41460_REG_PLL4 */
	MAX41460_WriteByte(0x67); /* MAX41460_REG_PLL5 */
	MAX41460_WriteByte(0x1A); /* MAX41460_REG_PLL6 */
	MAX41460_WriteByte(0x03); /* MAX41460_REG_PLL7 */

	// ASK 868.40 MHz (in CZE 1% Duty Cycle Limit)
	// MAX41460_WriteByte(0x36); /* MAX41460_REG_PLL3 */
	// MAX41460_WriteByte(0x48); /* MAX41460_REG_PLL4 */
	// MAX41460_WriteByte(0x66); /* MAX41460_REG_PLL5 */
	// MAX41460_WriteByte(0x28); /* MAX41460_REG_PLL6 */
	// MAX41460_WriteByte(0x04); /* MAX41460_REG_PLL7 */

	// ASK 869.50 MHz (in CZE 10% Duty Cycle Limit)
	// MAX41460_WriteByte(0x36); /* MAX41460_REG_PLL3 */
	// MAX41460_WriteByte(0x58); /* MAX41460_REG_PLL4 */
	// MAX41460_WriteByte(0x00); /* MAX41460_REG_PLL5 */
	// MAX41460_WriteByte(0x28); /* MAX41460_REG_PLL6 */
	// MAX41460_WriteByte(0x04); /* MAX41460_REG_PLL7 */


	// Tpll
	Radio_Delay125Us();

	MAX41460_CsbSet();

	// time since start > Tpll_MICROSECONDS + Txo_MICROSECONDS
	// time since start > 333uS
	Radio_Delay125Us();

	MAX41460_CsbReset();
	Radio_Delay125Us();
	Radio_Delay125Us();

	// write bit + register address
	MAX41460_WriteByte(MAX41460_REG_CFG7);
	MAX41460_WriteByte(0x02); /* MAX41460_REG_CFG7 */

	// switch MISO control from SPI to TIM21
	// RADIO_MOSI_GPIO_Port->MODER = mosiModeGpioValue;
	TIM22->CCER = ABUSED_TIMER_OUT_RESET;
	*mosiAfTimChangeReg = mosiAfTimChangeVal;

	// Ttx_MICROSECONDS
	// 15us
	Radio_Delay15Us();
}

int Radio_TransmitMessage(uint8_t *message, size_t len) {
	if (len > RADIO_TRANSMIT_MAX_LEN_BYTES) {
		return 1;
	}

	// currently unused: + RADIO_TRANSMIT_TERMINATION_PULSES * 2
	uint32_t gpioBsrrCommands[RADIO_TRANSMIT_PREAMBLE_PULSES * 2 + RADIO_TRANSMIT_SYNC_BYTES * 8 + (RADIO_TRANSMIT_MAX_LEN_BYTES + 1) * 8 * 2];
	uint32_t* gpioBsrrCommandsWr = gpioBsrrCommands;

	for (size_t i = 0; i < RADIO_TRANSMIT_PREAMBLE_PULSES; i++) {
		*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_SET;
		*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_RESET;
	}
	for (int i = RADIO_TRANSMIT_SYNC_BYTES * 8 - 1; i >= 0; i--) {
		if (0x930B51DE & (1 << i)) {
			*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_SET;
		} else {
			*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_RESET;
		}
	}
	for (size_t i = 0; i < len; i++) {
		for (int j = 7; j >= 0; j--) {
			// manchester encoding

			if (message[i] & (1 << j)) {
				*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_SET;
				*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_RESET;
			} else {
				*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_RESET;
				*gpioBsrrCommandsWr++ = ABUSED_TIMER_OUT_SET;
			}
		}
	}
	for (size_t i = 0; i < 8; i++) {
		*gpioBsrrCommandsWr++ = GPIO_MOSI_BRSS_SET;
		*gpioBsrrCommandsWr++ = GPIO_MOSI_BRSS_RESET;
	}
	len = gpioBsrrCommandsWr - gpioBsrrCommands;

	// setup STM32 peripherals
	Radio_PowerOn();

	// configure DMA + transfer parameters
	DMA1->IFCR = DMA_IFCR_CGIF2;
	DMA1_CSELR->CSELR = (DMA_REQUEST_9 << DMA_CSELR_C2S_Pos);
	// DMA1_Channel2->CPAR = (uint32_t)&RADIO_MOSI_GPIO_Port->BSRR;
	DMA1_Channel2->CPAR = (uint32_t)&TIM22->CCER;
	DMA1_Channel2->CMAR = (uint32_t)gpioBsrrCommands;
	DMA1_Channel2->CNDTR = len;
	DMA1_Channel2->CCR = DMA_PRIORITY_HIGH | DMA_MDATAALIGN_WORD | DMA_PDATAALIGN_WORD | DMA_MINC_ENABLE | DMA_MEMORY_TO_PERIPH | DMA_CCR_EN;

	// set transmission RF parameters and start transmission
	MAX41460_InitialProgramming();

	// start timer generating DMA triggers
	TIM6->DIER = TIM_DMA_UPDATE;
	TIM6->CR1 |= TIM_CR1_CEN;

	// wait for DMA completion
	while ((DMA1->ISR & DMA_IFCR_CTCIF2) == 0) {}

	// stop RF tramisssion
	MAX41460_CsbSet();

	// reconfigure MOSI from GPIO back to SPI
	// RADIO_MOSI_GPIO_Port->MODER = MAX41460_GetMosiGpioModeUpdateValue(MODE_AF);
	volatile uint32_t* mosiAfSpiChangeReg;
	uint32_t mosiAfSpiChangeVal = MAX41460_GetMosiGpioAfUpdateValue(&mosiAfSpiChangeReg, GPIO_AF0_SPI1);
	*mosiAfSpiChangeReg = mosiAfSpiChangeVal;

	// reset MAX41460 chip
	Radio_Delay15Us();
	MAX41460_CsbReset();
	MAX41460_WriteByte(MAX41460_REG_CFG8);
	MAX41460_WriteByte(0x01); /* MAX41460_REG_CFG8 */
	MAX41460_CsbSet();

	// shutdown STM32 peripherals
	Radio_PowerOff();

	return 0;
}
