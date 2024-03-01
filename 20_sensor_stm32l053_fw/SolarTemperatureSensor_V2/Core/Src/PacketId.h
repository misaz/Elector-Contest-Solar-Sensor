#ifndef PACKET_ID_H_
#define PACKET_ID_H_

#include <stdint.h>

typedef struct {
	uint16_t bootId;
	uint16_t packetCounter;
} PacketId;

void PacketId_Init(PacketId* id);
void PacketId_Increment(PacketId* id);

#endif
