#include "alert_func.h"

static const uint8_t 		MOTOR_DISABLE 				= 0;
static const uint8_t 		MOTOR_ENABLE					= 104; 		// 420 in [0..1023]
static const uint8_t 		NUMBER_SAMPLE_FILTER	= 80;
// static const uint16_t 	BREAKS_DIST_ADC[]   	= {512,409,307,254,223,184,168,153,143,123,113,102,92,82};
// static const uint16_t 	BREAKS_DIST_ADC[]   	= {2752,1565,1180,860,721,680,623,582,557};	
static const uint16_t 	BREAKS_DIST_ADC[]   	= {5504, 3130, 2360, 1720, 1442, 700};	
static const uint8_t 		BREAKS_SIZE         	= sizeof(BREAKS_DIST_ADC)/2;
static const float 			ALPHA 								= 0.98;
static const float 			COMP_ALPHA						= 0.02;

static const uint16_t DELAY_FOR_TEST 	= 0;
static const int 			MOTOR_PWM 			= BLUE_LED;
static const int 			SENSOR_SHARP1		= A0;			// Right
static const int 			SENSOR_SHARP2		= A1;			// Left

enum {
	MOTOR_VIB_RUN = 1, 
	MOTOR_VIB_REST = 0,
	MOTOR_STOP = 2
};

static uint8_t MultiMap(uint16_t);
static uint16_t ADC_Process_Value(uint16_t, uint8_t*);
static void MotorControl_Vibro(uint16_t);
static void TIM1_CtrlPWMOutputs(uint8_t);
static void InformVoice(char*);
/*--------------------------------------------------------------*/
void AlertSystemOpen() 
{
	// countSampleSensor1 = 0;
	// countSampleSensor2 = 0;
	pinMode(MOTOR_PWM, OUTPUT);
	MotorControl_Vibro(MOTOR_DISABLE);
}

void AlertSystemPoll() 
{
	static uint8_t countSampleSensor1 = 0, countSampleSensor2 = 0;
	static uint16_t idleTime1 = 0, idleTime2 = 0;
	static uint16_t filterADCValue1 = 0, filterADCValue2 = 0;

	uint16_t temp1, temp2;
	uint16_t rawADCValue1, rawADCValue2;
	uint16_t tempUART1, tempUART2;

	// if (Serial.available()>0)	
	// {
	// 	tempUART1 = Serial.parseInt(); 
	// 	tempUART2 = Serial.parseInt(); 
	// 	if (Serial.read()=='\n')
	// 	{
	// 		// Serial.println(tempUART1);
	// 		// Serial.println(tempUART2);
	// 		rawADCValue1 = tempUART1;
	// 		rawADCValue2 = tempUART2;
	// 		temp1 =	ADC_Process_Value(rawADCValue1, &countSampleSensor1);
	// 		temp2 =	ADC_Process_Value(rawADCValue2, &countSampleSensor2);
	// 		// Serial.print(countSampleSensor1); Serial.print("   ");
	// 		// Serial.println(countSampleSensor2);
	// 		// 0xFFFF : ADC signal is in detect range but not sure yet
	// 		if (temp1 != 0xFFFF && idleTime1 != temp1)
	// 		{
	// 			idleTime1 = temp1;
	// 		}
	// 		if (temp2 != 0xFFFF && idleTime2 != temp2)
	// 		{
	// 			idleTime2 = temp2;
	// 		}
	// 		Serial.print(idleTime1); Serial.print("   ");
	// 		Serial.println(idleTime2);
	// 	}
	// }
	
	rawADCValue1 = analogRead(SENSOR_SHARP1);
	rawADCValue2 = analogRead(SENSOR_SHARP2);

	filterADCValue1 = ALPHA * ((float)filterADCValue1) + COMP_ALPHA * ((float)rawADCValue1);
	filterADCValue2 = ALPHA * ((float)filterADCValue2) + COMP_ALPHA * ((float)rawADCValue2);

	// temp1 =	ADC_Process_Value(rawADCValue1, &countSampleSensor1);
	// temp2 =	ADC_Process_Value(rawADCValue2, &countSampleSensor2);

	temp1 =	ADC_Process_Value(filterADCValue1, &countSampleSensor1);
	temp2 =	ADC_Process_Value(filterADCValue2, &countSampleSensor2);

			// 0xFFFF : ADC signal is in detect range but not sure yet
	if (temp1 != 0xFFFF && idleTime1 != temp1)
	{
		idleTime1 = temp1;
	}
	if (temp2 != 0xFFFF && idleTime2 != temp2)
	{
		idleTime2 = temp2;
	}

	// 2 la LEFT, 1 la RIGHT
	// if (((idleTime1 > idleTime2) && (idleTime2 != 0)) || (idleTime1==0)
	// {
	// 	MotorControl_Vibro(idleTime2);
	// 	InformVoice((char*)"vL");
	// 	// Serial.print("Run idleTime2 = ");
	// 	// Serial.println(idleTime2);
	// }
	// else
	// {
	// 	MotorControl_Vibro(idleTime1);
	// 	if (idleTime1!=0)
	// 	{
	// 		InformVoice((char*)"vR");
	// 	}
	// 	// Serial.print("Run idleTime1 = ");				
	// 	// Serial.println(idleTime1);
	// }
	if (idleTime1==0 && idleTime2==0)
	{
		MotorControl_Vibro(idleTime2);
	}
	else if (idleTime1>=idleTime2)
	{
		if (idleTime2!=0)
		{	
			MotorControl_Vibro(idleTime2);
			InformVoice((char*)"Vt");
		}
		else {
			MotorControl_Vibro(idleTime1);
			InformVoice((char*)"Vr");			
		}
	}
	else if (idleTime2>idleTime1)
	{
		if (idleTime1!=0)
		{
			MotorControl_Vibro(idleTime1);
			InformVoice((char*)"Vr");
		}
		else {
			MotorControl_Vibro(idleTime2);
			InformVoice((char*)"Vt");			
		}	
	}
}
/*--------------------------------------------------------------*/
void InformVoice(char* dir)
{
	static uint32_t voiceHoldTimeMillis = 0;
	if (IsTimeExpired(voiceHoldTimeMillis, 3000)) // 2s for a remind
	{
		if (ArdCommWrite(VOICE_DIRECTION, dir)==1)
		{
			StartCountTime(&voiceHoldTimeMillis);
		}
	}
}

