#ifndef ARDUINO_COMMUNICATION
#define ARDUINO_COMMUNICATION 

#include <Energia.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include "alert_func.h"
#include "RTClib.h"

#ifdef __cplusplus
extern "C"{
#endif

enum { 
	VOICE_DIRECTION,
	VOICE_TIME,
	SYSTEM	
};
void ArdCommOpen();
uint8_t ArdCommWrite(uint8_t,char*);
uint8_t ArdCommWriteTime();
uint8_t ArdCommWriteVol();

#ifdef __cplusplus 
} // extern "C"
#endif
#endif