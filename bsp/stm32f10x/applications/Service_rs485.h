/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Service_rs485.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/9/6
  Last Modified :
  Description   : Service_rs485.c header file
  Function List :
  History       :
  1.Date        : 2017/9/6
    Author      : Enix
    Modification: Created file

******************************************************************************/
#ifndef __SERVICE_RS485_H__
#define __SERVICE_RS485_H__

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
typedef enum SOP_TYPE
{
    E_SOP_NULL,
    E_SOP_PARTICIAL,
    E_SOP_ADDRESS,
}SOP_TYPE_ENUM;

typedef enum EOP_TYPE
{
    E_EOP_NULL,
    E_EOP_PARTICIAL,
    E_EOP_CHECKSUM,
    E_EOP_PARTICIAL_AFTER_CHECKSUM,
    E_EOP_CHECKSUM_AFTER_PARTICIAL,
}EOP_TYPE_ENUM;

typedef enum CHECKSUM_TYPE
{
    E_CHECKSUM_LDS,
    E_CHECKSUM_CRC8,
    E_CHECKSUM_CRC16,
    E_CHECKSUM_CRC32,
    E_CHECKSUM_CUSTOMER,
}CHECKSUM_TYPE_ENUM;

typedef struct msg
{
    rt_uint8_t     *data_ptr; /* 数据块首地址*/
    rt_uint32_t     data_size; /* 数据块大小*/
}msg_t;

typedef struct PROTOCOL_PROPERITY
{
    rt_list_t       node;
    SOP_TYPE_ENUM   header_type;
    rt_uint8_t      header_len;
    EOP_TYPE_ENUM   ender_type;
    rt_uint8_t      ender_len;

    rt_uint8_t      header_param[4];
    rt_uint8_t      ender_param[4];

    CHECKSUM_TYPE_ENUM  checksum_type;
    rt_uint8_t      checksum[4];
    
    rt_uint32_t     (*checksum_calc)(void *p_buffer,rt_uint32_t len);
}PROTOCOL_PROPERITY_STRU;
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

extern void App_Rs485_CMD_Process(void);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SERVICE_RS485_H__ */
