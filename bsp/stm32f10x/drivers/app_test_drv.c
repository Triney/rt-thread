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
static    rt_mailbox        key_status;
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
void test_key_thread_entry(void* parameter)
{
    rt_uint8_t      i = 0;
    rt_uint32_t     ret;
    rt_uint32_t     temp;

    drv_matrix_key_init();

    key_status = rt_mb_create("key_press", 20, RT_IPC_FLAG_FIFO);

    while(1)
    {
        ret |= DrvScanRow(i);
        i++;
        i %= 3;
        if ( 0 == i )
        {   
            if ( temp != ret )
            {            
                temp = ret;
                rt_mb_send(key_status, ret);
                //rt_kprint("send key msg 0x%x \n",ret );
            }

            ret = 0;
        }
        ret = ret<<3;
        rt_thread_delay(RT_TICK_PER_SECOND/20);
    }
}

void test_74hc595_drv_thread_entry(void *parameter)
{
    rt_err_t        result;
    rt_uint32_t     ret = 0;
    
    Drv_74hc595_hw_init();   
    
    while(1)
    {
        result = rt_mb_recv(key_status, &ret, RT_WAITING_FOREVER);

        if (RT_EOK != result )
        {
            continue;
        }
        //rt_kprint("recv key msg 0x%x \n",ret );
        Drv_74hc595_data_write(ret);            

        rt_thread_delay(RT_TICK_PER_SECOND/5);
        Drv_74hc595_data_write(0xffff); 
    }
}

