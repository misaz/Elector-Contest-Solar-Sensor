#include "PacketId.h"
#include "Security.h"

void PacketId_Init(PacketId* id) {
	id->bootId = Security_GenerateBootId();
	id->packetCounter = 0;
}

void PacketId_Increment(PacketId* id) {
	id->packetCounter++;
	if (id->packetCounter == 0) {
		id->bootId = Security_GenerateBootId();
	}
}
