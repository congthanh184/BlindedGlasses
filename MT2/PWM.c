#include "stm8s.h"
#include "PWM.h"

#define PWM_Period	((uint16_t) 1024)

// TIM1_SetCompare1 to update duty; TIM1_SetCompare1()
void PWM_UpdateDuty(uint16_t duty){
	TIM1_SetCompare1(duty);
}

void CLK_Config(void){
	CLK_DeInit();

	// Config Fcpu and HSI prescaler
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);

	// Config clock to use 16Mhz HSI
	CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE);
}

void PWM_Init(uint16_t pwm_set_duty){

	//CLK_Config();
	CLK_DeInit();
	CLK_HSICmd(ENABLE);
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);

	/* Config TIM1 */
	TIM1_DeInit();

        // Config TIM1_CH1 as PWM output
	TIM1_TimeBaseInit(0, TIM1_COUNTERMODE_UP, PWM_Period, 0);

	// Config TM1_CH1 as output PWM
	TIM1_OC1Init(TIM1_OCMODE_PWM2,
			TIM1_OUTPUTSTATE_ENABLE,
			TIM1_OUTPUTNSTATE_DISABLE,
			pwm_set_duty,
			TIM1_OCPOLARITY_LOW,
			TIM1_OCNPOLARITY_HIGH,
			TIM1_OCIDLESTATE_SET,
	        TIM1_OCNIDLESTATE_RESET);

	//TIM1_CtrlPWMOutputs(ENABLE);
	//TIM1_CtrlPWMOutputs(DISABLE);

	TIM1_Cmd(ENABLE);

        // Config TM4 as 1ms interrupt
	TIM4_DeInit();

	TIM4_TimeBaseInit(TIM4_PRESCALER_128, 0x7D);
	TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
	TIM4_Cmd(ENABLE);
}

/**
 * @brief	This task will run every 1ms
 */



