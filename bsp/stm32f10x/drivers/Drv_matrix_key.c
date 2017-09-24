/******************************************************************************
  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Drv_matrix_key.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/7/9
  Last Modified :
  Description   : matrix key scan proc
  Function List :
  History       :
  1.Date        : 2017/7/9
    Author      : Enix
    Modification: Created file

******************************************************************************/

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/
#include "stm32f10x_conf.h"
#include "Drv_matrix_key.h"

#include <board.h>
#include <rtthread.h>
#include <Rtdef.h>

#ifdef RT_USING_PIN
#include "gpio.h"
#include <drivers/pin.h>
#endif
/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
 MATRIX_KEY_STRU    GPIO_Key =
 {
     {
         {row1_port    ,row1_pin},
         {row2_port    ,row2_pin},
         {row3_port    ,row3_pin},
     },
     {
        {column1_port   , column1_pin},
        {column2_port   , column2_pin},
        {column3_port   , column3_pin},
     }
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
static void delay(int cnt)
{
    volatile unsigned int dl;
    while(cnt--)
    {
        for(dl=0; dl<500; dl++);
    }
}

void drv_matrix_key_init(void)
{
//#ifdef RT_USING_PIN
#if 0
    rt_device_t     device;
    device = rt_device_find("pin");
    rt_pin_mode(rt_base_t pin, PIN_MODE_OUTPUT);

#else
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(row1_rcc | row2_rcc | row3_rcc
                           | column1_rcc | column2_rcc|column3_rcc, 
                           ENABLE);

    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin          = row1_pin;
    GPIO_Init(row1_port,&GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin          = row2_pin;
    GPIO_Init(row2_port,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin          = row3_pin;
    GPIO_Init(row3_port,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin          = column1_pin;
    GPIO_Init(column1_port,&GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin          = column2_pin;
    GPIO_Init(column2_port,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin          = column3_pin;
    GPIO_Init(column3_port,&GPIO_InitStructure);    

    GPIO_SetBits(row1_port, row1_pin);
    GPIO_SetBits(row2_port, row2_pin);    
    GPIO_SetBits(row3_port, row3_pin);  
#endif
}

rt_uint32_t DrvScanRow(rt_uint8_t   RowScan)
{
    rt_uint8_t      i;
    rt_uint32_t     ret;
    //assert(RowScan<3);

    ret = 0;

    for(i = 0; i < 3; i++)
    {
        if ( i ==  RowScan)
        {
            GPIO_ResetBits(GPIO_Key.row[i].port, GPIO_Key.row[i].pin);
        }
        else
        {
            GPIO_SetBits(GPIO_Key.row[i].port, GPIO_Key.row[i].pin);
        }
    }

    for(i = 0; i < 3 ;i++)
    {
        ret |= (GPIO_ReadInputDataBit(GPIO_Key.column[i].port,GPIO_Key.column[i].pin)<<i);
    }

    return ret;
}