void StartCountTime(uint32_t *previousMillis) 
{
	*previousMillis = millis();
}

uint8_t IsTimeExpired(uint32_t previousMillis, uint16_t waitTime) 
{
	uint32_t currentMillis = millis();
	if (currentMillis - previousMillis >= waitTime)
	{
		return 1;
	}
	return 0;
}


void TIM1_CtrlPWMOutputs(uint8_t pwm) 
{
	// Serial.println(pwm);
	analogWrite(MOTOR_PWM, pwm);
}

void MotorControl_Vibro(uint16_t idleTime)
{
	static uint8_t motorState = MOTOR_STOP;
	static uint32_t motorHoldTimeMillis = 0;

	switch (motorState) 
	{
		case MOTOR_STOP:
		// TIM1_CtrlPWMOutputs(MOTOR_DISABLE);
		if (idleTime != 0) {
			motorState = MOTOR_VIB_RUN;
			TIM1_CtrlPWMOutputs(MOTOR_ENABLE);
			// Serial.println("MOTOR RUN");
			StartCountTime(&motorHoldTimeMillis);
		}
		break;

		case MOTOR_VIB_RUN:
        // TIM1_CtrlPWMOutputs(MOTOR_ENABLE);
		if (IsTimeExpired(motorHoldTimeMillis, 80 + DELAY_FOR_TEST)) {
			motorState = MOTOR_VIB_REST;            
			TIM1_CtrlPWMOutputs(MOTOR_DISABLE);
			// Serial.println("MOTOR REST");
			StartCountTime(&motorHoldTimeMillis);
		}
		break;    

		case MOTOR_VIB_REST:
		TIM1_CtrlPWMOutputs(MOTOR_DISABLE);
		if (idleTime == 0) {
			motorState = MOTOR_STOP;
			TIM1_CtrlPWMOutputs(MOTOR_DISABLE);
			// Serial.println("MOTOR STOP");
		}
		else if (IsTimeExpired(motorHoldTimeMillis, idleTime + DELAY_FOR_TEST)) {
			motorState = MOTOR_VIB_RUN;
			TIM1_CtrlPWMOutputs(MOTOR_ENABLE);
			// Serial.println("MOTOR RUN");
			StartCountTime(&motorHoldTimeMillis);
		}
		break;

		default:
		motorState = MOTOR_STOP;
		TIM1_CtrlPWMOutputs(MOTOR_DISABLE);
		break;
	}
}

/**
 * Find the position equivalent to ADC value; BREAKS_DIST_ADC is descending
 * @param  val: adc value
 * @return position
 */
 uint8_t MultiMap(uint16_t val)
 {
    // take care the value is within range
    // val = constrain(val, _in[0], _in[size-1]);
    // BREAKS_DIST_ADC descending
 	if (val >= BREAKS_DIST_ADC[0]) return 1;
 	if (val <= BREAKS_DIST_ADC[BREAKS_SIZE-1]) return (BREAKS_SIZE-1);

    // search right interval
    uint8_t pos = 1;  // BREAKS_DIST_ADC[0] allready tested
    while(val < BREAKS_DIST_ADC[pos]) pos++;

    // interpolate in the right segment for the rest
    return pos; 
  }

/**
 * Process raw adc distant value
 * @param adcDistantValue
 * @return idleTime
 */
 uint16_t ADC_Process_Value(uint16_t adcDistantValue, uint8_t* count) 
 {
 	float res;

 	if (adcDistantValue >= BREAKS_DIST_ADC[BREAKS_SIZE-1]) 
 	{
 		(*count)++;

 		if (*count < NUMBER_SAMPLE_FILTER) {
            // 0xFFFF : ADC signal is in detect range but not sure yet
 			return 0xFFFF;
 		}
 		else{
 			*count=0;
 			res = MultiMap(adcDistantValue);
 			return ((uint16_t)res*50);
 		}
 	}
 	else {
 		*count = 0;
 		return 0;
 	}
 }
/*--------------------------------------------------------------*/
