#include "stm32f10x.h"                  // Device header

int16_t Encoder_Count;

void Encoder_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);  // 外部中断必须开AFIO时钟
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         // 上拉输入（先确认传感器为NPN型）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 配置EXTI线
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);
    
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;    // 中断模式（非事件模式）
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;// 触发方式（NPN传感器匹配）
    EXTI_Init(&EXTI_InitStructure);
    
    // NVIC配置（优先级分组2，抢占1，子优先级1，无问题）
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;   
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
	
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;   
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&NVIC_InitStructure);
	
}
int16_t Encoder_Get(void)
{
	int16_t Temp;
	Temp = Encoder_Count;
	Encoder_Count = 0;
	return Temp;
}

void EXTI0_IRQHandler(void)
{
	if (EXTI_GetFlagStatus(EXTI_Line0) == SET)
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 1)
		{
			Encoder_Count ++;
		}
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

void EXTI1_IRQHandler(void)
{
	if (EXTI_GetFlagStatus(EXTI_Line1) == SET)
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 1)
		{
			Encoder_Count --;
		}
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}










