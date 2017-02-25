#ifndef _PWM_MODULE_H_
#define _PWM_MODULE_H_

/* Macros */

#define Motor_GPIO_Port		GPIOC

#define Motor1_Pin		GPIO_PIN_1
#define Motor2_Pin		GPIO_PIN_2
#define Motor3_Pin		GPIO_PIN_3
#define Motor4_Pin		GPIO_PIN_4

// Functions
//void PWM_Init(void);
void PWM_Init(uint16_t pwm_set_duty);
#endif
