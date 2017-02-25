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
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

//#define DEBUG_VERSION
//#define VAR_VERSION

#define PWM_DUTY_BATTERY    ((uint16_t) 700)
#define PWM_DUTY_MOTOR      ((uint16_t) 420)

#define RANGE_DETECT_DEFAULT    1
#define RANGE_DETECT_SHORT      0
#define RANGE_DETECT_LONG       2
#define MODE_RANGE_CLK_INTERVAL ((uint16_t)1000)    // ms

#define COUNT_VALID_SAMPLE      37
#define COUNT_STABLE_SAMPLE     15
#define ADC_READ_PERIOD         4                   // ms

const uint16_t  RANGE_LIMIT[]       = {148, 118, 113}; /* 80, 100, 120 */
 const uint16_t  BREAKS_DIST_ADC[]   = {512,409,307,254,223,184,168,153,143,123,113,102,92,82};
 const uint8_t   BREAKS_SIZE         = sizeof(BREAKS_DIST_ADC)/2;
 const int16_t   SENSITIVE_DELTA[]   = {20, 20, 10, 6, 7, 3, 3, 2, 4, 2, 2, 2, 2};


enum {
    MOTOR_VIB_RUN = 1, 
    MOTOR_VIB_REST = 0,
    MOTOR_STOP = 2
};


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

/* Private variables ---------------------------------------------------------*/
//uint16_t set = 230; //gia tri cang giam thi khoang cach cang xa

 static uint16_t idle_time;
 static uint8_t modeRangeIndex = RANGE_DETECT_LONG;

 volatile uint8_t flagProcessADC=0;
volatile uint16_t hold_time = 0; // hold time is belong tim4 interrupt 1ms, step increase after ++1ms
volatile uint16_t modeCheckClock1ms = 0;
volatile uint16_t adcDistSensorRawData;
volatile uint8_t  adcReadClock1ms = 0;
/* Private function prototypes -----------------------------------------------*/
void Delay (uint16_t nCount);
void Check_Battery_Status(void);
void Motor_Vibro(uint8_t n);
void Get_RangeMode(void);
uint16_t ADC_Process_Value(uint16_t adcDistantValue);
uint8_t MultiMap(uint16_t val);
void MotorControl_Vibro(uint16_t idleTime); // idleTime in ms
/* Main functions ----------------------------------------------------------*/

/**
 * Run in TIM4 interrupt routine, call every 1ms
 */

 void Task_1ms(void)
 {
    hold_time++;
    modeCheckClock1ms++;
    adcReadClock1ms++;
    if (adcReadClock1ms>=ADC_READ_PERIOD)
    {
        ADC2_StartConversion();
        // Wait for EOC flag SET
        while (ADC2_GetFlagStatus()==RESET);
        adcDistSensorRawData = ADC2_GetConversionValue();
        flagProcessADC = 1;
        adcReadClock1ms = 0;
    }
 }

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
 void main(void)
 {
    uint16_t temp;

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
            // 0xFFFF : ADC signal is in detect range but not sure yet
            if (temp != 0xFFFF && idle_time != temp) {
                idle_time = temp;
            }
        }
        MotorControl_Vibro(idle_time);
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
*/
 void MotorControl_Vibro(uint16_t idleTime)
 {
    static uint8_t motorState = MOTOR_STOP;
    
    switch (motorState) 
    {
        case MOTOR_STOP:
        TIM1_CtrlPWMOutputs(DISABLE);
        if (idleTime != 0) {
            motorState = MOTOR_VIB_RUN;
            hold_time = 0;
        }
        break;
        case MOTOR_VIB_RUN:
        TIM1_CtrlPWMOutputs(ENABLE);
        if (hold_time > 80) {
            motorState = MOTOR_VIB_REST;            
            hold_time = 0;
        }
        break;    
        case MOTOR_VIB_REST:
        TIM1_CtrlPWMOutputs(DISABLE);
        if (idleTime == 0) {
            motorState = MOTOR_STOP;
        }
        else if (hold_time > idleTime) {
            motorState = MOTOR_VIB_RUN;
            hold_time = 0;
        }
        break;
        default:
        motorState = MOTOR_STOP;
        TIM1_CtrlPWMOutputs(DISABLE);
        break;
    }
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
 *         return 0 if out of bound. No top bound.
 */
 uint8_t MultiMap(uint16_t val)
 {
    static int8_t lastPos = 0;

    // take care the value is within range
    // val = constrain(val, _in[0], _in[size-1]);
    // BREAKS_DIST_ADC descending
    if (val <= BREAKS_DIST_ADC[BREAKS_SIZE-1]) 
    {
        lastPos = 0;
        return 0;
    }

    // search right interval
    uint8_t pos = 1;  // BREAKS_DIST_ADC[0] allready tested
    while(val < BREAKS_DIST_ADC[pos]) pos++;

    int delta_left, delta_right;
    delta_right = abs((int16_t)val - BREAKS_DIST_ADC[lastPos]);
    delta_left = abs((int16_t)val - BREAKS_DIST_ADC[lastPos-1]);

    if (delta_left <= SENSITIVE_DELTA[lastPos] || 
        delta_right <= SENSITIVE_DELTA[lastPos])
    {
        pos = lastPos;
    }
    lastPos = pos;
    // interpolate in the right segment for the rest
    return pos; 
}

/**
 * Process raw adc distant value
 * @param adcDistantValue
 * @return idle_time
 */
 uint16_t ADC_Process_Value(uint16_t adcDistantValue) {
    static uint8_t countStableVal = 0;
    static uint8_t countValidVal = 0;
    static uint8_t lastDistLevel;
    uint8_t dist_level = MultiMap(adcDistantValue);

    if (dist_level > 0)
    {
        countValidVal++;
        if (countValidVal > COUNT_VALID_SAMPLE)
        {
            countValidVal = 0;
            countStableVal = 0;
            return (uint16_t)dist_level * 40;
        }

        if (abs((int8_t)lastDistLevel - dist_level)>=13)
        {
            lastDistLevel = dist_level;
            countStableVal = 1;
            return 0xFFFF;
        }
        else if (countStableVal < COUNT_STABLE_SAMPLE)
        {
            countStableVal++;
            return 0xFFFF;
        }
        else {
            countValidVal = 0;
            countStableVal = 0;
            return (uint16_t)dist_level * 40;
        }
    }
    else if (dist_level == 0)
    {
        countStableVal = 0;
        countValidVal = 0;
        lastDistLevel = 0;
    }
    return 0;
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
