#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "PWM.h"
#include "AD.h"
#include "Encoder.h"
#include "OLED.h"
#include "Buzzer.h"


int main(void)
{
	PWM_Init();
	AD_Init();
	Encoder_Init();
	OLED_Init();
	Buzzer_Init();
	
	uint16_t Light;
	int16_t CT;
	
	while (1)
	{		
		CT = Encoder_Get();
		Light = AD_Getvalue() / 4095.0 * 100 ;
		Delay_ms(100);
		
		OLED_ShowNum(1, 1, Light, 3);		
		OLED_ShowSignedNum(2, 1, CT, 4);
		
		int32_t Light_val = (int32_t)Light;
		int32_t CT_val = (int32_t)CT;
		
		if (Light_val > CT_val)
		{
			Buzzer_ON();
			PWM_SetCompare1(0);
			Delay_ms(100);
			PWM_SetCompare1(100);
			Delay_ms(100);
			
		}
		else
		{
			Buzzer_OFF();
			PWM_SetCompare1(Light);
		}

	
	}
	
	
	
	
	
	
	
	
	
	
	
	
}
