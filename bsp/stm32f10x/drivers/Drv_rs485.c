/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : Drv_rs485.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/8/17
  Last Modified :
  Description   : stm32 driver for rs485
  Function List :
  History       :
  1.Date        : 2017/8/17
    Author      : Enix
    Modification: Created file

******************************************************************************/

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
static rt_serial_t *serial;
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/
typedef enum
{
    SERIAL_PAR_NONE,                /*!< No parity. */
    SERIAL_PAR_ODD,                 /*!< Odd parity. */
    SERIAL_PAR_EVEN                 /*!< Even parity. */
} SERIAL_PARITY;


rt_bool_t Drv_RS485_SerialInit(
    rt_uint32_t     ulBaudRate)
{
    /**
     * set 485 mode receive and transmit control IO
     * @note MODBUS_SLAVE_RT_CONTROL_PIN_INDEX need be defined by user
     */
    rt_pin_mode(MODBUS_SLAVE_RT_CONTROL_PIN_INDEX, PIN_MODE_OUTPUT);

    /* set serial name */

    #if defined(RT_USING_UART2)
    extern struct rt_serial_device serial2;
    serial = &serial2;

    /* set serial configure parameter */
    serial->config.baud_rate = ulBaudRate;
    serial->config.stop_bits = STOP_BITS_1;
    switch(eParity)
    {
        case SERIAL_PAR_NONE: 
        {
            serial->config.data_bits = DATA_BITS_8;
            serial->config.parity = PARITY_NONE;
            break;
        }
        case SERIAL_PAR_ODD: 
        {
            serial->config.data_bits = DATA_BITS_9;
            serial->config.parity = PARITY_ODD;
            break;
        }
        case SERIAL_PAR_EVEN: 
        {
            serial->config.data_bits = DATA_BITS_9;
            serial->config.parity = PARITY_EVEN;
            break;
        }
    }
    /* set serial configure */
    serial->ops->configure(serial, &(serial->config));

    /* open serial device */
    if (!serial->parent.open(&serial->parent,
            RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX )) 
    {
        serial->parent.rx_indicate = serial_rx_ind;
    } else {
        return FALSE;
    }

    /* software initialize */
    rt_event_init(&event_serial, "slave event", RT_IPC_FLAG_PRIO);
    rt_thread_init(&thread_serial_soft_trans_irq,
                   "slave trans",
                   serial_soft_trans_irq,
                   RT_NULL,
                   serial_soft_trans_irq_stack,
                   sizeof(serial_soft_trans_irq_stack),
                   10, 5);
    rt_thread_startup(&thread_serial_soft_trans_irq);

    return TRUE;
    #endif
}


/**
 * This function is serial receive callback function
 *
 * @param dev the device of serial
 * @param size the data size that receive
 *
 * @return return RT_EOK
 */
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size) {
    prvvUARTRxISR();
    return RT_EOK;
}

