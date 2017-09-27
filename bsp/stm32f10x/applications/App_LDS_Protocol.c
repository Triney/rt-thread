/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : App_LDS_Protocol.c
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/9/6
  Last Modified :
  Description   : LDS 485 protocol process
  Function List :
  History       :
  1.Date        : 2017/9/6
    Author      : Enix
    Modification: Created file

******************************************************************************/
#include "App_LDS_Protocol.h"
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
typedef struct Flag
{
    rt_uint8_t  reserve:6;
    rt_uint8_t  dim_dir:1;
    rt_uint8_t  is_repeat:1;
}bf_ChannelFlag;
 
typedef     void (*pFunRelayTimer)(void *parameter);
 
typedef struct channelDataType_tag
{
	rt_uint8_t		    GoalLevel;          //通道需要变化到的目标电平    
	rt_uint8_t		    OringalLevel;		//通道变化前的电平
	rt_uint8_t         	PresetOffset;       //场景偏移量          
	bf_ChannelFlag      flag_bit;  			//bit0:重复（1yes，0no）
	
	rt_uint8_t	        LogicChannel;		//逻辑通道号
	rt_uint8_t         	PrePreset;			//先前场景
	rt_uint8_t	        MaxLevel;			//最大电平值
	rt_uint8_t	        AreaLink;			//区域连接值
	
	rt_uint8_t	        SwitchLevel;		//开关电平值
	rt_uint8_t         	CurrentLevel;		//当前电平值
	rt_uint8_t         	Area;				//所属区
	rt_uint8_t	        AppendArea;			//附加区

    rt_uint8_t          ChangeStep;         //亮度改变的步长
    rt_uint8_t          ucReserve[3];       //保留字节，对齐用
	
	rt_uint16_t    	    PastedTime;			//已经经过的渐变时间
	rt_uint16_t    	    timeToGoal;	        //总共需要渐变的时间
	
    pFunRelayTimer      timeout_func;
    rt_timer_t          tim_action;
} channelDataType;
 
typedef struct dim_param
{
    rt_uint8_t      bank;
    rt_uint8_t      fade_rate_low;
    rt_uint8_t      fade_rate_high;
}st_dim_param;

typedef struct cmd_field
{
    rt_uint8_t      header;
    #pragma anon_unions 
    union
    {
        rt_uint8_t  device_code;
        rt_uint8_t  area;
    };
    union
    {
        rt_uint8_t  box_num;
        rt_uint8_t  append_area;
    };
    union
    {
        rt_uint8_t          option_code;
        CTRL_OPCODE_ENUM    ctrl_opcode;
        SETTING_OPCODE_ENUM setting_opcode;
    };
    union
    {
        rt_uint8_t      param[3];
        st_dim_param    dim_param;
    };
    rt_uint8_t      check_sum;
    
}st_cmd_field;

typedef struct SETTING_PROCESS_HANDLER
{
    SETTING_OPCODE_ENUM                 command;
    rt_size_t   (*command_handler)(st_cmd_field *parameter,rt_uint8_t *ack_buffer);
}SETTING_PROCESS_HANDLER_STRU;
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
rt_bool_t   g_is_device_selected = RT_FALSE;
rt_uint8_t  g_device_box_num     = 0xFF;
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
#define LDS_COMMAND_MAX_LEN 8
#define MAX_CHANNEL_NUM     12
#define DEVICE_CODE         0x94
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

rt_uint32_t LDSCheckSum(rt_uint8_t *buffer,rt_uint32_t Num)
{
	rt_uint8_t i,sum=0;
	for(i=Num;i!=0;i--)
	{
		sum+=*buffer++;
	}
	sum = 256-sum;
	return (rt_uint32_t)sum;	
}

rt_bool_t   IsChannelAreaAccept(rt_uint8_t channel,rt_uint8_t area)
{
    return RT_TRUE;
}

rt_bool_t   IsChannelAppendAreaAccpet(rt_uint8_t channel,rt_uint8_t area)
{    
    return RT_TRUE;
}

/*****************************************************************************
 Prototype    : IsDeviceSelect
 Description  : get current device select status
 Input        : void  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2017/9/26
    Author       : Enix
    Modification : Created function

*****************************************************************************/
rt_bool_t IsDeviceSelect(void)
{
    return g_is_device_selected;
}

