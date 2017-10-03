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
channelDataType     device_channel[MAX_CHANNEL_NUM];
/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
DEVICE_SETTING_FLAG_STRU              device_settiong_option; 
rt_uint8_t  g_device_box_num     = 0xFF;
/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

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
    if ( device_channel[channel] == area)
    {
        return RT_TRUE; 
    }
    else
    {
        return RT_FALSE;
    }
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
    return device_settiong_option.is_device_selected;
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
        ret = device_settiong_option.is_device_selected = RT_TRUE;

    }    
    else if ( DEVICE_CODE != parameter->device_code)
    {
        ret = device_settiong_option.is_device_selected = RT_FALSE;

    }
    else if (g_device_box_num == parameter->box_num)
    {
        ret = device_settiong_option.is_device_selected = RT_TRUE;

    }
    else
    {
        ret = device_settiong_option.is_device_selected = RT_FALSE;
    }

    return ret;
    
}

void SetDeviceDeselectForce(void)
{    
    device_settiong_option.is_device_selected = RT_FALSE;
}

void SetBlockRWAddress(rt_uint16_t address)
{    
    device_settiong_option.is_device_EE_BlockOption = RT_TRUE;
    device_settiong_option.block_ee_address         = address;
}

void SetBlockRWExit(void)
{    
    device_settiong_option.is_device_EE_BlockOption = RT_FALSE;
    device_settiong_option.block_ee_address         = 0;
}

rt_bool_t IsDeviceEEBlockOptionEnable(void)
{    
    return device_settiong_option.is_device_EE_BlockOption;
}

