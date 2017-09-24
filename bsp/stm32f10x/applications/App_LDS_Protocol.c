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
    }
    union
    {
        rt_uint8_t      param[3];
        st_dim_param    dim_param;
    }
    rt_uint8_t      check_sum;
    
}st_cmd_field;

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/
typedef enum CTRL_OPCODE
{
    E_Ctrl_Opcode_Preset                          = 0xfe,
    E_Ctrl_Opcode_Fade_area_to_off                = 0xfd,
    E_Ctrl_Opcode_Program_level_to_preset         = 0xfc,
    E_Ctrl_Opcode_Reset_to_preset                 = 0xfb,
    E_Ctrl_Opcode_Configture_DMX                  = 0xfa,
    E_Ctrl_Opcode_Set_jion                        = 0xf9,
    E_Ctrl_Opcode_Panel_disable                   = 0xf8,
    E_Ctrl_Opcode_Panel_enable                    = 0xf7,
    E_Ctrl_Opcode_Panic                           = 0xf6,
    E_Ctrl_Opcode_Un_panic                        = 0xf5,
    E_Ctrl_Opcode_PE_Speed                        = 0xf4,
    E_Ctrl_Opcode_Suspend_PE                      = 0xf3,
    E_Ctrl_Opcode_Restart_PE                      = 0xf2,
    E_Ctrl_Opcode_Set_air_link                    = 0xf1,
    E_Ctrl_Opcode_Clear_air_link                  = 0xf0,
    E_Ctrl_Opcode_Request_area_link               = 0xef,
    E_Ctrl_Opcode_Relply_area_link                = 0xee,
    E_Ctrl_Opcode_Suspend_motion                  = 0xed,
    E_Ctrl_Opcode_Resume_motion                   = 0xec,
    E_Ctrl_Opcode_Disable_motion_detector         = 0xeb,
    E_Ctrl_Opcode_Disable_motion_detector_this_preset = 0xea,
    E_Ctrl_Opcode_Enable_motion_detector_current_preset = 0xe9,
    E_Ctrl_Opcode_Disable_motion_detector_all_preset = 0xe8,
    E_Ctrl_Opcode_Enable_motion_detector_all_preset  = 0xe7,
    E_Ctrl_Opcode_Set_rmask                          = 0xe6,
    E_Ctrl_Opcode_Fade_channel_area_to_level          = 0xe5,
    E_Ctrl_Opcode_Reply_with_channel_levle            = 0xe4,
    E_Ctrl_Opcode_Request_channel_level               = 0xe3,
    E_Ctrl_Opcode_Reply_current_preset                = 0xe2,
    E_Ctrl_Opcode_Request_current_preset              = 0xe1,
    E_Ctrl_Opcode_Preset_offset_and_bank              = 0xe0,
    E_Ctrl_Opcode_Select_current_preset               = 0xdf,
    E_Ctrl_Opcode_Save_current_preset                   = 0xde,
    E_Ctrl_Opcode_Restore_saved_preset                  = 0xdd,
    E_Ctrl_Opcode_Fade_to_off                           = 0xdc,
    E_Ctrl_Opcode_Fade_to_on                            = 0xdb,
    E_Ctrl_Opcode_Stop_fade                             = 0xda,
    E_Ctrl_Opcode_Fade_channel_to_preset                = 0xd9,
    E_Ctrl_Opcode_Stop_fade_set_toggle_preset           = 0xd8,
    E_Ctrl_Opcode_Stop_fade_set_currten_preset          = 0xd7,
    E_Ctrl_Opcode_Fade_channel_to_toggle_preset         = 0xd6,
    E_Ctrl_Opcode_Fade_channel_to_current_preset        = 0xd5,
    E_Ctrl_Opcode_Toggle_Channel                        = 0xd4,
    E_Ctrl_Opcode_Fade_channel_level_0_1_sec            = 0xd3,
    E_Ctrl_Opcode_Fade_channel_level_1_sec              = 0xd2,
    E_Ctrl_Opcode_Fade_channel_level_1_min              = 0xd1,
    E_Ctrl_Opcode_Inc_level                             = 0xd0,
    E_Ctrl_Opcode_Dec_level                             = 0xcf,
    E_Ctrl_Opcode_Fade_area                             = 0xce,
    E_Ctrl_Opcode_Stop_fade_area                        = 0xcd,                 
}CTRL_OPCODE_ENUM;

