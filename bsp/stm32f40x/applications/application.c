/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2014-04-27     Bernard      make code cleanup. 
 */

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#include "stm32f4xx_eth.h"
#endif

#ifdef RT_USING_GDB
#include <gdb_stub.h>
#endif

#ifdef RT_USING_SPI
    #ifdef RT_USING_SST25VFXX
    #include "spi_flash_sst25vfxx.h"
    #endif
#endif

#include "rtdevice.h"
#if 0
void rt_tcp_serve_console(void* parameter)
{
    struct tcp_pcb  consolo_pcb;
    rt_uint8_t     *date;

    date = rt_malloc(512);
    if ( RT_NULL == date )
    {
        rt_kprintf("Memory malloc failed!");
        return;
    }
    tcp_listen(consolo_pcb);

    if

}
#endif

void rt_led_toggle_thread_entry(void* parameter)
{
    rt_device_t     device;
    
    device = rt_device_find("pin");
    if (RT_NULL != device )
    {
        rt_pin_mode(53,PIN_MODE_OUTPUT);
        
        while(1)
        {
            if(PIN_LOW == rt_pin_read(53))
            {
                rt_pin_write(53,PIN_HIGH);
            }
            else
            {
                rt_pin_write(53,PIN_LOW);
            }
            rt_thread_delay(RT_TICK_PER_SECOND / 2);
        }
    }
}

void rt_init_thread_entry(void* parameter)
{
    /* GDB STUB */
#ifdef RT_USING_GDB
    gdb_set_device("uart6");
    gdb_start();
#endif

#ifdef RT_USING_I2C

#endif

    /* LwIP Initialization */
#ifdef RT_USING_LWIP
    {
        extern void lwip_sys_init(void);

        /* register ethernetif device */
        eth_system_device_init();

        rt_hw_stm32_eth_init();

        /* init lwip system */
        lwip_sys_init();
        rt_kprintf("TCP/IP initialized!\n");
    }
    

#endif

#ifdef RT_USING_SPI
    #ifdef RT_USING_SST25VFXX
    sst25vfxx_init("extFlash","spi31");
    #endif
#endif

#ifdef RT_USING_RTC
    {
        time_t now;
        rt_device_t device;
        
        device = rt_device_find("rtc");
        if (device != RT_NULL)
        {
            if (rt_device_open(device, 0) == RT_EOK)
            {
                rt_device_control(device, RT_DEVICE_CTRL_RTC_GET_TIME, &now);
                rt_device_close(device);
            }
            rt_kprintf("Today is %s\n", ctime(&now));
        }

    }
#endif

    telnet_srv();
}

int rt_application_init()
{
    rt_thread_t tid;

    tid = rt_thread_create("init",
        rt_init_thread_entry, RT_NULL,
        2048, RT_THREAD_PRIORITY_MAX/3, 20);

    if (tid != RT_NULL)
        rt_thread_startup(tid);
    
    tid = rt_thread_create("led_toggle",
        rt_led_toggle_thread_entry, RT_NULL,
        256, RT_THREAD_PRIORITY_MAX-1, 20);

    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}

/*@}*/
