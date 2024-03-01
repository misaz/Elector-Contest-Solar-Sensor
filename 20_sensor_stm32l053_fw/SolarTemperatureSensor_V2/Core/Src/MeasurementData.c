#include "MeasurementData.h"

#include <limits.h>

void MeasurementData_Reset(MeasurementData* data) {
	data->temperatureMax = INT16_MIN;
	data->temperatureMin = INT16_MAX;
	data->temperatureSum = 0;
	data->temperaturesCount = 0;

	data->voltage10 = 0;
	data->voltage20 = 0;
	data->voltage30 = 0;

	data->isVoltage10Loaded = 0;
	data->isVoltage20Loaded = 0;
	data->isVoltage30Loaded = 0;
}

void MeasurementData_Copy(MeasurementData* destination, MeasurementData* source) {
	destination->temperatureMax = source->temperatureMax;
	destination->temperatureMin = source->temperatureMin;
	destination->temperatureSum = source->temperatureSum;
	destination->temperaturesCount = source->temperaturesCount;

	destination->voltage10 = source->voltage10;
	destination->voltage20 = source->voltage20;
	destination->voltage30 = source->voltage30;

	destination->isVoltage10Loaded = source->isVoltage10Loaded;
	destination->isVoltage20Loaded = source->isVoltage20Loaded;
	destination->isVoltage30Loaded = source->isVoltage30Loaded;
}
