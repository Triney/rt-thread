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
#include "app_test_drv.h"
#include "Drv_74hc595.h"
#include "Drv_matrix_key.h"
#include "service_rs485.h"
#include <board.h>
#include <rtthread.h>
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
struct    rt_messagequeue  *mq_relay;

struct    rt_semaphore     *sem_relay;

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
rt_uint32_t         g_relay_state;
rt_uint32_t         g_relay_need_act;

RELAY_ON_OFF_VAL_STRU   on_off_value[12]=
{
    {0x1        ,0x2        },
    {0x4        ,0x8        },    
    {0x10       ,0x20       },
    {0x40       ,0x80       },
    {0x100      ,0x200      },
    {0x400      ,0x800      },    
    {0x1000     ,0x2000     },
    {0x4000     ,0x8000     },  
    {0x10000    ,0x20000    },
    {0x40000    ,0x80000    },    
    {0x100000   ,0x200000   },
    {0x400000   ,0x8000000  },    
};    
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


rt_uint8_t g_buffer[64];
#if 0
void test_74hc595_drv_thread_entry(void *parameter)
{
    rt_err_t        result;
    rt_uint32_t     ret = 0;
    rt_uint32_t     i;
    rt_uint32_t     val;
    
    msg_t           msg;
    extern rt_mq_t     mq_rs485_snd;

    trace("%ds %dms thread 595 output start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  
    
    Drv_74hc595_hw_init(); 
    Drv_74hc595_data_write(0xffff);     

    mb_relay_acton = rt_mb_create("Relay", 200, RT_IPC_FLAG_FIFO);
    
    while(1)
    {
        trace("%ds %dms excute thread 74hc595 \n",rt_tick_get()/200,
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
                trace("%ds %dms receive mb : channel %d : %d \n",rt_tick_get()/200,
                                            (rt_tick_get()%200)*5,channel,action);
            }

        }          
        rt_thread_delay(RT_TICK_PER_SECOND/10);
        Drv_74hc595_data_write(0xffff); 
    }
}
#else
void service_relay_ctrl(void *parameter)
{
    rt_uint8_t      i;
    rt_uint32_t     relay_val;

    trace("%ds %dms thread relay_ctrl start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  

    sem_relay = rt_sem_create("relay_ctrl", 0, RT_IPC_FLAG_FIFO);
    while(1)
    {    
        if ( RT_EOK == rt_sem_take(sem_relay, RT_WAITING_FOREVER))
        {
            for ( i = 0 ; i <12 ; i++ )
            {
                if((g_relay_need_act & (1ul<<i)) ==(1ul<<i))
                {   
                    g_relay_need_act &= ~(1<<i);
                    if ((g_relay_state & (1ul<<i)) ==(1ul<<i))
                    {
                        relay_val = ~on_off_value[i].on_val;
                    }
                    else
                    {
                        relay_val = ~on_off_value[i].off_val;
                    }

                    Drv_74hc595_data_write(relay_val); 
                    rt_thread_delay(RT_TICK_PER_SECOND/10);
                    Drv_74hc595_data_write(0xffff); 
                    break;
                }
            }
        }
    }
}

void service_relay_req_process(void *parameter)
{
    rt_err_t        result;
    rt_uint32_t     i;
    rt_uint32_t     val;

    RELAY_OUTPUT_REQ_STRU       req;
    
    msg_t           msg;
    extern rt_mq_t     mq_rs485_snd;

    trace("%ds %dms thread relay req process start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  
    
    Drv_74hc595_hw_init(); 
    Drv_74hc595_data_write(0xffff);     

    mq_relay = rt_mq_create("Relay", 
                            sizeof(RELAY_OUTPUT_REQ_STRU),
                            200,
                            RT_IPC_FLAG_FIFO);

    while(1)
    {
        result = rt_mq_recv(mq_relay, 
                            &req,
                            sizeof(RELAY_OUTPUT_REQ_STRU),
                            RT_WAITING_FOREVER);

        if (RT_EOK == result )
        {
            switch ( req.req_type)
            {
                case E_REQ_RELAY_STATUS:
                    
                    break;
                case E_REQ_RELAY_ON:
                    g_relay_state   |= (1<<req.req_val);
                    g_relay_need_act|= (1<<req.req_val);
                    rt_sem_release(sem_relay);
                    break;
                case E_REQ_RELAY_OFF:
                    g_relay_state   &= ~(1<<req.req_val);
                    g_relay_need_act|= (1<<req.req_val);
                    rt_sem_release(sem_relay);                    
                    break;
                default:
                    break;
            }
        
        }
    }
}

void service_relay_init(void)
{   
    rt_thread_t     init_thread;

    init_thread = rt_thread_create("rel_pro",
                                   service_relay_req_process, RT_NULL,
                                   512, 31, 5);
    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);    

    init_thread = rt_thread_create("rel_ctrl",
                                   service_relay_ctrl, RT_NULL,
                                   256, 10, 5);
    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);  
}
#endif

