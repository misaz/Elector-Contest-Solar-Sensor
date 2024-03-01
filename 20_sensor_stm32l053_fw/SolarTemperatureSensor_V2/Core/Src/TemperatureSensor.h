#ifndef TEMPERATURESENSOR_H_
#define TEMPERATURESENSOR_H_

#include <stdint.h>

int TemperatureSensor_BeginMeasurement();
int TemperatureSensor_GetTemperature(int16_t* temperatureRaw);

#endif
