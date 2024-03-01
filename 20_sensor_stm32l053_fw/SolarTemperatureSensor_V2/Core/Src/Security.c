#include "Security.h"
#include "App.h"

#include "wolfSSL.I-CUBE-wolfSSL_conf.h"
#include <wolfssl/wolfcrypt/aes.h>

#include "main.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_utils.h"

static uint32_t deviceId;

static const __attribute__((section(".encryption_key"))) uint8_t deviceIv[16] = { 0x2a, 0xda, 0xc6, 0x5e, 0x4a, 0x6c, 0x07, 0xde, 0x32, 0x49, 0xf5, 0xb3, 0x1c, 0x6c, 0x42, 0xc0 };
static const __attribute__((section(".encryption_key"))) uint8_t deviceKey[16] = { ... enter your AES key here ... };

extern RNG_HandleTypeDef hrng;

static int Security_EncryptAes128Block(uint8_t *plaintext, uint8_t *encrypted, const uint8_t *key, const uint8_t *iv) {
	Aes aes;
	int aesStatus;

	aesStatus = wc_AesInit(&aes, NULL, INVALID_DEVID);
	if (aesStatus) {
		wc_AesFree(&aes);
		return 1;
	}

	aesStatus = wc_AesSetIV(&aes, iv);
	if (aesStatus) {
		wc_AesFree(&aes);
		return 1;
	}

	aesStatus = wc_AesSetKey(&aes, key, 16, iv, AES_ENCRYPTION);
	if (aesStatus) {
		wc_AesFree(&aes);
		return 1;
	}

	aesStatus = wc_AesCbcEncrypt(&aes, encrypted, plaintext, 16);
	if (aesStatus) {
		wc_AesFree(&aes);
		return 1;
	}

	return 0;
}

static int Security_ComputeDeviceId(uint32_t *deviceId) {
	// following credentials can be the same for every device
	// AES is used as a replacement for has function. I do not integrate hashing support
	// with wolfCrypt for reducing flash size requirements.
	static const uint8_t iv[16] = { 0xd7, 0x2c, 0xc2, 0xf7, 0x65, 0x03, 0x99, 0x48, 0x1b, 0x66, 0x64, 0x89, 0xda, 0x36, 0x17, 0x3e };
	static const uint8_t key[16] = { 0x35, 0x58, 0xc6, 0x07, 0x85, 0x0e, 0x8a, 0xdb, 0x40, 0xfe, 0xb3, 0x33, 0x10, 0xfe, 0xcc, 0x3d };

	uint32_t plaintext[4] = { LL_GetUID_Word0(), LL_GetUID_Word1(), LL_GetUID_Word2(), 0 };
	uint32_t encrypted[4] = { 0, 0, 0, 0 };

	int status = Security_EncryptAes128Block((uint8_t*)plaintext, (uint8_t*)encrypted, key, iv);
	if (status) {
		return status;
	}

	*deviceId = encrypted[0] ^ encrypted[1] ^ encrypted[2] ^ encrypted[3];

	return 0;
}

int Security_Init() {
	return Security_ComputeDeviceId(&deviceId);
}

uint32_t Security_GetDeviceId() {
	return deviceId;
}

int Security_EncryptBlock(uint8_t *plaintext, uint8_t *encrypted, size_t plaintextSize) {
	if (plaintextSize != 16) {
		return 1;
	}

	return Security_EncryptAes128Block(plaintext, encrypted, deviceKey, deviceIv);
}

uint16_t Security_GenerateBootId() {
	__HAL_RCC_RNG_CLK_ENABLE();
	MX_RNG_Init();

	uint32_t randomNumber;

	HAL_StatusTypeDef hStatus = -1;
	uint32_t attemps = 100;
	while (hStatus != 0 && attemps--) {
		hStatus = HAL_RNG_GenerateRandomNumber(&hrng, &randomNumber);
		if (hStatus) {
			App_SleepShortMs(1);
		}
	}

	HAL_RNG_DeInit(&hrng);
	__HAL_RCC_RNG_FORCE_RESET();
	__HAL_RCC_RNG_RELEASE_RESET();
	__HAL_RCC_RNG_CLK_DISABLE();

	if (hStatus)  {
		return 0x7977;
	} else {
		return (uint16_t)randomNumber;
	}

}
