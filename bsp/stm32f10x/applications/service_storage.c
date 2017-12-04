/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : service_storage.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/12/2
  Last Modified :
  Description   : process read write storage reqs
  Function List :
              service_eep_rw_req_process
              service_flash_rw_req_process
  History       :
  1.Date        : 2017/12/2
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include "service_storage.h"
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
rt_mq_t         service_eep_rw_req;
rt_mq_t         service_eep_rw_ack;

rt_mq_t         service_flash_rw_req;
rt_mq_t         service_flash_rw_ack;

rt_mutex_t      eep_mutex;
rt_mutex_t      flash_mutex;
/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
static rt_uint8_t      flash_buffer[FLASH_PAGE_SIZE];
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


void service_eep_rw_req_process(void *parameter)
{
    static rt_device_t  eep_devices;
    READ_WRITE_REQ_STRU rw_msg;
    READ_WRITE_ACK_STRU ack_msg;

    eep_devices = rt_device_find("EEP");

    if (RT_NULL != eep_devices )
    {
        rt_device_open(eep_devices, RT_DEVICE_OFLAG_RDWR);
    }
    else
    {
        return;
    }
    
    trace("%ds %dms thread eep rw start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5);  
    while ( 1 )
    {
        if (RT_EOK != rt_mq_recv(service_eep_rw_req, &rw_msg, sizeof(READ_WRITE_REQ_STRU), RT_WAITING_FOREVER))
        {
            continue;
        }
        
        if ( (0 == rw_msg.rw_size)
            ||(RT_NULL == rw_msg.ptr))
        {
            continue;
        }
            
        if (E_STORAGE_TYPE_READ  == rw_msg.req_type)
        {
            ack_msg.rw_size = rt_device_read(eep_devices,
                                            rw_msg.addr,rw_msg.ptr, rw_msg.rw_size);
            if ( ack_msg.rw_size == rw_msg.rw_size )
            {    
                ack_msg.ack = E_STORAGE_ACK_SUCCESS;
            }
            else
            {
                ack_msg.ack = E_STORAGE_ACK_FAIL;
            }
        }
        else if (E_STORAGE_TYPE_WRITE == rw_msg.req_type)
        {
            rt_uint8_t     *ptr;
            rt_uint32_t     addr;
            rt_uint32_t     size;
            rt_uint32_t     rc,i;
            rt_uint32_t     b_to_wr;

            ptr = rw_msg.ptr;
            addr= rw_msg.addr;
            size= rw_msg.rw_size;

            #if 0
            trace("write addr = 0x%x , content :", addr,size);
            for ( i = 0 ; i < size ; i++ )
            {
                trace(" 0x%x" ,*(ptr + i));
            }
            trace("\n");
            #endif
            rc = addr % EE_PAGE_SIZE;

            
            if ( 0 != rc )
            {
                b_to_wr = EE_PAGE_SIZE - rc;
                if ( size < b_to_wr )
                {
                    b_to_wr = size;
                }
                
                if ( b_to_wr > 0)
                {
                    rc = rt_device_write(eep_devices,
                                    addr, ptr, b_to_wr);                        
                }
                
                if ( 0 != rc )
                {
                    rt_thread_delay(RT_TICK_PER_SECOND / 100);
                    size -= b_to_wr;
                    addr += b_to_wr;
                    ptr  += b_to_wr;                        
                }
                else
                {
                    size = 0;
                }

            }

            while ( size > 0 )
            {
                if ( size < EE_PAGE_SIZE)
                {
                    b_to_wr = size;
                }
                else
                {
                    size = EE_PAGE_SIZE;
                }
                rc = rt_device_write(eep_devices,
                                    addr, ptr, b_to_wr);
                if ( 0 != rc )
                {
                    rt_thread_delay(RT_TICK_PER_SECOND / 100);
                    size -= b_to_wr;
                    addr += b_to_wr;
                    ptr  += b_to_wr;                        
                }
                else
                {
                    size = 0;
                }
            }

            if ( 0 != rc )
            {
                ack_msg.ack = E_STORAGE_ACK_SUCCESS;
            }
            else
            {
                ack_msg.ack = E_STORAGE_ACK_FAIL;
            }
            
        }
        if ( rw_msg.req_type < E_STORAGE_TYPE_MAX)
        {
            rt_mq_send(service_eep_rw_ack, &ack_msg, sizeof(READ_WRITE_ACK_STRU));
        }

 
    }
}

void service_flash_rw_req_process(void *parameter)
{
    rt_device_t         flash_devices;
    READ_WRITE_REQ_STRU rw_msg;
    READ_WRITE_ACK_STRU ack_msg;

    flash_devices = rt_device_find("Flash0");

    if (RT_NULL != flash_devices )
    {
        rt_device_open(flash_devices, RT_DEVICE_OFLAG_RDWR);
    }
    else
    {
        return;
    }
    
    trace("%ds %dms thread flash rw start \n",rt_tick_get()/200,
                            (rt_tick_get()%200)*5); 
    while ( 1 )
    {
        if (RT_EOK != rt_mq_recv(service_flash_rw_req, &rw_msg, sizeof(READ_WRITE_REQ_STRU), RT_WAITING_FOREVER))
        {
            continue;
        }
        
        if ( (0 == rw_msg.rw_size)
            ||(RT_NULL == rw_msg.ptr))
        {
            continue;
        }  
        
        if (E_STORAGE_TYPE_READ  == rw_msg.req_type)
        {
            ack_msg.rw_size = rt_device_read(flash_devices,
                                            rw_msg.addr,rw_msg.ptr, rw_msg.rw_size);
            if ( ack_msg.rw_size != rw_msg.rw_size )
            {    
                ack_msg.ack = E_STORAGE_ACK_SUCCESS;
            }
            else
            {
                ack_msg.ack = E_STORAGE_ACK_FAIL;
            }
        }
        else if (E_STORAGE_TYPE_WRITE == rw_msg.req_type)
        {
            rt_uint8_t     *ptr;
            rt_uint32_t     addr;
            rt_uint32_t     page_index;
            rt_uint32_t     size;
            rt_uint32_t     rc;
            rt_uint32_t     b_to_wr;

            ptr = rw_msg.ptr;
            addr= rw_msg.addr;
            page_index = addr / FLASH_PAGE_SIZE;
            size= rw_msg.rw_size;
            
            rc = addr % FLASH_PAGE_SIZE; 
            
            if ( 0 != rc )
            {
                b_to_wr = FLASH_PAGE_SIZE - rc;
                if ( size < b_to_wr )
                {
                    b_to_wr = size;
                }
                
                if ( b_to_wr > 0)
                {
                    rc = rt_device_read(flash_devices, page_index, flash_buffer, FLASH_PAGE_SIZE);
                    memcpy(flash_buffer + rc,ptr,b_to_wr);
                    rc = rt_device_write(flash_devices,
                                    addr, flash_buffer, 1);                        
                }
                
                if ( 0 != rc )
                {
                    size -= b_to_wr;
                    addr += b_to_wr;
                    ptr  += b_to_wr;                        
                }
                else
                {
                    size = 0;
                }

            }

            while ( size > 0 )
            {
                if ( size < EE_PAGE_SIZE)
                {
                    b_to_wr = size;
                }
                else
                {
                    size = EE_PAGE_SIZE;
                }
                
                rc = rt_device_read(flash_devices, page_index, flash_buffer, FLASH_PAGE_SIZE);
                memcpy(flash_buffer,ptr,b_to_wr); 
                
                rc = rt_device_write(flash_devices,
                                    addr, flash_buffer, 1);
                if ( 0 != rc )
                {
                    size -= b_to_wr;
                    addr += b_to_wr;
                    ptr  += b_to_wr;                        
                }
                else
                {
                    size = 0;
                }
            }

            if ( 0 != rc )
            {
                ack_msg.ack = E_STORAGE_ACK_SUCCESS;
            }
            else
            {
                ack_msg.ack = E_STORAGE_ACK_FAIL;
            }            
        }
        if ( rw_msg.req_type < E_STORAGE_TYPE_MAX)
        {
            rt_mq_send(service_flash_rw_ack, &ack_msg, sizeof(READ_WRITE_ACK_STRU));
        }
    }
}

void service_storage_resource_init(void)
{    

    service_eep_rw_req = rt_mq_create("rw_req",
                                    sizeof(READ_WRITE_REQ_STRU),
                                    5,
                                    RT_IPC_FLAG_FIFO);

    service_eep_rw_ack = rt_mq_create("rw_ack",
                                    sizeof(READ_WRITE_REQ_STRU),
                                    5,
                                    RT_IPC_FLAG_FIFO);

    eep_mutex   = rt_mutex_create("eep_mutes", RT_IPC_FLAG_FIFO);  


    service_flash_rw_req = rt_mq_create("rw_req",
                                    sizeof(READ_WRITE_REQ_STRU),
                                    5,
                                    RT_IPC_FLAG_FIFO);

    service_flash_rw_ack = rt_mq_create("rw_ack",
                                    sizeof(READ_WRITE_REQ_STRU),
                                    5,
                                    RT_IPC_FLAG_FIFO);    

    flash_mutex   = rt_mutex_create("flash_mutes", RT_IPC_FLAG_FIFO);  
}

void service_storage_io_init(void)
{    
    rt_thread_t     tid;

    service_storage_resource_init();

    tid = rt_thread_create("eep_io",
                        service_eep_rw_req_process,
                        0,
                        1024,
                        16,
                        25);
    if ( RT_NULL != tid )
    {
        rt_thread_startup(tid);
    }

    tid = rt_thread_create("flash_io",
                        service_flash_rw_req_process,
                        0,
                        1024,
                        16,
                        25);
    if ( RT_NULL != tid )
    {
        rt_thread_startup(tid);
    }    
}

rt_err_t service_ee_write_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size)
{
    READ_WRITE_REQ_STRU rw_msg;
    READ_WRITE_ACK_STRU ack_msg;  

    rw_msg.req_type = E_STORAGE_TYPE_WRITE;
    rw_msg.addr     = addr;
    rw_msg.ptr      = buffer;
    rw_msg.rw_size     = size;

    rt_mq_send(service_eep_rw_req, &rw_msg, sizeof(READ_WRITE_REQ_STRU));

    if(RT_EOK != rt_mq_recv(service_eep_rw_ack, &ack_msg, sizeof(READ_WRITE_ACK_STRU), size/16 + 2))
    {
        return RT_ETIMEOUT;
    }
    else
    {
        if (E_STORAGE_ACK_SUCCESS == ack_msg.ack)
        {
            return RT_EOK;
        }
        else
        {
            return RT_EIO;
        }
    }
}

