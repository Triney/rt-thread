/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : service_storage.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/12/2
  Last Modified :
  Description   : service_storage.c header file
  Function List :
  History       :
  1.Date        : 2017/12/2
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include <board.h>
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
 enum STORAGE_DEVICE
{
    E_STORAGE_DEVICE_I2C_EEP    = 0,
    E_STORAGE_DEVICE_SPI_FLASH  ,
    E_STORAGE_DEVICE_MAX        ,
};
typedef rt_uint8_t  STORAGE_DEVICE_ENUM;
    
enum STORAGE_REQ_TYPE
{
    E_STORAGE_TYPE_READ     = 0,
    E_STORAGE_TYPE_WRITE    ,
    E_STORAGE_TYPE_MAX      ,
};
typedef rt_uint8_t  STORAGE_REQ_TYPE_ENUM;

enum STORAGE_ACK_TYPE
{
    E_STORAGE_ACK_SUCCESS   = 0,
    E_STORAGE_ACK_FAIL,
};
typedef rt_uint8_t STORAGE_ACK_TYPE_ENUM;

typedef struct READ_WRITE_REQ
{
    STORAGE_REQ_TYPE_ENUM   req_type;
    rt_uint32_t             addr;
    rt_uint8_t             *ptr;
    rt_uint32_t             rw_size;
}READ_WRITE_REQ_STRU;

typedef struct READ_WRITE_ACK
{
    STORAGE_ACK_TYPE_ENUM   ack;
    rt_uint32_t             rw_size;    
}READ_WRITE_ACK_STRU;
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
extern rt_mq_t         service_eep_rw_req;
extern rt_mq_t         service_eep_rw_ack;

extern rt_mq_t         service_flash_rw_req;
extern rt_mq_t         service_flash_rw_ack;
/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/
#define     EE_PAGE_SIZE        16 //24C64
#define     FLASH_PAGE_SIZE     4096
/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

#ifndef __SERVICE_STORAGE_H__
#define __SERVICE_STORAGE_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern void service_eep_rw_req_process(void *parameter);
extern void service_flash_rw_req_process(void *parameter);
rt_err_t service_ee_write_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size);
rt_err_t service_ee_read_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size);
rt_err_t service_flash_write_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size);
rt_err_t service_flash_read_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SERVICE_STORAGE_H__ */
