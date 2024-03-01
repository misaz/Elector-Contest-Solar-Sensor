#ifndef VOLTAGEMONITOR_H_
#define VOLTAGEMONITOR_H_

#include <stdint.h>

typedef struct {
	int16_t batteryVoltage;
	int16_t vccVoltage;
} VoltageMonitor_Voltages;

int VoltageMonitor_GetVoltages(VoltageMonitor_Voltages* voltages);

#endif
