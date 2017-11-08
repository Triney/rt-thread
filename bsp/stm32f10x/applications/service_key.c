/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : service_key.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/10/30
  Last Modified :
  Description   : key scan and key fucntion register
  Function List :
              test_key_scan_thread_entry
  History       :
  1.Date        : 2017/10/30
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include "stm32f10x_conf.h"
#include "Drv_74hc595.h"
#include "Drv_matrix_key.h"
//#include "service_rs485.h"
#include "service_key.h"

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
rt_event_t          evtKeyPress;
rt_event_t          evtKeyRelease;
rt_event_t          evtKeyLongPress;
rt_event_t          evtKeyLongPressRelease;

rt_list_t           g_key_press_evt_func_head_node;
rt_list_t           g_key_release_evt_func_head_node;
rt_list_t           g_key_long_press_evt_func_head_node;
rt_list_t           g_key_long_press_release_evt_func_head_node;
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
 *----------------------------------------------*/
static rt_uint8_t bitcount(rt_uint32_t x)  
{  
    rt_uint32_t b;  
   
    for (b = 0; x != 0; x &= (x-1))  
        b++;  
    return b;  
}   

static rt_bool_t service_key_combination_find(rt_uint32_t event)
{                                          
    rt_list_t                 *evt_head_node;
    rt_list_t                 *key_evt_node;
    KEY_EVENT_FUNCTION_STRU   *cur_searching_node;

    if ( bitcount(event) < 2 )
    {
        return RT_FALSE;
    }

    evt_head_node = &g_key_press_evt_func_head_node;

    for ( key_evt_node = evt_head_node->next; 
          key_evt_node != evt_head_node; 
          key_evt_node = key_evt_node->next)
    {
        cur_searching_node = (KEY_EVENT_FUNCTION_STRU *)(rt_list_entry(key_evt_node, KEY_EVENT_FUNCTION_STRU, node));
        if ( cur_searching_node->event == event)
        {
            return RT_TRUE;
        }
    }    
    return RT_FALSE;
}    

void service_key_scan_thread_entry(void* parameter)
{
    rt_uint32_t     ret;
    rt_uint32_t     last_read_key = 0;
    rt_uint32_t     cur_key = 0;

    rt_uint32_t     press = 0;
    rt_uint32_t     release = 0;
    rt_uint32_t     count = 0; 

    rt_uint32_t     long_press_time;

    rt_uint32_t     last_comb_key = 0;
    
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
            if ( bitcount(release | count) < 2 )
            {
                if ( last_comb_key == 0 )
                {
                    if ( long_press_time >=20 )
                    {
                        rt_event_send(evtKeyLongPressRelease, release);
                    }
                    else
                    {
                        rt_event_send(evtKeyRelease, release);
                    }
                    long_press_time = 0;                    
                }
                else
                {
                    last_comb_key &= (~release);
                }

            }
            else if(RT_TRUE== service_key_combination_find(release | count))
            {
                rt_event_send(evtKeyRelease, (release | count));
                last_comb_key = count;
            }
        }
        
        if ((0 == (press | release)) 
            &&(0 != count))
        {   
            rt_uint8_t bitcounts;
            bitcounts = bitcount(count);
            if (1 == bitcounts)     //only one key holds on press
            {
                if(long_press_time < (KEY_SCAN_TICK_PER_SECOND * 2))
                {    
                    long_press_time++;
                }
                else if (long_press_time == (KEY_SCAN_TICK_PER_SECOND * 2))
                {
                    rt_event_send( evtKeyLongPress, count);
                    long_press_time ++;
                }
            }
            else
            {
                if (RT_TRUE == service_key_combination_find(count))
                {
                    if (0 == last_comb_key )
                    {
                        rt_event_send( evtKeyPress, count);
                        last_comb_key = count;
                    }
                    
                }
                long_press_time = 0;
            }            
        }
        rt_thread_delay(RT_TICK_PER_SECOND/KEY_SCAN_TICK_PER_SECOND);
    }
}


void service_key_process(void *parameter)
{
    rt_uint32_t     recved;

    while(1)
    {
        if (RT_EOK == rt_event_recv(evtKeyPress, 0x1ff, 
                                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 1, &recved))
        {
            service_key_evt_func_execute(&g_key_press_evt_func_head_node,
                                          recved);
        }

        if ( RT_EOK == rt_event_recv(evtKeyRelease, 0x1ff, 
                                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 1, &recved ))
        {
            service_key_evt_func_execute(&g_key_release_evt_func_head_node,
                                          recved);
        }

        if ( RT_EOK == rt_event_recv(evtKeyLongPress, 0x1ff, 
                                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 1, &recved ))
        {
            service_key_evt_func_execute(&g_key_long_press_evt_func_head_node,
                                          recved);
        }
        
        if ( RT_EOK == rt_event_recv(evtKeyLongPressRelease, 0x1ff, 
                                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 1, &recved ))
        {
            service_key_evt_func_execute(&g_key_long_press_release_evt_func_head_node,
                                          recved);
        }        
    }    
}

