#include "button.h"
#include "hardware_profile.h"
#include "alert_func.h"

static const uint8_t STATE_ACTIVE    = LOW;
static const uint8_t STATE_IDLE      = HIGH;

static const uint16_t BUTTON_TIME_BOUNCE = 20;
static const uint16_t BUTTON_TIME_HOLD   = 1000;

// This array has to agree in term of arragement
// with enum Button_Name_Type
const uint8_t BUTTON_ARRAY[] = {
    PA_3, 
    PA_2
};

static void ButtonRelease();
static void ButtonPressInProcess();
static void ButtonPress();
static void ButtonReleaseInProcess();

static void (*button_state_table[])() = {
    ButtonRelease,
    ButtonPressInProcess,
    ButtonPress,
    ButtonReleaseInProcess
};

static Button_State_Type button_current_state;
static Button_State_Type buttonStateOutput;
static uint8_t buttonIdOutput;
static uint32_t previousMillis;
static uint8_t buttonOnActiveId;

/*--------------------------------------------------------------*/
void ButtonOpen() {
    // pinMode(PUSH1, INPUT_PULLUP);
    // pinMode(PUSH2, INPUT_PULLUP);
    for (int i = 0; i < sizeof(BUTTON_ARRAY); ++i)
    {
        pinMode(BUTTON_ARRAY[i], INPUT);
    }

    button_current_state = BUTTON_RELEASE;
    buttonStateOutput = BUTTON_RELEASE;
    buttonIdOutput = 0;
}

void ButtonPoll() {
    button_state_table[button_current_state]();
}

void ButtonRead(uint8_t *buttonId, Button_State_Type *state) {
	*state = buttonStateOutput;
	*buttonId = buttonIdOutput;
}

void ButtonClear() {
	buttonStateOutput = BUTTON_RELEASE;
	buttonIdOutput = 0;
}
/*--------------------------------------------------------------*/
// void StartCountTime() {
//     previousMillis = millis();
// }

uint8_t IsAnyButtonActive(uint8_t *buttonId) {
    *buttonId = 0;
    // sizeof is okay, for button_array is 1byte-element array
    for (int i = 0; i < sizeof(BUTTON_ARRAY); ++i)
    {
        if (digitalRead(BUTTON_ARRAY[i])==STATE_ACTIVE)
        {
            *buttonId = i;
            return STATE_ACTIVE;
        }
    }
    return STATE_IDLE;
}

// uint8_t IsTimeExpired(uint32_t previousMillis, uint16_t waitTime) {
//     uint32_t currentMillis = millis();
//     if (currentMillis - previousMillis >= waitTime)
//     {
//         return 1;
//     }
//     return 0;
// }

void ButtonRelease() {
    if (IsAnyButtonActive(&buttonOnActiveId)==STATE_ACTIVE)
    {   
        button_current_state = BUTTON_PRESS_IN_PROCESS;
        StartCountTime(&previousMillis);
    }
}

void ButtonPressInProcess() {
    if (IsTimeExpired(previousMillis, BUTTON_TIME_BOUNCE)) 
    {
        if(digitalRead(BUTTON_ARRAY[buttonOnActiveId])==STATE_ACTIVE)
        {
            button_current_state = BUTTON_PRESS;
            // Serial.println("Button Pressed");
            StartCountTime(&previousMillis);
        }
        else 
        {
            button_current_state = BUTTON_RELEASE;
        }
    }
}

void ButtonPress() {
    if (IsTimeExpired(previousMillis, BUTTON_TIME_HOLD))
    {
        buttonIdOutput = buttonOnActiveId;
        buttonStateOutput = BUTTON_HOLD;
        StartCountTime(&previousMillis);

        // Serial.print(buttonOnActiveId);
        // Serial.print(" ");
        // Serial.println("Button Hold");
    }

    if (digitalRead(BUTTON_ARRAY[buttonOnActiveId])==STATE_IDLE)
    {
        button_current_state = BUTTON_RELEASE_IN_PROCESS;
        StartCountTime(&previousMillis);
    }
}

void ButtonReleaseInProcess() {
    if (IsTimeExpired(previousMillis, BUTTON_TIME_BOUNCE))
    {
        if (digitalRead(BUTTON_ARRAY[buttonOnActiveId])==STATE_IDLE)
        {
            button_current_state = BUTTON_RELEASE;
            buttonIdOutput = buttonOnActiveId;
            buttonStateOutput = BUTTON_PRESS;

            // Serial.print(buttonOnActiveId);
            // Serial.print(" ");
            // Serial.println("Button Released");
        }
        else 
        {
            button_current_state = BUTTON_PRESS;
        }
    }
}
/*--------------------------------------------------------------*/