/*****************************************************************************
 Prototype    : SetDeviceSelect
 Description  : set device selected if cmd parameter want to select device
 Input        : st_cmd_field *parameter  
 Output       : None
 Return Value : RT_TRUE     device selected
                RT_FALSE    device unselected
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2017/9/26
    Author       : Enix
    Modification : Created function

*****************************************************************************/
rt_bool_t SetDeviceSelect(st_cmd_field *parameter)
{
    rt_bool_t   ret;
    if ( (0xAA == parameter->device_code)
        &&(0x55 == parameter->box_num))
    {
        ret = g_is_device_selected = RT_TRUE;

    }    
    else if ( DEVICE_CODE != parameter->device_code)
    {
        ret = g_is_device_selected = RT_FALSE;

    }
    else if (g_device_box_num == parameter->box_num)
    {
        ret = g_is_device_selected = RT_TRUE;

    }
    else
    {
        ret = g_is_device_selected = RT_FALSE;
    }

    return ret;
    
}

void SetDeviceDeselectForce(void)
{    
    g_is_device_selected = RT_FALSE;
}

void App_LDSCmd_Ctrl_Preset(void *parameter)
{
    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter;

    rt_uint8_t  i;
    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {
        if ((RT_TRUE == IsChannelAreaAccept(i, rcv_cmd->area))
            &&(RT_TRUE == IsChannelAppendAreaAccpet(i, rcv_cmd->append_area)))
        {
            ;
        }
    }

}

rt_size_t App_LDSCmd_Setting_Setup(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( 0x01 == parameter->param[0]);
    {
        if ( 0x01 == parameter->param[1])
        {
            SetDeviceSelect(parameter);
        }
        else
        {
            SetDeviceDeselectForce();
        }
    }
    return 0;
}

rt_size_t App_LDSCmd_Reboot(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        NVIC_SystemReset();
    }
    return 0;
}

rt_size_t App_LDS_CMD_ReadEE(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    

        rt_uint32_t address;
        rt_device_t device;

        address  = parameter->param[1];
        address  = address<<8;
        address |= parameter->param[0];

        device = rt_device_find("Flash0");

        if  (RT_NULL != device )
        {
            rt_device_read(device, address, ack_buffer + 6, 1);
            memcpy(ack_buffer,parameter,6);
            ack_buffer[3] = E_Opcode_Echo_EEPROM;
            return 7;
        }
    
    return 0;
}

rt_size_t App_LDS_CMD_WriteEE(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint32_t address;
        rt_device_t device;

        address  = parameter->param[1];
        address  = address<<8;
        address |= parameter->param[0];

        device = rt_device_find("Flash0");

        if  (RT_NULL != device )
        {
            rt_device_write(device, address, ack_buffer + 6, 1);
            memcpy(ack_buffer,parameter,6);
            rt_device_read(device, address, ack_buffer + 6, 1);
            ack_buffer[3] = E_Opcode_Echo_EEPROM;
            return 7;
        }
    }
    return 0;
}

const SETTING_PROCESS_HANDLER_STRU setting_handler_array[]=
{    
    {E_Opcode_Setup         ,App_LDSCmd_Setting_Setup},
    {E_Opcode_Reboot_Device ,App_LDSCmd_Reboot       },
    {E_Opcode_Read_EEPROM   ,App_LDS_CMD_ReadEE      },
    {E_Opcode_Write_EEPROM  ,App_LDS_CMD_WriteEE     },
};
rt_size_t   setting_hanler_cnt = sizeof(setting_handler_array)/sizeof(SETTING_PROCESS_HANDLER_STRU);
rt_size_t App_LDS_Setting_Handler(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t  i;
    
    for ( i = 0 ; i < setting_hanler_cnt ; i++ )
    {
        if (setting_handler_array[i].command == parameter->option_code )
        {
            return setting_handler_array[i].command_handler(parameter,ack_buffer);
        }
    }
    return RT_FALSE;
}    

void App_LDS_GetParameter(rt_uint8_t *in, st_cmd_field *param)
{    
    memcpy(param,in,8);
}

void App_LDS_Protocol_Register(void)
{    
    PROTOCOL_PROPERITY_STRU init_struct;
    st_cmd_field           *param_ptr;
    rt_uint8_t             *ack_ptr;

    Service_Rs485_RegisterStruct_Init(&init_struct);

    init_struct.header_type = E_SOP_PARTICIAL;
    init_struct.header_len  = 1;
    init_struct.header_param[0] = 0xFA;

    init_struct.checksum_len = 1;
    init_struct.checksum_calc = LDSCheckSum;

    ack_ptr = rt_malloc(LDS_COMMAND_MAX_LEN + 4); // protect overflow

    init_struct.ack = ack_ptr;

    param_ptr = rt_malloc(sizeof(st_cmd_field));
    init_struct.parameter = param_ptr;
    init_struct.get_parameter = App_LDS_GetParameter;
    
    init_struct.process_handler = App_LDS_Setting_Handler;
    
    Service_Rs485_CMD_Register(&init_struct);
}
