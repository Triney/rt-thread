/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : service_key.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/10/30
  Last Modified :
  Description   : service_key.c header file
  Function List :
  History       :
  1.Date        : 2017/10/30
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include <rtthread.h>
#include <rtdevice.h>
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
typedef void (*pKeyFunction)(void *parameter);

typedef enum KEY_ACTION_TYPE
{
    E_KEY_PRESS     = 0,
    E_KEY_RELEASE   ,
    E_KEY_LONG_PRESS,
    E_KEY_LONG_PRESS_RELEAE,
    E_KEY_ACT_MAX,
}KEY_ACTION_TYPE_ENUM;

typedef struct KEY_EVENT_FUNCTION
{
    rt_list_t               node;
    rt_uint32_t             event;
    pKeyFunction            pFunc;
    void                   *parameter;
}KEY_EVENT_FUNCTION_STRU;

typedef struct KEY_ACTION_EVT
{
    KEY_ACTION_TYPE_ENUM    key_msg_type;
    rt_uint32_t             key_value;
}KEY_ACTION_EVT_STRU;
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/


typedef enum SM_KEY
{
    SM_KEY_PRESSED_NONE             = 0,
    SM_KEY_LPRESS_WAIT_CONFIRM      ,
    SM_KEY_LPRESS_CONFIRMED         ,
    SM_KEY_LPRESS_WAIT_RELEASE      ,
    SM_KEY_CONBINATION              ,
    SM_KEY_CONBINATION_RELEASED_SOME,
    SM_KEY_CONBINATION_RELEASED_ALL ,
    SM_KEY_STATUS_MAX               ,
}SM_KEY_ENUM;
/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
//#define KEY_USE_EVT 
#define KEY_NUMBERS     16

#define KEY_SCAN_TICK_PER_SECOND        20

#define SERVICE_KEY     (1<<0)
#define CH1_KEY         (1<<1)
#define CH2_KEY         (1<<2)
#define CH3_KEY         (1<<3)
#define CH4_KEY         (1<<4)
#define CH5_KEY         (1<<5)
#define CH6_KEY         (1<<6)
#define CH7_KEY         (1<<7)
#define CH8_KEY         (1<<8)

#define AUX_INPUT       (1<<13)
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

#ifndef __SERVICE_KEY_H__
#define __SERVICE_KEY_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern void service_key_process(void *parameter);
extern void service_key_scan_thread_entry(void* parameter);
extern void service_key_fucntion_register(rt_uint32_t          event,
                                          KEY_ACTION_TYPE_ENUM   key_evt_type,
                                         pKeyFunction           p_key_func,
                                         void                  *parameter);
extern void service_key_function_unregister(rt_uint32_t          event,
                                          KEY_ACTION_TYPE_ENUM   key_evt_type);
extern void service_key_evt_func_execute(rt_list_t *evt_list_head,
                                          rt_uint32_t   event);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SERVICE_KEY_H__ */