rt_err_t service_ee_read_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size)
{
    READ_WRITE_REQ_STRU rw_msg;
    READ_WRITE_ACK_STRU ack_msg;  

    rw_msg.req_type = E_STORAGE_TYPE_READ;
    rw_msg.addr     = addr;
    rw_msg.ptr      = buffer;
    rw_msg.rw_size     = size;

    rt_mq_send(service_eep_rw_req, &rw_msg, sizeof(READ_WRITE_REQ_STRU));

    if(RT_EOK != rt_mq_recv(service_eep_rw_ack, &ack_msg, sizeof(READ_WRITE_ACK_STRU), size/16 + 4))
    {
        return RT_ETIMEOUT;
    }
    else
    {
        if (E_STORAGE_ACK_SUCCESS == ack_msg.ack)
        {
            return RT_EOK;
        }
        else
        {
            return RT_EIO;
        }
    }    
}

rt_err_t service_flash_write_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size)
{
    READ_WRITE_REQ_STRU rw_msg;
    READ_WRITE_ACK_STRU ack_msg;  

    rw_msg.req_type = E_STORAGE_TYPE_WRITE;
    rw_msg.addr     = addr;
    rw_msg.ptr      = buffer;
    rw_msg.rw_size     = size;

    rt_mq_send(service_flash_rw_req, &rw_msg, sizeof(READ_WRITE_REQ_STRU));

    if(RT_EOK != rt_mq_recv(service_flash_rw_ack, &ack_msg, sizeof(READ_WRITE_ACK_STRU), size/16 + 2))
    {
        return RT_ETIMEOUT;
    }
    else
    {
        if (E_STORAGE_ACK_SUCCESS == ack_msg.ack)
        {
            return RT_EOK;
        }
        else
        {
            return RT_EIO;
        }
    }
}

rt_err_t service_flash_read_req( rt_uint32_t addr,
                                    rt_uint8_t *buffer, 
                                    rt_uint32_t size)
{
    READ_WRITE_REQ_STRU rw_msg;
    READ_WRITE_ACK_STRU ack_msg;  

    rw_msg.req_type = E_STORAGE_TYPE_READ;
    rw_msg.addr     = addr;
    rw_msg.ptr      = buffer;
    rw_msg.rw_size     = size;

    rt_mq_send(service_flash_rw_req, &rw_msg, sizeof(READ_WRITE_REQ_STRU));

    if(RT_EOK != rt_mq_recv(service_flash_rw_ack, &ack_msg, sizeof(READ_WRITE_ACK_STRU), size/16 + 2))
    {
        return RT_ETIMEOUT;
    }
    else
    {
        if (E_STORAGE_ACK_SUCCESS == ack_msg.ack)
        {
            return RT_EOK;
        }
        else
        {
            return RT_EIO;
        }
    }    
}
