#include "app.h"
#include "radio_driver.h"
#include "cmsis_compiler.h"
#include <stdio.h>
#include "stm32wlxx_hal_uart.h"

static void App_InitRadio();
static void App_StartReceiving();
static void App_RadioInterruptHandler(RadioIrqMasks_t event);

static volatile int isPacketReceived = 0;

extern UART_HandleTypeDef hlpuart1;

#define RF_FREQUENCY								868400000
#define TX_OUTPUT_POWER								5
#define FSK_FDEV                                    25000
#define FSK_DATARATE                                10000
#define FSK_BANDWIDTH                               100000

void App_Init() {
	App_InitRadio();
	App_StartReceiving();
	HAL_UARTEx_DisableFifoMode(&hlpuart1);
}

void App_Run() {
	HAL_StatusTypeDef uartStatus;

	while (1) {
		if (isPacketReceived) {
			isPacketReceived = 0;

			PacketStatus_t packetInfo;
			uint8_t buffer[40];
			uint8_t size;
			SUBGRF_GetPayload(buffer, &size, sizeof(buffer));
			SUBGRF_GetPacketStatus(&packetInfo);

			uint8_t message[20];
			for (int i = 0; i < sizeof(message); i++) {
				message[i] = 0;
				uint16_t data1 = buffer[i * 2 + 0];
				uint16_t data2 = buffer[i * 2 + 1];
				uint16_t data = (data1 << 8) | data2;
				for (int j = 0; j < 8; j++) {
					uint16_t manchesterBit = (data & 0xC000) >> 14;
					data <<= 2;
					if (manchesterBit == 1) {
						message[i] = (message[i] << 1) | 0;
					} else if (manchesterBit == 2) {
						message[i] = (message[i] << 1) | 1;
					} else {
						// manchester error
					}
				}
			}

			char stringBuffer[80];

			for (int i = 0; i < sizeof(message); i++) {
				snprintf(stringBuffer, sizeof(stringBuffer), "%02x", message[i]);
				uartStatus = HAL_UART_Transmit(&hlpuart1, (uint8_t*)stringBuffer, 2, 100);
				if (uartStatus) {
					__BKPT(0);
				}
	 		}

			snprintf(stringBuffer, sizeof(stringBuffer),
					" STATUS=%d RSSI_SYNC=%d RSSI_AVG=%d FREQ_ERR=%ld\r\n",
					packetInfo.Params.Gfsk.RxStatus,
					packetInfo.Params.Gfsk.RssiSync,
					packetInfo.Params.Gfsk.RssiAvg,
					packetInfo.Params.Gfsk.FreqError);
			size_t len = strlen(stringBuffer);
			uartStatus = HAL_UART_Transmit(&hlpuart1, (uint8_t*)stringBuffer, len, 100);
			if (uartStatus) {
				__BKPT(0);
			}

		} else {
			__WFI();
		}
	}
}

static void App_InitRadio() {
	SUBGRF_Init(App_RadioInterruptHandler);

	// "By default, the SMPS clock detection is disabled and must be enabled before enabling the SMPS." (6.1 in RM0453)
	SUBGRF_WriteRegister(SUBGHZ_SMPSC0R, (SUBGRF_ReadRegister(SUBGHZ_SMPSC0R) | SMPS_CLK_DET_ENABLE));
	SUBGRF_SetRegulatorMode();

	SUBGRF_SetBufferBaseAddress(0x00, 0x00);

	SUBGRF_SetRfFrequency(RF_FREQUENCY);
	SUBGRF_SetRfTxPower(TX_OUTPUT_POWER);
	SUBGRF_SetStopRxTimerOnPreambleDetect(false);

	SUBGRF_SetPacketType(PACKET_TYPE_GFSK);

	ModulationParams_t modulationParams;
	modulationParams.PacketType = PACKET_TYPE_GFSK;
	modulationParams.Params.Gfsk.Bandwidth = SUBGRF_GetFskBandwidthRegValue(FSK_BANDWIDTH);
	modulationParams.Params.Gfsk.BitRate = FSK_DATARATE;
	modulationParams.Params.Gfsk.Fdev = FSK_FDEV;
	modulationParams.Params.Gfsk.ModulationShaping = MOD_SHAPING_OFF;
	SUBGRF_SetModulationParams(&modulationParams);

	PacketParams_t packetParams;
	packetParams.PacketType = PACKET_TYPE_GFSK;
	packetParams.Params.Gfsk.AddrComp = RADIO_ADDRESSCOMP_FILT_OFF;
	packetParams.Params.Gfsk.CrcLength = RADIO_CRC_OFF;
	packetParams.Params.Gfsk.DcFree = RADIO_DC_FREE_OFF;
	packetParams.Params.Gfsk.HeaderType = RADIO_PACKET_FIXED_LENGTH;
	packetParams.Params.Gfsk.PayloadLength = 40;
	packetParams.Params.Gfsk.PreambleLength = 32;
	packetParams.Params.Gfsk.PreambleMinDetect = RADIO_PREAMBLE_DETECTOR_08_BITS;
	packetParams.Params.Gfsk.SyncWordLength = 32; // bytes to bits
	SUBGRF_SetPacketParams(&packetParams);

	SUBGRF_SetSyncWord((uint8_t[] ) { 0x93, 0x0B, 0x51, 0xDE, 0x00, 0x00, 0x00, 0x00 });
	// SUBGRF_SetWhiteningSeed(0x01FF);

	HAL_Delay(1000);
}

static void App_StartReceiving() {
	HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, 1);
	HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, 1);

	uint16_t irq = IRQ_RX_DONE;
	SUBGRF_SetDioIrqParams(irq, irq, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
	SUBGRF_SetSwitch(RFO_LP, RFSWITCH_RX);
	SUBGRF_SetRx(0xFFFFFF);

	HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, 0);
}

static void App_RadioInterruptHandler(RadioIrqMasks_t event) {
	if (event == IRQ_RX_DONE) {
		isPacketReceived = 1;
	}
}
