/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Drv_74hc595.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/7/13
  Last Modified :
  Description   : Drv_74hc595.c header file
  Function List :
  History       :
  1.Date        : 2017/7/13
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include <Rtdef.h>

#include <board.h>
#include <rtthread.h>

#include "stm32f10x_conf.h"


/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
struct pin_index
{
    uint32_t rcc;
    GPIO_TypeDef *gpio;
    uint32_t pin;
};

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
#define             OE1_RCC     RCC_APB2Periph_GPIOB
#define             OE2_RCC     RCC_APB2Periph_GPIOB
#define             SH_CP_RCC   RCC_APB2Periph_GPIOD
#define             ST_CP_RCC   RCC_APB2Periph_GPIOB
#define             DS_RCC      RCC_APB2Periph_GPIOC

#define             OE1_port    GPIOB
#define             OE2_port    GPIOB

#define             OE1_pin     GPIO_Pin_9
#define             OE2_pin     GPIO_Pin_8

#define             SH_CP_PORT  GPIOD
#define             SH_CP_PIN   GPIO_Pin_2

#define             ST_CP_PORT  GPIOB
#define             ST_CP_PIN   GPIO_Pin_3

#define             DS_PORT     GPIOC
#define             DS_PIN      GPIO_Pin_12
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

#ifndef __DRV_74HC595_H__
#define __DRV_74HC595_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

void Drv_74hc595_hw_init(void);
void Drv_74hc595_data_write(rt_uint32_t ulBitVal);
//void test_74hc595_drv_thread_entry(void *parameter);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __DRV_74HC595_H__ */
