#ifndef BUTTON_H
#define BUTTON_H 

#include <Energia.h>
#include <stdint.h>
// #include "alert_func.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum { 
    BUTTON_RELEASE = 0,
    BUTTON_PRESS_IN_PROCESS,
    BUTTON_PRESS,
    BUTTON_RELEASE_IN_PROCESS, 
    BUTTON_HOLD
} Button_State_Type;

typedef enum {  
	BUTTON_TIME,
	BUTTON_VOL 
} Button_Name_Type;

void ButtonOpen();
void ButtonPoll();
void ButtonRead(uint8_t *buttonId, Button_State_Type *state);
void ButtonClear();

// extern static void (*button_state_table[])();

#ifdef __cplusplus
} // extern "C"
#endif
#endif
