/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : stm32fxxx_soft_i2c.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/10/2
  Last Modified :
  Description   : implements for soft iic driver
  Function List :
              stm32_get_scl
              stm32_get_sda
              stm32_mdelay
              stm32_set_scl
              stm32_set_sda
              stm32_udelay
  History       :
  1.Date        : 2017/10/2
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include <board.h>
#include <rtthread.h>

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "rtdevice.h"

#include "delay.h"
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
#define     I2C1_GPIO_RCC   (RCC_APB2Periph_GPIOB)
#define     I2C1_GPIO       (GPIOB)
#define     I2C1_GPIO_SDA   (GPIO_Pin_7)
#define     I2C1_GPIO_SCL   (GPIO_Pin_6)

#define     SDA_IN()  {GPIOB->CRL&=0X0FFFFFFF; GPIOB->CRL|=0X80000000;}
#define     SDA_OUT() {GPIOB->CRL&=0X0FFFFFFF; GPIOB->CRL|=0X30000000;}
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


static void I2C_RCC_Configuration(void)
{
    //RCC->APB2ENR|=1<<4;//先使能外设IO PORTC时钟
    RCC_APB2PeriphClockCmd( I2C1_GPIO_RCC, ENABLE );      
}
/**
  * @brief Init Eeprom gpio
  */
static void I2C_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = I2C1_GPIO_SDA | I2C1_GPIO_SCL;
    GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed =GPIO_Speed_50MHz;
    GPIO_Init(I2C1_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(I2C1_GPIO , I2C1_GPIO_SDA);
    GPIO_SetBits(I2C1_GPIO , I2C1_GPIO_SCL); 
}


void stm32_set_sda(void *data, rt_int32_t state)
{
    if(state == 1)
        GPIO_SetBits(I2C1_GPIO , I2C1_GPIO_SDA);   
    else if(state == 0)
        GPIO_ResetBits(I2C1_GPIO , I2C1_GPIO_SDA); 
}

void stm32_set_scl(void *data, rt_int32_t state)
{
    if(state == 1)
        GPIO_SetBits(I2C1_GPIO , I2C1_GPIO_SCL);   
    else if(state == 0)
        GPIO_ResetBits(I2C1_GPIO , I2C1_GPIO_SCL); 
}

rt_int32_t stm32_get_sda(void *data)
{
    return (rt_int32_t)GPIO_ReadInputDataBit(I2C1_GPIO , I2C1_GPIO_SDA);
}    
rt_int32_t stm32_get_scl(void *data)
{
    return (rt_int32_t)GPIO_ReadInputDataBit(I2C1_GPIO , I2C1_GPIO_SCL);
}

void stm32_udelay(rt_uint32_t us)
{
    delay_us(us);
}

void stm32_mdelay(rt_uint32_t ms)
{
    delay_ms(ms);
}

void stm32_sda_in(void)
{
    SDA_IN();
}

void stm32_sda_out(void)
{
    SDA_OUT();
}    
static const struct  rt_i2c_bit_ops stm32_i2c_bit_ops =
{
    (void*)0xaa,     //no use in set_sda,set_scl,get_sda,get_scl
    stm32_set_sda,
    stm32_set_scl,
    stm32_get_sda,
    stm32_get_scl,
    stm32_udelay,
    20, 
    5
};

int rt_hw_i2c_init(void)
{
    static struct rt_i2c_bus_device stm32_i2c;//"static" add by me. It must be add "static", or it will be hard fault
    
    I2C_RCC_Configuration();
    I2C_GPIO_Configuration();
    
    rt_memset((void *)&stm32_i2c, 0, sizeof(struct rt_i2c_bus_device));
    stm32_i2c.priv = (void *)&stm32_i2c_bit_ops;
    rt_i2c_bit_add_bus(&stm32_i2c, "i2c1");   
        
    return 0;
}
