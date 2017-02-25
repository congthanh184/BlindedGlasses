#include "Metro.h"
#include "button.h"
#include "alert_func.h"
#include "arduino_communication.h"
#include <Wire.h>
/*--------------------------------------------------------------*/
int led=PB_3;
uint8_t led_state = 0;

// Metro BlinkLed = Metro(250);
Metro ScanButton = Metro(10);
Metro RunAlertSystem = Metro(1);

void ProcessUserInput();
/*--------------------------------------------------------------*/
void setup()
{
  // put your setup code here, to run once:
  // Serial.begin(115200);
  ButtonOpen();
  AlertSystemOpen();
  ArdCommOpen();
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(100);
  digitalWrite(led, HIGH);
}

void loop()
{
  if (RunAlertSystem.check())
  {
    ProcessUserInput();
    AlertSystemPoll();
  }

  if (ScanButton.check())
  {
    ButtonPoll();
  }

  // if (BlinkLed.check())   
  // {
    // led_state = led_state^1;
    // digitalWrite(led, led_state);
  // }
}
/*--------------------------------------------------------------*/
void ProcessUserInput() {
  uint8_t buttonId;
  Button_State_Type buttonState;
  uint8_t sendOK;

  ButtonRead( &buttonId, &buttonState);
  sendOK = 0;
  if (buttonState==BUTTON_PRESS)    
  {
    if (buttonId==BUTTON_TIME)
    {
      sendOK = ArdCommWriteTime();
    }
    else if (buttonId==BUTTON_VOL)
    {
      sendOK = ArdCommWriteVol();
    }
    if (sendOK)
    {
      ButtonClear();
    }
  }
}
/*--------------------------------------------------------------*/

