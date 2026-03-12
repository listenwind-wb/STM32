#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "PWM.h"
#include "AD.h"
#include "Encoder.h"
#include "OLED.h"



int main(void)
{
	PWM_Init();
	AD_Init();
	Encoder_Init();
	OLED_Init();
	
	uint16_t Light;
	int16_t Count;
	
	Count = Encoder_Get();


	while (1)
	{		
		Count = Encoder_Get();
		Light = AD_Getvalue() / 4095.0 * 100 ;
		Delay_ms(100);
		OLED_ShowNum(1, 1, Light, 3);
		PWM_SetCompare1(Light);
		OLED_ShowSignedNum(2, 1, Count, 4);
	
	}
	
	
	
	
	
	
	
	
	
	
	
	
}
