#ifndef __KEGERATOR_H__
#define __KEGERATOR_H__

#define MIN_TEMP 30
#define MAX_TEMP 55

#define OVERSAMPLE 8

extern SemaphoreHandle_t temp_mutex;
extern uint16_t current_temp;
extern uint16_t setpoint;
extern uint8_t setpoint_dirty;

void write_config();
#endif