/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : app_test_drv.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/8/6
  Last Modified :
  Description   : app_test_drv.c header file
  Function List :
  History       :
  1.Date        : 2017/8/6
    Author      : Enix
    Modification: Created file

******************************************************************************/

#ifndef __APP_TEST_DRV_H__
#define __APP_TEST_DRV_H__


#include "stm32f10x_conf.h"
#include <Rtdef.h>

#ifdef RT_USING_PIN
#include "gpio.h"
#include <drivers/pin.h>
#endif
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/
extern struct    rt_messagequeue  *mq_relay;
/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

typedef enum RELAY_REQ_TYPE
{
    E_REQ_RELAY_STATUS  = 0,
    E_REQ_RELAY_ON      ,
    E_REQ_RELAY_OFF     ,
    E_REQ_RELAY_MAX     ,
}RELAY_REQ_TYPE_ENUM;

typedef struct RELAY_OUTPUT_REQ
{
    RELAY_REQ_TYPE_ENUM                req_type;
    #pragma anon_unions
    union
    {
        rt_uint32_t                     req_val;
        rt_uint8_t                     *ack_ptr;
        rt_uint32_t                    *responese_val;
    };
}RELAY_OUTPUT_REQ_STRU;

typedef struct RELAY_ON_OFF_VAL
{
    rt_uint32_t     on_val;
    rt_uint32_t     off_val;
}RELAY_ON_OFF_VAL_STRU;
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



#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */
    
void service_relay_init(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __APP_TEST_DRV_H__ */
