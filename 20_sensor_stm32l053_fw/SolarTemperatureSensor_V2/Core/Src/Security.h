#ifndef SECURITY_H_
#define SECURITY_H_

#include <stdint.h>
#include <stddef.h>

int Security_Init();

int Security_EncryptBlock(uint8_t *plaintext, uint8_t *encrypted, size_t plaintextSize);

uint32_t Security_GetDeviceId();
uint16_t Security_GenerateBootId();

#endif
