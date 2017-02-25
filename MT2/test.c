/**
 ******************************************************************************
 * @file    Digital_Eyes\main.c
 * @author  KBM Team
 * @version  V4.1.0
 * @date     13-October-2014
 * @brief   This file contains the main function for digital eyes.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm8s.h"
#include "ADC.h"
#include "PWM.h"

#include <stdio.h>
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define DEBUG_VERSION
//#define VAR_VERSION

#define	PWM_DUTY_BATTERY 	((uint16_t) 700)
#define	PWM_DUTY_MOTOR 		((uint16_t) 420)

#define RANGE_DETECT_DEFAULT    1
#define RANGE_DETECT_SHORT      0
#define RANGE_DETECT_LONG       2
#define MODE_RANGE_CLK_INTERVAL ((uint16_t)1000)    // ms
// #define RANGE_LOW_LIMIT         ((uint16_t)490)

/* Private variables ------------------------------------------------------------*/

volatile uint16_t hold_time = 0; // hold time is belong tim4 interrupt 1ms, step increase after ++1ms
volatile uint16_t modeCheckClock1ms = 0;
volatile uint16_t adcDistSensorRawData;
volatile uint8_t flagProcessADC=0;

 const uint16_t  RANGE_LIMIT[]       = {148, 118, 103}; // 80, 100, 120 
 const uint16_t  BREAKS_DIST_ADC[]   = {444, 473, 377, 291, 236, 198, 171, 148, 131, 118, 111, 103, 98, 95, 87};  
 const uint8_t   BREAKS_SIZE         = 15;
 const uint16_t  CRITICAL_DIST       = 210;


 static uint16_t idle_time;
 static uint8_t modeRangeIndex = RANGE_DETECT_LONG;

#ifdef _RAISONANCE_
#define PUTCHAR_PROTOTYPE int putchar (char c)
#define GETCHAR_PROTOTYPE int getchar (void)
#elif defined (_COSMIC_)
#define PUTCHAR_PROTOTYPE char putchar (char c)
#define GETCHAR_PROTOTYPE char getchar (void)
#else /* _IAR_ */
#define PUTCHAR_PROTOTYPE int putchar (int c)
#define GETCHAR_PROTOTYPE int getchar (void)
#endif /* _RAISONANCE_ */

/* Private function prototypes -----------------------------------------------*/
 void Delay (uint16_t nCount);
 void Check_Battery_Status(void);
 void Motor_Vibro(uint8_t n);
 void Get_RangeMode(void);
 uint16_t ADC_Process_Value(uint16_t adcDistantValue);
 uint8_t MultiMap(uint16_t val);
/* Main functions ----------------------------------------------------------*/

