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
static    struct rt_mailbox       *key_status;
struct    rt_mailbox       *mb_relay_acton;

rt_event_t          evtKeyPress;
rt_event_t          evtKeyRelease;
rt_event_t          evtKeyLongPress;
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
void test_key_scan_thread_entry(void* parameter)
{
    rt_uint8_t      i = 0;
    rt_uint32_t     ret;
    rt_uint32_t     last_read_key = 0;
    rt_uint32_t     cur_key = 0;

    rt_uint32_t     press = 0;
    rt_uint32_t     release = 0;
    rt_uint32_t     count = 0; 

    drv_matrix_key_init();

//    key_status = rt_mb_create("key_press", 128, RT_IPC_FLAG_FIFO);

    evtKeyPress = rt_event_create("key_press",RT_IPC_FLAG_FIFO);
    evtKeyRelease = rt_event_create("key_Release",RT_IPC_FLAG_FIFO);
    evtKeyLongPress = rt_event_create("key_LPress",RT_IPC_FLAG_FIFO);

    while(1)
    {
        ret = DrvScanRow(2);
        ret <<= 3;
        ret |= DrvScanRow(1);
        ret <<= 3; 
        ret |= DrvScanRow(0);

        ret = ret^0x1ff;
        #if 0
        press = ret & (ret ^count);
        release = ret ^ (press ^ count);
        count = ret;
        #else
        cur_key = (ret&last_read_key)|count &( ret^last_read_key);

        press = cur_key & (cur_key ^ count);
        release = cur_key ^(press ^ count);

        last_read_key = ret;
        count = cur_key;
        
        #endif

        if (0 != press)
        {
            rt_event_send(evtKeyPress, press);
        }
        
        if (0 != release)
        {
            rt_event_send(evtKeyRelease, release);
        }
        
        if ((0 == press)&&(0 != count))
        {    
            rt_event_send(evtKeyLongPress, count);
        }
        rt_thread_delay(RT_TICK_PER_SECOND/10);
    }
}

typedef struct msg
{
    rt_uint8_t     *data_ptr; /* ??????*/
    rt_uint32_t     data_size; /* ?????*/
}msg_t;

rt_uint8_t g_buffer[64];
void test_74hc595_drv_thread_entry(void *parameter)
{
    rt_err_t        result;
    rt_uint32_t     ret = 0;
    rt_uint32_t     i;
    rt_uint32_t     val;
    
    msg_t           msg;
    extern rt_mq_t     mq_rs485_snd;
    
    Drv_74hc595_hw_init(); 
    Drv_74hc595_data_write(0xffff);     

    mb_relay_acton = rt_mb_create("Relay", 200, RT_IPC_FLAG_FIFO);
    
    while(1)
    {
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
                rt_kprintf("%d.%dms receive mb : channel %d : %d \n",rt_tick_get()/200,
                                            (rt_tick_get()%200)*5,channel,action);
            }

        }          
        rt_thread_delay(RT_TICK_PER_SECOND/10);
        Drv_74hc595_data_write(0xffff); 
    }
}

void test_key_process(void *parameter)
{
    rt_uint8_t      buff[8];
    rt_uint32_t     recved;
    while(1)
    {
        if (RT_EOK == rt_event_recv(evtKeyPress, 0x1ff, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 2, &recved))
        {
            switch ( recved )
            {
                case 1 :
                    buff[0] = 0xfa;
                    rt_ringbuffer_put(&send_buffer_rb,buff , 8);
                    msg.data_ptr = &send_buffer_rb;
                    msg.data_size = 8;
                    rt_mq_send(mq_rs485_snd, &msg, sizeof(msg_t));
                    break;
            }
        }
    }    
}


