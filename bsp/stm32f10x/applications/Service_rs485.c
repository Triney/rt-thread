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

#include "Service_Rs485.h"
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

struct rt_ringbuffer    send_buffer_rb;
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

static rt_bool_t protocol_choose(
    void                    *rcv_frame, 
    rt_uint8_t               rcv_len,
    PROTOCOL_PROPERITY_STRU *protocol_info)
{   

    rt_uint8_t  *receive_from = (rt_uint8_t *)rcv_frame;
    
    if ( rcv_len < protocol_info->min_len)
    {
        return RT_FALSE;
    }

    if (E_SOP_PARTICIAL == protocol_info->header_type )
    {
        if(0 !=memcmp(receive_from,
                            &(protocol_info->header_param[0]),
                            protocol_info->header_len))
        {
            return RT_FALSE;
        }
    }

    if ( E_EOP_PARTICIAL == protocol_info->ender_type )
    {
        if(0 !=memcmp(receive_from + rcv_len - protocol_info->ender_len,
                            &(protocol_info->ender_param[0]),
                            protocol_info->ender_len))
        {
            return RT_FALSE;
        }
    }

    if (RT_NULL != protocol_info->checksum_calc)
    {
        rt_uint32_t checksum_calc;
        rt_uint32_t checksum_received;

        //checksum_received = 
        memcpy(&checksum_received,
                receive_from + rcv_len - protocol_info->ender_len - protocol_info->checksum_len,
                protocol_info->checksum_len);

        checksum_calc = protocol_info->checksum_calc(rcv_frame,rcv_len - protocol_info->checksum_len);

        if ( checksum_received !=  checksum_calc)
        {
            return RT_FALSE;
        }
        else
        {
            return RT_TRUE;
        }
        
    }
    else
    {
        return RT_FALSE;
    }

}

