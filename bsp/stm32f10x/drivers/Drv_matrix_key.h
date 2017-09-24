/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Drv_matrix_key.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/7/12
  Last Modified :
  Description   : Drv_matrix_key.c header file
  Function List :
  History       :
  1.Date        : 2017/7/12
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
typedef struct KEYPIN
{
    GPIO_TypeDef   *port;
    uint16_t        pin;
}KEYPIN_STRU;

typedef struct MATRIX_KEY
{
    KEYPIN_STRU row[3];
    KEYPIN_STRU column[3];
}MATRIX_KEY_STRU;

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
#define             row1_rcc    (RCC_APB2Periph_GPIOC)
#define             row1_port   (GPIOC)
#define             row1_pin    (GPIO_Pin_0)

#define             row2_rcc    (RCC_APB2Periph_GPIOC)
#define             row2_port   (GPIOC)
#define             row2_pin    (GPIO_Pin_1)

#define             row3_rcc    (RCC_APB2Periph_GPIOC)
#define             row3_port   (GPIOC)
#define             row3_pin    (GPIO_Pin_2)

#define             column1_rcc    (RCC_APB2Periph_GPIOC)
#define             column1_port   (GPIOC)
#define             column1_pin    (GPIO_Pin_3)

#define             column2_rcc    (RCC_APB2Periph_GPIOA)
#define             column2_port   (GPIOA)
#define             column2_pin    (GPIO_Pin_0)

#define             column3_rcc    (RCC_APB2Periph_GPIOC)
#define             column3_port   (GPIOC)
#define             column3_pin    (GPIO_Pin_4)
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

#ifndef __DRV_MATRIX_KEY_H__
#define __DRV_MATRIX_KEY_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern rt_uint32_t DrvScanRow(rt_uint8_t);
extern void drv_matrix_key_init(void);
//void test_key_thread_entry(void* parameter);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __DRV_MATRIX_KEY_H__ */