typedef enum SETTING_OPCODE
{
    E_Opcode_Device_Identify                 = 0xfe,
    E_Opcode_Setup                           = 0xfd,
    E_Opcode_Reboot_Device                   = 0xfc,
    E_Opcode_Read_EEPROM                     = 0xfb,
    E_Opcode_Write_EEPROM                    = 0xfa,
    E_Opcode_Echo_EEPROM                     = 0xf9,
    E_Opcode_Vrms_Request                    = 0xf8,
    E_Opcode_Phase_A_Vrms                    = 0xf7,
    E_Opcode_CALIBRATE_Device_Phase_to_Vrms  = 0xf6,
    E_Opcode_Light_Level_Request             = 0xf5,
    E_Opcode_Light_Level_of_Device_Reply     = 0xf4,
    E_Opcode_DMX_Port_Control                = 0xf3,
    E_Opcode_DMX_Port_Scene_Capture          = 0xf2,     
    E_Opcode_Start_Task                      = 0xf1,
    E_Opcode_Stop_Task                       = 0xf0,
    E_Opcode_Suspend_Task                    = 0xef,
    E_Opcode_Resume_Task                     = 0xee,
    E_Opcode_Enable_Event                    = 0xed,
    E_Opcode_Disable_Event                   = 0xec,
    E_Opcode_Trigger_Event                   = 0xeb,
    E_Opcode_Flash_Read                      = 0xea,
    E_Opcode_Flash_Read_Ack                  = 0xe9,
    E_Opcode_Flash_Program_Enable            = 0xe8,
    E_Opcode_Flash_Prog_Enable_Ack           = 0xe7,
    E_Opcode_Flash_Write                     = 0xe6,
    E_Opcode_Flash_Write_Ack                 = 0xe5,
    E_Opcode_Flash_Erase                     = 0xe4,
    E_Opcode_Flash_Erase_Ack                 = 0xe3,
    E_Opcode_Read_RAM                        = 0xe2,
    E_Opcode_Write_RAM                       = 0xe1,
    E_Opcode_Echo_RAM                        = 0xe0,
    E_Opcode_BLOCK_EE_Read_Enable            = 0xdf,
    E_Opcode_BLOCK_EE_Read_Ack               = 0xde,
    E_Opcode_BLOCK_EE_Write_Enable           = 0xdd,
    E_Opcode_BLOCK_EE_Write_Ack              = 0xdc,
    E_Opcode_BLOCK_Flash_Read_Enable         = 0xdb,
    E_Opcode_BLOCK_Flash_Read_Ack            = 0xda,
    E_Opcode_BLOCK_Flash_Write_Enable        = 0xd9,
    E_Opcode_BLOCK_Flash_Write_Ack           = 0xd8,
    E_Opcode_Request_Device_Status           = 0xd7,
    E_Opcode_Report_Device_Status            = 0xd6,
    E_Opcode_Request_Channel_Status          = 0xcb,
    E_Opcode_Request_Firmware_version        = 0xfa,
    E_Opcode_Request_Physical_Channel_Level  = 0xb8,
    E_Opcode_Set_Physical_Channel_Level      = 0xb7,
    E_Opcode_DM320_dali_cmd                  = 0x6f
}SETTING_OPCODE_ENUM;
/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
#define MAX_CHANNEL_NUM     12
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

rt_uint8_t LDSCheckSum(rt_uint8_t *buffer,rt_uint8_t Num)
{
	rt_uint8_t i,sum=0;
	for(i=Num;i!=0;i--)
	{
		sum+=*buffer++;
	}
	sum = 256-sum;
	return sum;	
}

rt_bool_t   IsChannelAreaAccept(rt_uint8_t channel,rt_uint8_t area)
{
    return RT_TRUE;
}

rt_bool_t   IsChannelAppendAreaAccpet(rt_uint8_t channel,rt_uint8_t area)
{    
    return RT_TRUE;
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

