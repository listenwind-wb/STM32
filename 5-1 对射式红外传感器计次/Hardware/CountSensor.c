#include "stm32f10x.h"                  // Device header
#include "Delay.h"

uint16_t CountSensor_Count;


void CountSensor_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);  // 外部中断必须开AFIO时钟
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         // 上拉输入（先确认传感器为NPN型）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 配置EXTI线：GPIOB14对应EXTI_Line14
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);
    
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;    // 中断模式（非事件模式）
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;// 双边沿触发
    EXTI_Init(&EXTI_InitStructure);
    
    // NVIC配置（优先级分组2，抢占1，子优先级1，无问题）
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;   // B14属于EXTI15_10中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
}

uint16_t CountSensor_Get(void) {
    return CountSensor_Count;
}

// 外部中断15_10服务函数（处理B14中断）
void EXTI15_10_IRQHandler(void) {
    // 1. 先判断是否是B14的中断（避免其他线干扰）
    if (EXTI_GetITStatus(EXTI_Line14) == SET) {
        // 2. 短延时消抖（关键：过滤引脚抖动，10ms足够，无需400ms）
        Delay_ms(10);
        // 3. 二次确认引脚电平
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0 || GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1 ) {
            CountSensor_Count++;  // 仅真触发时计数
        }
        // 4. 清除中断标志（必须做，否则会重复进入中断）
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
}