/**
 * Run in TIM4 interrupt routine, call every 1ms
 */

 void Task_1ms(void)
 {
 	hold_time++;
 	modeCheckClock1ms++;
 	ADC2_StartConversion();
    // Wait for EOC flag SET
 	while (ADC2_GetFlagStatus()==RESET);
//    ADC_Read_Value();
 	adcDistSensorRawData = ADC2_GetConversionValue();
 	flagProcessADC = 1;
 }

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
 void main(void)
 {
 	uint16_t temp;
 	int16_t delta;

    /*----Initialize I/Os in Output Mode----*/
    ///GPIO_Init(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PINS, GPIO_MODE_OUT_PP_LOW_FAST);
    /*----Initialize Modules For Construction----*/
 	PWM_Init(PWM_DUTY_BATTERY);
 	ADC_Init();
    /*----Check Capacity of Battery and warning if needed-----*/
 	Delay(1000);
 	Check_Battery_Status();
 	Delay(3000);
    /*----ReInitialize Modules For Main Function----*/
 	PWM_Init(PWM_DUTY_MOTOR);
 	ADC_Init();

#ifdef DEBUG_VERSION
    // UART3 for debug
 	UART3_DeInit();
 	UART3_Init((uint32_t)115200,
 		UART3_WORDLENGTH_8D,
 		UART3_STOPBITS_1,
 		UART3_PARITY_NO,
 		UART3_MODE_TX_ENABLE);
    // end init UART3
#endif

    /*----Get current mode of range----*/
 	Get_RangeMode();

    /*................................................................................*/
 	while (1)
 	{
 		if (modeCheckClock1ms >= MODE_RANGE_CLK_INTERVAL) {
 			modeCheckClock1ms = 0;
 			Get_RangeMode();
 		}

 		if (flagProcessADC) {
 			flagProcessADC=0;
 			temp = ADC_Process_Value(adcDistSensorRawData);
 			if (temp != 0xFFFF) {
                // check if hold_time is in vibration phase
 				if ((hold_time > idle_time) && (hold_time < (idle_time+80)) && 
 					(idle_time != 0)
 					{
                    // compensate hold_time so vibration time can be preserved
 						delta = (int16_t)(idle_time - temp);
 						hold_time += delta;
#ifdef DEBUG_VERSION
 						printf("new idle_time: %3d, new hold_time: %3d, delta: %d\n\r", temp, hold_time, delta);
#endif
 					}
 					idle_time = temp;
 				}
 			}

 			if (idle_time != 0) {
 				if (hold_time <= idle_time) {
 					TIM1_CtrlPWMOutputs(DISABLE);
 				}
 				else if (hold_time < (idle_time + 80)) {
 					TIM1_CtrlPWMOutputs(ENABLE);
 				}
 				else {
 					hold_time = 0;
 					TIM1_CtrlPWMOutputs(DISABLE);
 				}
 			}
 			else {
 				TIM1_CtrlPWMOutputs(DISABLE);
 			}
 		}
    /*................................................................................*/
 	}

/* Private functions ---------------------------------------------------------*/

/**
 * Check ADC_Channel1 for selecting range mode.
 */
 void Get_RangeMode(void) {
#ifdef VAR_VERSION
 	static uint8_t firstRun = 1;
 	uint16_t adcPotValue = 0;
 	uint8_t mode = 0;

    /* config ADC2 channel 1 for mode selecting */
 	ADC2_ConversionConfig(ADC2_CONVERSIONMODE_SINGLE,
 		ADC2_CHANNEL_1,
 		ADC2_ALIGN_RIGHT);
 	ADC2_StartConversion();
    // Wait for EOC flag SET
 	while (ADC2_GetFlagStatus()==RESET);
 	adcPotValue = ADC2_GetConversionValue();
 	mode = (uint8_t) (adcPotValue/345);

 	if (firstRun) {
 		firstRun = 0;
 		modeRangeIndex = mode;
 	}

 	if (mode != modeRangeIndex) {
 		modeRangeIndex = mode;
 		Motor_Vibro(mode+1);
 	}
    /* return channel 0 for range sensor */
 	ADC2_ConversionConfig(ADC2_CONVERSIONMODE_SINGLE,
 		ADC2_CHANNEL_0,
 		ADC2_ALIGN_RIGHT);
#endif
 }

/**
 * Vibrate motor for notification function
 * @param n
 */
 void Motor_Vibro(uint8_t n)
 {
 	uint8_t i;
 	for(i = 0; i < n; i++)
 	{
 		TIM1_CtrlPWMOutputs(DISABLE);
 		Delay(200);
 		TIM1_CtrlPWMOutputs(ENABLE);
 		Delay(200);
 	}
 	TIM1_CtrlPWMOutputs(DISABLE);
 }

/**
 * @brief Delay
 * @param nCount, hold_time
 * @retval None
 */
 void Delay(uint16_t nCount)
 {
    /* Decrement nCount value */
 	while (hold_time < nCount)
 		{;}
 	if(hold_time >= nCount){
 		hold_time = 0;
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
 	if (val <= BREAKS_DIST_ADC[BREAKS_SIZE-1]) return BREAKS_SIZE;

    // search right interval
    uint8_t pos = 1;  // BREAKS_DIST_ADC[0] allready tested
    while(val < BREAKS_DIST_ADC[pos]) pos++;

    // interpolate in the right segment for the rest
    return pos; 
}

/**
 * Process raw adc distant value
 * @param adcDistantValue
 * @return idle_time
 */
 uint16_t ADC_Process_Value(uint16_t adcDistantValue) {
 	static uint8_t count = 0;
 	float res;

 	if ((adcDistantValue >= RANGE_LIMIT[modeRangeIndex]) && (adcDistantValue <= BREAKS_DIST_ADC[0])) {
 		count++;
 		if(count < 130) {
 			return 0xFFFF;
 		}
 		else{
 			count=0;
 			res = MultiMap(adcDistantValue);

#ifdef DEBUG_VERSION
            // printf("adc: %4d, id: %d, res: %d\n\r", adcDistantValue, idRange, (uint16_t)res);
 			printf("adc: %4d, res: %d\n\r", adcDistantValue, (uint16_t)res);
#endif
 			return ((uint16_t)res*50);
 		}
 	}
 	else {
 		count = 0;
 		return 0;
 	}
 }

/**
 * ADC Read Value Function----> This task will update data every 1ms
 */
/*void ADC_Read_Value(void)
{
    uint16_t ADC_Val = 0;
    float res;
    ADC_Val = ADC2_GetConversionValue();
    if(count < 150){
        if((ADC_Val > set)&&(ADC_Val < 550)){
            count++;
//            goto abc;
        }
        else{
            count=0;
            idle_time=0;
//            goto abc;
        }
    }
    else{
        count=0;
        if((ADC_Val >= set) && (ADC_Val < 380)){
            res = 1200 - (3 * ADC_Val);
            idle_time = (uint16_t) res;
        }
        else{
            res = 540 - (4/5) * ADC_Val;
            idle_time = 100;
        }
    }
    //---------------------------------------
//    abc:
//    if((ADC_Val < set)||(ADC_Val > 550)){
//        idle_time = 0;
//        //return;
//    }
}*/

/**
 * @brief Check the battery status when start up and vibrate for notification
 */
 void Check_Battery_Status(void)
 {
 	uint16_t dbat=0;
 	ADC2_ConversionConfig(ADC2_CONVERSIONMODE_SINGLE,
 		ADC2_CHANNEL_12,
 		ADC2_ALIGN_RIGHT);
 	ADC2_StartConversion();
 	while (ADC2_GetFlagStatus()==RESET);
 	dbat = ADC2_GetConversionValue();

    // Deconfig for Distance Sensor Signal
 	ADC2_ConversionConfig(ADC2_CONVERSIONMODE_SINGLE,
 		ADC2_CHANNEL_0,
 		ADC2_ALIGN_RIGHT);

 	if(dbat > 750){
 		Motor_Vibro(3);
 	}
 	else if((dbat > 700)&&(dbat <= 750)){
 		Motor_Vibro(2);
 	}
 	else{
 		Motor_Vibro(1);
 	}
 }

#ifdef DEBUG_VERSION
/**
 * @brief Retargets the C library printf function to the UART.
 * @param c Character to send
 * @retval char Character sent
 */
 PUTCHAR_PROTOTYPE
 {
    /* Write a character to the UART1 */
 	UART3_SendData8(c);
    /* Loop until the end of transmission */
 	while (UART3_GetFlagStatus(UART3_FLAG_TXE) == RESET);

 	return (c);
 }
#endif

/////////////////////////////////NOT USE////////////////////////////////////////
#ifdef USE_FULL_ASSERT

 void assert_failed(uint8_t* file, uint32_t line)
 { 

 	while (1)
 	{
 	}
 }
#endif

/**
 * @}
 */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
