/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Drv_74hc595.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/7/13
  Last Modified :
  Description   : driver for 74hc595
  Function List :
  History       :
  1.Date        : 2017/7/13
    Author      : Enix
    Modification: Created file

******************************************************************************/

#include "stm32f10x_conf.h"
#include "Drv_74hc595.h"

#include <board.h>
#include <rtthread.h>
#include <Rtdef.h>
#include "delay.h"
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
static void delay(int cnt)
{
    volatile unsigned int dl;
    while(cnt--)
    {
        for(dl=0; dl<500; dl++);
    }
}

void Drv_74hc595_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(OE1_RCC | OE2_RCC | SH_CP_RCC | ST_CP_RCC | DS_RCC, 
                           ENABLE);

    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_10MHz;

    GPIO_InitStructure.GPIO_Pin     = OE1_pin;
    GPIO_Init(OE1_port, &GPIO_InitStructure);
    GPIO_ResetBits(OE1_port, OE1_pin);

    GPIO_InitStructure.GPIO_Pin     = OE2_pin;
    GPIO_Init(OE2_port, &GPIO_InitStructure);  
    GPIO_ResetBits(OE2_port, OE2_pin);

    GPIO_InitStructure.GPIO_Pin     = SH_CP_PIN;
    GPIO_Init(SH_CP_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(SH_CP_PORT, SH_CP_PIN);

    GPIO_InitStructure.GPIO_Pin     = ST_CP_PIN;
    GPIO_Init(ST_CP_PORT, &GPIO_InitStructure);    
    GPIO_ResetBits(ST_CP_PORT, ST_CP_PIN);

    GPIO_InitStructure.GPIO_Pin     = DS_PIN;
    GPIO_Init(DS_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(DS_PORT, DS_PIN);
}

void drv_74hc595_shift(rt_uint8_t BitVal)
{    
    rt_uint8_t      i;

    for(i = 0 ;i < 8 ;i++)
    {
        if (0 != (BitVal & (1<<i)))   
        {
            GPIO_SetBits(DS_PORT, DS_PIN);
        }
        else
        {
            GPIO_ResetBits(DS_PORT, DS_PIN);
        }
        delay_us(1);
        GPIO_SetBits(SH_CP_PORT, SH_CP_PIN);
        delay_us(10);
        GPIO_ResetBits(SH_CP_PORT, SH_CP_PIN);
        delay_us(10);
    }
}

void Drv_74hc595_data_write(rt_uint32_t ulBitVal)
{   
    rt_uint8_t      val;

    GPIO_SetBits(OE1_port,OE1_pin);
    GPIO_SetBits(OE2_port,OE2_pin);

    delay_us(20);
    
    GPIO_ResetBits(ST_CP_PORT, ST_CP_PIN);

    val = (rt_uint8_t)ulBitVal;
    drv_74hc595_shift(val);
    #if 1
    GPIO_ResetBits(ST_CP_PORT, ST_CP_PIN);
    delay_us(20);
    GPIO_SetBits(ST_CP_PORT, ST_CP_PIN);    
    delay_us(20);
    #endif

    val = (rt_uint8_t)(ulBitVal >> 8);
    drv_74hc595_shift(val);    
    
    GPIO_SetBits(ST_CP_PORT, ST_CP_PIN);    
    delay_us(20);

    GPIO_ResetBits(SH_CP_PORT, SH_CP_PIN);
    GPIO_ResetBits(ST_CP_PORT, ST_CP_PIN);
    GPIO_ResetBits(DS_PORT, DS_PIN);
    
    GPIO_ResetBits(OE1_port,OE1_pin);
    GPIO_ResetBits(OE2_port,OE2_pin);    
}