void Service_Rs485_Rcv_thread(void *parameter)
{   
//    msg_t           msg;

    rt_size_t       size;
    rt_device_t     device;
    
    trace("%ds %dms thread rs485 rcv start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  
    
    device = rt_device_find("uart2");
    
    while ( 1 )
    {
    
        if ( RT_EOK != rt_sem_take(sem_rs_485_rcv, RT_WAITING_FOREVER))
        {
            continue;
        }

        RS485TimersDisable();

        size = rt_device_read(device, 0, buffer, 64);

        {
            rt_list_t   *list_node;

            PROTOCOL_PROPERITY_STRU *protocol_node;

            for (list_node = rs_485_protocol_head_node.next; list_node != &rs_485_protocol_head_node; list_node = protocol_node->node.next)
            {
                protocol_node = (PROTOCOL_PROPERITY_STRU *)(rt_list_entry(list_node, PROTOCOL_PROPERITY_STRU, node));

                if ( RT_FALSE == protocol_choose(buffer,size,protocol_node) )
                {
                    continue;
                }
                else
                {
                    rt_size_t ack_size;
                    
                    protocol_node->get_parameter(buffer,protocol_node->parameter);

                    ack_size= protocol_node->process_handler(protocol_node->parameter,protocol_node->ack);
                    
                    if (0 != ack_size)
                    {   
                        rt_uint8_t      *buff;
                        rt_uint32_t     checksum;
                        msg_t   msg;
                        checksum = protocol_node->checksum_calc((rt_uint8_t *)(protocol_node->ack),ack_size);
                        
                        buff = (rt_uint8_t *)((rt_uint8_t *)(protocol_node->ack) + ack_size);
                        buff[0] = (checksum &0xff);
                        buff[1] = ((checksum>>8) &0xff);
                        buff[2] = ((checksum>>16) &0xff);
                        buff[3] = ((checksum>>24) &0xff);
                        rt_ringbuffer_put(&send_buffer_rb, protocol_node->ack, ack_size + protocol_node->checksum_len);

                        msg.data_ptr = &send_buffer_rb;
                        msg.data_size = ack_size + protocol_node->checksum_len;
                        rt_mq_send(mq_rs485_snd, &msg, sizeof(msg_t));
                    }
                    break;
                }
            }



        } 
        #if 0
        if ( 0xfa == buffer[0] )
        {
            buffer[0] = 0xfe;
            msg.data_ptr    = buffer;
            msg.data_size   = sizeof(buffer);
            rt_mq_send(mq_rs485_snd,&msg,sizeof(msg_t));
        }
        #endif
    }
}

void Service_Rs485_Send_thread(void *parameter)
{
    rt_device_t     device;

    msg_t           msg;
    
    trace("%ds %dms thread rs485 send start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  
    
    device = rt_device_find("uart2");  

    while(1)
    {
        if (RT_EOK == rt_mq_recv(mq_rs485_snd, &msg, sizeof(msg_t), RT_WAITING_FOREVER))
        {
            rt_uint8_t  *ptr;
            //rt_size_t   size;
            

            ptr = rt_malloc(msg.data_size);

            //size = 
            rt_exit_critical();
            rt_ringbuffer_get(msg.data_ptr, ptr, msg.data_size);
            GPIO_ResetBits(RS485_OE_GPIO, RS485_OE_PIN);
            rt_device_write(device, 0, ptr, msg.data_size);
            GPIO_SetBits(RS485_OE_GPIO, RS485_OE_PIN);
            rt_exit_critical();
            rt_free(ptr);
            
        }
    }
}
#if 1
void Service_Rs485_RegisterStruct_Init(PROTOCOL_PROPERITY_STRU *param)
{    
    memset(param, 0, sizeof(PROTOCOL_PROPERITY_STRU));
}

void Service_Rs485_CMD_Register(PROTOCOL_PROPERITY_STRU *param)
{    
    PROTOCOL_PROPERITY_STRU *ptr;

    if (RT_NULL == param )
    {
        return;
    }

    ptr = (PROTOCOL_PROPERITY_STRU *) rt_malloc(sizeof(PROTOCOL_PROPERITY_STRU));

    if ( RT_NULL == ptr )
    {
        return;
    }

    memcpy(ptr,param,sizeof(PROTOCOL_PROPERITY_STRU));

    if (RT_TRUE == rt_list_isempty(&rs_485_protocol_head_node))
    {
        rt_list_insert_after(&rs_485_protocol_head_node, &(ptr->node));
    }
    else
    {
        rt_list_t   *protocol_node;

        PROTOCOL_PROPERITY_STRU *node_end;

        for (protocol_node = rs_485_protocol_head_node.next; protocol_node != &rs_485_protocol_head_node; protocol_node = protocol_node->next)
        {
            node_end = (PROTOCOL_PROPERITY_STRU *)(rt_list_entry(protocol_node, PROTOCOL_PROPERITY_STRU, node));
        }

        rt_list_insert_after(&(node_end->node), &(ptr->node));
    } 
}
#endif
void App_Rs485_CMD_Process(void)
{
    rt_uint8_t     *mem_ptr;

    rt_device_t     rs485;
    rt_thread_t     tid;

    RS485_OE_Init();

    RS485_Timeout_init(40);

    
    sem_rs_485_rcv = rt_sem_create("rs_485_rcv_timeout", 0, RT_IPC_FLAG_FIFO);

    mq_rs485_snd = rt_mq_create("rs485_snd", 16, 5, RT_IPC_FLAG_FIFO);

    mem_ptr = rt_malloc(1024);
    rt_ringbuffer_init( &send_buffer_rb, mem_ptr, 1024);

    rt_list_init(&rs_485_protocol_head_node);

    rs485 = rt_device_find("uart2");

    if (RT_NULL == rs485 )
    {
        return;
    }

    rt_device_set_rx_indicate(rs485, serial_rx_ind);

    rt_device_open(rs485, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX );

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



