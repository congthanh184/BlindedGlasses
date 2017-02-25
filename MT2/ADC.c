#include "stm8s.h"
#include "adc.h"

/**
 * @brief	Init ADC Module
 *
 */
void ADC_Init(void){

// Config GPIO for ADC0: PB0
	/*  Init GPIO for ADC2 */
	GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_IN_FL_NO_IT);
	GPIO_Init(GPIOF, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);
// Config ADC

	/* De-Init ADC peripheral*/
	ADC2_DeInit();

	/* Init ADC2 peripheral */
	ADC2_Init(ADC2_CONVERSIONMODE_SINGLE,	// ADC2_CONVERSIONMODE_CONTINUOUS
			ADC2_CHANNEL_0,
			ADC2_PRESSEL_FCPU_D2,
			ADC2_EXTTRIG_TIM,
			DISABLE, ADC2_ALIGN_RIGHT,
			ADC2_SCHMITTTRIG_CHANNEL0,
			DISABLE);

	/* Enable EOC interrupt */
	//ADC2_ITConfig(ENABLE);

	/* Enable general interrupts */
	enableInterrupts();

	/*Start Conversion */
	ADC2_StartConversion();
}