rt_size_t App_LDSCmd_Ctrl_Preset(void *parameter,rt_uint8_t *ack_buffer)
{
    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter;

    rt_uint8_t      i;
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_rate;

    if ( rcv_cmd->dim_param.bank >95)
    {
        return 0;
    }

    fade_rate = rcv_cmd->dim_param.fade_rate_low;
    fade_rate <<=8;
    fade_rate = rcv_cmd->dim_param.fade_rate_high;

    fade_rate = fade_rate * RT_TICK_PER_SECOND / 50;
    
    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {
        if ((RT_TRUE == IsChannelAreaAccept(i, rcv_cmd->area))
            &&(RT_TRUE == IsChannelAppendAreaAccpet(i, rcv_cmd->append_area)))
        {
            device_channel[i].;
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

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

        device = rt_device_find("EEP");

        if  (RT_NULL != device )
        {
            if ( RT_EOK == rt_device_open(device, 0))
            {   
                extern uint8_t  tmp_data[4096];
                rt_device_read(device, address, tmp_data, 1);
                rt_device_close(device);
                memcpy(ack_buffer,parameter,6);
                memcpy(ack_buffer+6 ,tmp_data,1);
                ack_buffer[3] = E_Opcode_Echo_EEPROM;

                return 7;
            }
            else
            {
                return 0;
            }
        }
    
    return 0;
}

rt_size_t App_LDS_CMD_WriteEE(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint32_t address;
        rt_device_t device;

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

        device = rt_device_find("EEP");

        if  (RT_NULL != device )
        {
            if ( RT_EOK == rt_device_open(device, 0))
            {
                extern uint8_t  tmp_data[4096];
                rt_device_write(device, address, parameter->param + 2, 1);
                memcpy(ack_buffer,parameter,6);
                rt_device_read(device, address, tmp_data, 1);
                memcpy(ack_buffer+6 ,tmp_data,1);
                rt_device_close(device);
                ack_buffer[3] = E_Opcode_Echo_EEPROM;
                return 7;
            }
            else
            {
                return 0;
            }
        }
    }
    return 0;
}
rt_size_t App_LDS_CMD_BlockReadEEEnable(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint16_t address;
        rt_device_t device;

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

        device = rt_device_find("EEP");

        if  (RT_NULL != device )
        {
            SetBlockRWAddress(address);
            
            memcpy(ack_buffer,parameter,6);
            
            ack_buffer[3] = E_Opcode_BLOCK_EE_Read_Ack;
            ack_buffer[6] = 0;
            return 7;            
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

rt_size_t App_LDS_CMD_BlockReadEEAck(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint16_t address;
        rt_device_t device;

        if (RT_FALSE == IsDeviceEEBlockOptionEnable() )
        {
            return 0;
        }

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

        device = rt_device_find("EEP");

        if  (RT_NULL != device )
        {
            extern rt_uint8_t  tmp_data[4096];
            SetBlockRWAddress(address);

            if ( RT_EOK == rt_device_open(device, 0))
            {
                rt_device_read(device, address, tmp_data, 6);
                rt_device_close(device);
                
                ack_buffer[0] = 0xFC;
                memcpy(ack_buffer + 1, tmp_data,6);
                return 7;            
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

rt_size_t App_LDS_CMD_BlockWriteEEEnable(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint16_t address;
        rt_device_t device;

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

        device = rt_device_find("EEP");

        if  (RT_NULL != device )
        {
            SetBlockRWAddress(address);
            
            memcpy(ack_buffer,parameter,6);

            ack_buffer[3] = E_Opcode_BLOCK_EE_Write_Ack;
            ack_buffer[6] = 0;
            return 7;            
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

rt_size_t App_LDS_CMD_Requset_Version(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{ 
    ack_buffer[0] = 0xFA;
    ack_buffer[1] = DEVICE_CODE;
    ack_buffer[2] = g_device_box_num;
    ack_buffer[3] = E_Opcode_Device_Identify;
    ack_buffer[4] = 1;
    ack_buffer[5] = 0;            
    ack_buffer[6] = 0;
    return 7;
}
const SETTING_PROCESS_HANDLER_STRU setting_handler_array[]=
{    
    {E_Opcode_Setup                 ,App_LDSCmd_Setting_Setup},
    {E_Opcode_Reboot_Device         ,App_LDSCmd_Reboot       },
    {E_Opcode_Read_EEPROM           ,App_LDS_CMD_ReadEE      },
    {E_Opcode_Write_EEPROM          ,App_LDS_CMD_WriteEE     },
    {E_Opcode_BLOCK_EE_Read_Enable  ,App_LDS_CMD_BlockReadEEEnable     },
    {E_Opcode_BLOCK_EE_Write_Enable ,App_LDS_CMD_BlockWriteEEEnable     },
    {E_Opcode_BLOCK_EE_Read_Ack     ,App_LDS_CMD_BlockReadEEAck},
    {E_Opcode_Request_Firmware_version,App_LDS_CMD_Requset_Version},
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
    return 0;
}
rt_size_t App_LDS_BlockWrite_Handler(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{   
    #define EE_PAGE_SIZE    16
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint8_t  temp[8];
        rt_uint8_t *in;

        if (RT_FALSE == IsDeviceEEBlockOptionEnable())
        {
            return 0;
        }
        else
        {    
            rt_uint16_t     address;
            rt_uint16_t     i,j;
            rt_device_t     device;

            in = (rt_uint8_t *)parameter;
            memcpy(temp,in+1,6);

            address = device_settiong_option.block_ee_address;
                
            device = rt_device_find("EEP");
            if  (RT_NULL == device )
            {
                return 0;
            }
            
    		i = address%256;
    		j = i-(i>>4)*16+5;
    		if(j < 16)
    		{
    			rt_device_write(device,address,temp,6);
    		}
    		else
    		{
    			i=j-15;
    			j=6-i;
    			rt_device_write(device,address
    								,temp
    								,j);

    			rt_thread_delay(1);

    			rt_device_write(device,address+j
    								,temp+j
    								,i);
    		}
                                        
            ack_buffer[0] = 0xFA;
            ack_buffer[1] = DEVICE_CODE;
            ack_buffer[2] = g_device_box_num;
            ack_buffer[3] = E_Opcode_BLOCK_EE_Write_Ack;
            ack_buffer[4] = (address >> 8)&0xFF;
            ack_buffer[5] = (address )&0xFF;            
            ack_buffer[6] = 0;

            address += 6;
            SetBlockRWAddress(address);
            return 7;  
        }
    }
    return 0;
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

    init_struct.header_param[0] = 0xFC;
    init_struct.process_handler = App_LDS_BlockWrite_Handler;
    Service_Rs485_CMD_Register(&init_struct);

    
}
