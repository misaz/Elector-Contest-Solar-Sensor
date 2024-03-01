#ifndef MEASUREMENT_DATA_H_
#define MEASUREMENT_DATA_H_

#include <stdint.h>

typedef struct {
	int32_t temperatureSum;
	int16_t temperatureMin;
	int16_t temperatureMax;

	uint16_t voltage10;
	uint16_t voltage20;
	uint16_t voltage30;

	uint8_t temperaturesCount;
	uint8_t isVoltage10Loaded;
	uint8_t isVoltage20Loaded;
	uint8_t isVoltage30Loaded;
} MeasurementData;

void MeasurementData_Reset(MeasurementData* data);
void MeasurementData_Copy(MeasurementData* destination, MeasurementData* source);

#endif
