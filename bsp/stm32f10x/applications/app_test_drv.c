/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : app_test_drv.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/8/5
  Last Modified :
  Description   : for test drv
  Function List :
  History       :
  1.Date        : 2017/8/5
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include "stm32f10x_conf.h"
#include "Drv_74hc595.h"
#include "Drv_matrix_key.h"
#include "service_rs485.h"

#include <board.h>
#include <rtthread.h>
#include <Rtdef.h>

#ifdef RT_USING_PIN
#include "gpio.h"
#include <drivers/pin.h>
#endif
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
struct    rt_mailbox       *mb_relay_acton;

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/\


rt_uint8_t g_buffer[64];
void test_74hc595_drv_thread_entry(void *parameter)
{
    rt_err_t        result;
    rt_uint32_t     ret = 0;
    rt_uint32_t     i;
    rt_uint32_t     val;
    
    msg_t           msg;
    extern rt_mq_t     mq_rs485_snd;

    trace("%d.%ds thread 595 output start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  
    
    Drv_74hc595_hw_init(); 
    Drv_74hc595_data_write(0xffff);     

    mb_relay_acton = rt_mb_create("Relay", 200, RT_IPC_FLAG_FIFO);
    
    while(1)
    {
        trace("%d.%dms excute thread 74hc595 \n",rt_tick_get()/200,
                                    (rt_tick_get()%200)*5);
        result = rt_mb_recv(mb_relay_acton, &ret, RT_WAITING_FOREVER);

        if (RT_EOK == result )
        {   
            rt_uint8_t  channel;
            rt_uint8_t  action;

            rt_uint16_t  relay_val = 0;

            channel = (rt_uint8_t) (ret&0x0f);
            action  = (rt_uint8_t) ((ret>>4)&0x0f);

            if ( RT_TRUE == action  )
            {
                relay_val = 1;
            }
            else
            {
                relay_val = 2;
            }

            if  ( channel < 12 )
            {
            
                relay_val = relay_val<<(channel * 2);
                relay_val = ~relay_val;
            
                Drv_74hc595_data_write(relay_val);                  
                trace("%d.%dms receive mb : channel %d : %d \n",rt_tick_get()/200,
                                            (rt_tick_get()%200)*5,channel,action);
            }

        }          
        rt_thread_delay(RT_TICK_PER_SECOND/10);
        Drv_74hc595_data_write(0xffff); 
    }
}


