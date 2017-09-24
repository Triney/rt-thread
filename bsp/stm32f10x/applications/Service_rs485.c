/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Service_rs485.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/8/27
  Last Modified :
  Description   : RS 485 init, tx thread,rx thread and timeout process 
  Function List :
  History       :
  1.Date        : 2017/8/27
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include <board.h>
#include <rtthread.h>

#ifdef  RT_USING_COMPONENTS_INIT
#include <components.h>
#endif  /* RT_USING_COMPONENTS_INIT */

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
rt_sem_t    sem_rs_485_rcv;
rt_mq_t     mq_rs485_snd;
/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
rt_uint8_t      buffer[64];

rt_list_t       rs_485_protocol_head_node;
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
#define RS485_OE_RCC        RCC_APB2Periph_GPIOA
#define RS485_OE_GPIO       GPIOA
#define RS485_OE_PIN        (GPIO_Pin_1)
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

void RS485_OE_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RS485_OE_RCC,ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = RS485_OE_PIN;
    GPIO_Init(RS485_OE_GPIO, &GPIO_InitStructure);

    GPIO_ResetBits(RS485_OE_GPIO, RS485_OE_PIN);

}

void RS485_Timeout_init(rt_uint16_t usTim1Timerout50us)
{    
    
	rt_uint16_t PrescalerValue = 0;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	//====================================时钟初始化===========================
	//使能定时器3时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	//====================================定时器初始化===========================
	//定时器时间基配置说明
	//HCLK为72MHz，APB1经过2分频为36MHz
	//TIM3的时钟倍频后为72MHz（硬件自动倍频,达到最大）
	//TIM3的分频系数为3599，时间基频率为72 / (1 + Prescaler) = 20KHz,基准为50us
	//TIM最大计数值为usTim1Timerout50u
	
	PrescalerValue = (rt_uint16_t) (SystemCoreClock / 20000) - 1;
	//定时器1初始化
	TIM_TimeBaseStructure.TIM_Period = (rt_uint16_t) usTim1Timerout50us;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	//预装载使能
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	//====================================中断初始化===========================
	//设置NVIC优先级分组为Group2：0-3抢占式优先级，0-3的响应式优先级
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//清除溢出中断标志位
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	//定时器3溢出中断关闭
	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
	//定时器3禁能
	TIM_Cmd(TIM3, DISABLE);
	return ;
}

void RS485TimersEnable()
{
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, ENABLE);
}

void RS485TimersDisable()
{
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
	TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, DISABLE);
}

void TIM3_IRQHandler(void)
{
	rt_interrupt_enter();
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		
		TIM_ClearFlag(TIM3, TIM_FLAG_Update);	     //清中断标记
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);	 //清除定时器T3溢出中断标志位
		rt_sem_release(sem_rs_485_rcv);
	}
	rt_interrupt_leave();
}

/**
 * This function is serial receive callback function
 *
 * @param dev the device of serial
 * @param size the data size that receive
 *
 * @return return RT_EOK
 */
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size) {
    RS485TimersEnable();
    return RT_EOK;
}

void Service_Rs485_Rcv_thread(void *parameter)
{   
    msg_t           msg;

    rt_size_t       size;
    rt_device_t     device;

    device = rt_device_find("uart2");
    
    while ( 1 )
    {
        if ( RT_EOK != rt_sem_take(sem_rs_485_rcv, RT_WAITING_FOREVER))
        {
            continue;
        }

        RS485TimersDisable();

        size = rt_device_read(device, 0, buffer, 64);

        if ( size < 8 )
        {
            continue;
        }

        if ( 0xfa == buffer[0] )
        {
            buffer[0] = 0xfe;
            msg.data_ptr    = buffer;
            msg.data_size   = sizeof(buffer);
            rt_mq_send(mq_rs485_snd,&msg,sizeof(msg_t));
        }
    }
}

void Service_Rs485_Send_thread(void *parameter)
{
    rt_device_t     device;

    msg_t           msg;

    device = rt_device_find("uart2");  

    while(1)
    {
        if (RT_EOK == rt_mq_recv(mq_rs485_snd, &msg, sizeof(msg_t), RT_WAITING_FOREVER))
        {
            GPIO_ResetBits(RS485_OE_GPIO, RS485_OE_PIN);
            rt_device_write(device, 0, msg.data_ptr, msg.data_size);
            GPIO_SetBits(RS485_OE_GPIO, RS485_OE_PIN);
        }
    }
}

void App_Rs485_RegisterStruct_Init(PROTOCOL_PROPERITY_STRU *param)
{    
    memset(param, 0, sizeof(PROTOCOL_PROPERITY_STRU));
}

void App_Rs485_CMD_Register(PROTOCOL_PROPERITY_STRU *param)
{    
    PROTOCOL_PROPERITY_STRU *ptr;

    if (RT_NULL == param );
    {
        return;
    }

    ptr = (PROTOCOL_PROPERITY_STRU *) rt_malloc(sizeof(PROTOCOL_PROPERITY_STRU));

    if ( RT_NULL == ptr )
    {
        return;
    }

    if (RT_TRUE == rt_list_isempty(&rs_485_protocol_head_node))
    {
        rt_list_insert_after(rs_485_protocol_head_node, ptr->node);
    }
    else
    {
        while
    }
}

void App_Rs485_CMD_Process(void)
{
    rt_device_t     rs485;
    rt_thread_t     tid;

    RS485_OE_Init();

    RS485_Timeout_init(40);

    
    sem_rs_485_rcv = rt_sem_create("rs_485_rcv_timeout", 0, RT_IPC_FLAG_FIFO);

    mq_rs485_snd = rt_mq_create("rs485_snd", 16, 5, RT_IPC_FLAG_FIFO);

    rs485 = rt_device_find("uart2");

    if (RT_NULL == rs485 )
    {
        return;
    }

    rt_device_set_rx_indicate(rs485, serial_rx_ind);

    rt_device_open(rs485, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_STREAM | RT_DEVICE_FLAG_INT_RX);

    tid = rt_thread_create("rs485_rcv",
                           Service_Rs485_Rcv_thread, 
                           0, 1024, 25, 5);
    if ( RT_NULL != tid)
    {
        rt_thread_startup(tid);
    }

    tid = rt_thread_create("rs485_send",
                           Service_Rs485_Send_thread, 
                           0, 1024, 24, 5);
    if ( RT_NULL != tid)
    {
        rt_thread_startup(tid);
    }    
    
}



