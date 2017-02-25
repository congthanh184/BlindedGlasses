#ifndef ALERT_FUNC
#define ALERT_FUNC 

#include <Energia.h>
#include <stdint.h>
#include "arduino_communication.h"

#ifdef __cplusplus
extern "C"{
#endif

void AlertSystemOpen();
void AlertSystemPoll();
extern void StartCountTime(uint32_t*);
extern uint8_t IsTimeExpired(uint32_t, uint16_t);

#ifdef __cplusplus
} // extern "C"
#endif
#endif