void service_key_fucntion_register(rt_uint32_t          event,
                                          KEY_ACTION_TYPE_ENUM   key_evt_type,
                                         pKeyFunction           p_key_func,
                                         void *parameter)
{   
    KEY_EVENT_FUNCTION_STRU             *ptr;
    rt_list_t       *evt_head_node;
    
    ptr = rt_malloc(sizeof(KEY_EVENT_FUNCTION_STRU));
    if (RT_NULL == ptr )
    {
        return;
    }

    ptr->event = event;
    ptr->pFunc = p_key_func;
    ptr->parameter = parameter;

    if ( E_KEY_PRESS == key_evt_type )
    {
        evt_head_node = &g_key_press_evt_func_head_node;
    }
    else if(E_KEY_RELEASE == key_evt_type)
    {
        evt_head_node = &g_key_release_evt_func_head_node;
    }
    else if(E_KEY_LONG_PRESS== key_evt_type)
    {
        evt_head_node = &g_key_long_press_evt_func_head_node;
    }
    else if(E_KEY_LONG_PRESS_RELEAE== key_evt_type)
    {
        evt_head_node = &g_key_long_press_release_evt_func_head_node;
    }
    else
    {
        return;
    }

    if (RT_TRUE == rt_list_isempty(evt_head_node))
    {
        rt_list_insert_after(evt_head_node, &(ptr->node));
    }
    else
    {
        rt_list_t                 *key_evt_node;
        KEY_EVENT_FUNCTION_STRU   *cur_searching_node;

        for ( key_evt_node = evt_head_node->next; 
              key_evt_node != evt_head_node; 
              key_evt_node = key_evt_node->next)
        {
            cur_searching_node = (KEY_EVENT_FUNCTION_STRU *)(rt_list_entry(key_evt_node, KEY_EVENT_FUNCTION_STRU, node));
            if ( cur_searching_node->event == event)
            {
                cur_searching_node->pFunc = p_key_func;
                cur_searching_node->parameter = parameter;
                rt_free(ptr);
                return;
            }
        }

        rt_list_insert_before(evt_head_node, &(ptr->node));
        
    }
    return;
}

void service_key_function_unregister(rt_uint32_t          event,
                                          KEY_ACTION_TYPE_ENUM   key_evt_type)
{                                          
    rt_list_t       *evt_head_node;
    rt_list_t                 *key_evt_node;
    KEY_EVENT_FUNCTION_STRU   *cur_searching_node;

    for ( key_evt_node = evt_head_node->next; 
          key_evt_node != evt_head_node; 
          key_evt_node = key_evt_node->next)
    {
        cur_searching_node = (KEY_EVENT_FUNCTION_STRU *)(rt_list_entry(key_evt_node, KEY_EVENT_FUNCTION_STRU, node));
        if ( cur_searching_node->event == event)
        {
            rt_list_remove(&(cur_searching_node->node));
            break;
        }
    }    
    return;
}

void service_key_evt_func_execute(rt_list_t *evt_list_head,
                                          rt_uint32_t   event)
{    
    rt_list_t                 *key_evt_node;
    KEY_EVENT_FUNCTION_STRU   *cur_searching_node;

    for ( key_evt_node = evt_list_head->next; 
          key_evt_node != evt_list_head; 
          key_evt_node = key_evt_node->next)
    {
        cur_searching_node = (KEY_EVENT_FUNCTION_STRU *)(rt_list_entry(key_evt_node, KEY_EVENT_FUNCTION_STRU, node));
        if ( cur_searching_node->event == event)
        {
            if (RT_NULL != cur_searching_node->pFunc )
            {
                cur_searching_node->pFunc(cur_searching_node->parameter);
            }
            
            return;
        }
    }    
}

void service_key_start(void)
{
    rt_thread_t     init_thread;
    
    drv_matrix_key_init();

    rt_list_init(&g_key_press_evt_func_head_node);
    rt_list_init(&g_key_release_evt_func_head_node);
    rt_list_init(&g_key_long_press_evt_func_head_node);
    rt_list_init(&g_key_long_press_release_evt_func_head_node);
    
    evtKeyPress = rt_event_create("key_press",RT_IPC_FLAG_FIFO);
    evtKeyRelease = rt_event_create("key_Release",RT_IPC_FLAG_FIFO);
    evtKeyLongPress = rt_event_create("key_LPress",RT_IPC_FLAG_FIFO);
    evtKeyLongPressRelease = rt_event_create("key_LRelease",RT_IPC_FLAG_FIFO);
    
    #if 1
    init_thread = rt_thread_create("scan_key",
                                   service_key_scan_thread_entry, RT_NULL,
                                   256, 29, 5);
    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);
    #endif
    
    #if 1
    init_thread = rt_thread_create("key_service",
                                   service_key_process, RT_NULL,
                                   256, 30, 5);
    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);
    #endif
}

