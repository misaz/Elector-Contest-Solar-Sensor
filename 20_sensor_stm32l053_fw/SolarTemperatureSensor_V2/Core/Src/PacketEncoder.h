#ifndef PACKET_ENCODER_H_
#define PACKET_ENCODER_H_

#include "MeasurementData.h"
#include "PacketId.h"

#include <stdint.h>
#include <stddef.h>

int PacketEncoder_EncodeNewDataPacket(uint8_t *buffer, size_t bufferSize, MeasurementData *data, PacketId* packetId, int retransmissionNumber);

#endif
