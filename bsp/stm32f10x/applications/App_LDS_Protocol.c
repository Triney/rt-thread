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
#include "stdio.h"
#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/
extern struct rt_mailbox       *mb_relay_acton;
/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
typedef struct Flag
{
    rt_uint8_t  reserve:4;
    rt_uint8_t  is_dimming:1;
    rt_uint8_t  is_switch:1;
    rt_uint8_t  dim_dir:1;
    rt_uint8_t  is_repeat:1;
}bf_ChannelFlag;
 

 
typedef struct channelDataType_tag
{
	rt_uint8_t		    TargetLevel;          //通道需要变化到的目标电平    
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
    rt_uint8_t          OnDelay;
    rt_uint8_t          OffDelay;           
    rt_uint8_t          action;       //保留字节，对齐用
	
	rt_uint32_t    	    StartTime;			//已经经过的渐变时间
	rt_uint32_t    	    TotalTime;	        //总共需要渐变的时间
	
//    typedef     void (*timeoutFunc)(void *parameter);
//    void               *parameter;
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

typedef struct CTRL_PROCESS_HANDLER
{
    CTRL_OPCODE_ENUM                 command;
    rt_size_t   (*command_handler)(st_cmd_field *parameter,rt_uint8_t *ack_buffer);
}CTRL_PROCESS_HANDLER_STRU;
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
channelDataType     g_device_channel[MAX_CHANNEL_NUM];
/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
DEVICE_PRIORITY_REF_STRU                g_device_setting_ref = 
{
    0x94,0xFF,0x0100,0x800,0x10,0x0,0
};
DEVICE_SETTING_FLAG_STRU                device_settiong_option; 

rt_device_t         eep_device;
rt_uint8_t          g_device_box_num     = 0xFF;
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

static void DeviceChannelAct(void *parameter)
{    
    rt_uint8_t      *pst = (rt_uint8_t *)parameter;

    rt_uint8_t      channel;
    rt_uint8_t      is_channal_act_on;  
    rt_uint32_t     value;

    value   = 0;
    channel = *pst & 0x0f;

    is_channal_act_on = *pst & 0x10;

    if ( channel >= 12)
    {
        return;
    }
    
    g_device_channel[channel].flag_bit.is_dimming = RT_FALSE;
    g_device_channel[channel].OringalLevel = g_device_channel[channel].TargetLevel;

    if(g_device_channel[channel].TargetLevel >= g_device_channel[channel].SwitchLevel)
    {
        value |= (1<<4);
    }

    value |= channel;
    rt_mb_send(mb_relay_acton, value);
    
    return;
}
static rt_bool_t   IsChannelAreaAccept(rt_uint8_t channel,rt_uint8_t area)
{
    if ( 0 == area )
    {
        return RT_TRUE; 
    }
    if ( g_device_channel[channel].Area == area)
    {
        return RT_TRUE; 
    }
    else
    {
        return RT_FALSE;
    }
}

static rt_bool_t   IsChannelAppendAreaAccpet(rt_uint8_t channel,rt_uint8_t area)
{    
    return RT_TRUE;
}


static rt_bool_t IsLogicChannelAccpet(rt_uint8_t channel,rt_uint8_t logic_channel)
{    
    if ( g_device_channel[channel].LogicChannel == logic_channel)
    {
        return RT_TRUE;
    }
    else
    {
        return RT_FALSE;
    }
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
static rt_bool_t IsDeviceSelect(void)
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
static rt_bool_t SetDeviceSelect(st_cmd_field *parameter)
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
    else if (g_device_setting_ref.Device_box_num == parameter->box_num)
    {
        ret = device_settiong_option.is_device_selected = RT_TRUE;

    }
    else
    {
        ret = device_settiong_option.is_device_selected = RT_FALSE;
    }

    return ret;
    
}

static void SetDeviceDeselectForce(void)
{    
    device_settiong_option.is_device_selected = RT_FALSE;
}

static void SetBlockRWAddress(rt_uint16_t address)
{    
    device_settiong_option.is_device_EE_BlockOption = RT_TRUE;
    device_settiong_option.block_ee_address         = address;
}

static void SetBlockRWExit(void)
{    
    device_settiong_option.is_device_EE_BlockOption = RT_FALSE;
    device_settiong_option.block_ee_address         = 0;
}

static rt_bool_t IsDeviceEEBlockOptionEnable(void)
{    
    return device_settiong_option.is_device_EE_BlockOption;
}

static rt_uint32_t GetDimFadeTime(rt_uint8_t high,rt_uint8_t low)
{
    rt_uint32_t fade_rate;

    fade_rate = high;
    fade_rate <<=8;
    fade_rate |= low;
    fade_rate = fade_rate * RT_TICK_PER_SECOND / 50;

    return fade_rate;
}

static void LDSSetChannelTargetLevel(rt_uint8_t channel,
                                             rt_uint8_t target_level,
                                             rt_uint32_t fade_time)
{   
    rt_uint8_t      cmp_lvl;
    rt_uint32_t     actual_fade;
    channelDataType *ptr;

    ptr = &g_device_channel[channel];

    if (RT_TRUE == ptr->flag_bit.is_dimming)
    {
        ptr->CurrentLevel = (rt_tick_get() - ptr->StartTime)\
            *(ptr->TargetLevel - ptr->OringalLevel)/ptr->TotalTime;
    }
    else
    {
        ptr->CurrentLevel = ptr->TargetLevel;
    }


    ptr->TargetLevel = target_level;


    if (ptr->TargetLevel > ptr->MaxLevel)
    {
        ptr->TargetLevel = ptr->MaxLevel;
    }  

    if ( RT_TRUE == ptr->flag_bit.is_switch)
    {
        if ( ptr->TargetLevel >= ptr->SwitchLevel)
        {
            ptr->TargetLevel = DIM_LEVEL_MAX;//最亮值
        }
        else
        {
            ptr->TargetLevel = DIM_LEVEL_MIN;
        }
        cmp_lvl = ptr->SwitchLevel;
    }
    else
    {
        cmp_lvl = ptr->TargetLevel;
    }
  
    if ( cmp_lvl < ptr->CurrentLevel)
    {
        ptr->flag_bit.dim_dir       = E_DIM_DOWN;
        ptr->flag_bit.is_dimming    = RT_TRUE;

        actual_fade = (ptr->CurrentLevel - cmp_lvl) \
                        *fade_time / RT_UINT8_MAX;
        actual_fade = actual_fade + (RT_TICK_PER_SECOND * ptr->OffDelay *60);
        
        rt_timer_control(ptr->tim_action,
                         RT_TIMER_CTRL_SET_TIME,
                         &actual_fade);

        rt_timer_start(ptr->tim_action);
    }
    else if ( cmp_lvl > ptr->CurrentLevel)
    {
        ptr->flag_bit.dim_dir       = E_DIM_UP;
        ptr->flag_bit.is_dimming    = RT_TRUE;
        
        actual_fade = (cmp_lvl - ptr->CurrentLevel) \
                        *fade_time / RT_UINT8_MAX;
        
        actual_fade += (RT_TICK_PER_SECOND * ptr->OnDelay *60);
        rt_timer_control(ptr->tim_action,
                         RT_TIMER_CTRL_SET_TIME,
                         &actual_fade); 
        
        rt_timer_start(ptr->tim_action);
    }
    else
    {
        ptr->flag_bit.is_dimming    = RT_FALSE;
    }    
}

static rt_size_t App_LDSCmd_Ctrl_Preset(void *parameter,rt_uint8_t *ack_buffer)
{
    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter;

    rt_uint8_t      i;
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_rate;
    rt_uint32_t     PresetAddress;


    if ( rcv_cmd->dim_param.bank >95)
    {
        return 0;
    }

    fade_rate = GetDimFadeTime(rcv_cmd->dim_param.fade_rate_high,\
                               rcv_cmd->dim_param.fade_rate_low);

    PresetAddress = ADDRESS_PRESET_START + 
                    (g_device_setting_ref.PresetOffset + rcv_cmd->dim_param.bank)*12;

    rt_device_read(eep_device,PresetAddress,Preset_Target_Level,12);
    
    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        Preset_Target_Level[i] = ~Preset_Target_Level[i];

        if ( 0xFF ==Preset_Target_Level[i] )
        {
            continue;
        }

        LDSSetChannelTargetLevel(i,Preset_Target_Level[i],fade_rate);
    }
    return 0;
}

rt_size_t App_LDS_CMD_Ctrl_Fade_Area_Off(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter;   

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        LDSSetChannelTargetLevel(i,DIM_LEVEL_MIN,fade_time);
    }
}
#if 0
rt_size_t App_LDS_CMD_Ctrl_Save_Preset(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;
    rt_uint32_t     PresetAddress;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter;    
    
    if ( rcv_cmd->dim_param.bank >95)
    {
        return 0;
    }
    
    PresetAddress = ADDRESS_PRESET_START + 
                    (g_device_setting_ref.PresetOffset + rcv_cmd->dim_param.bank)*12;
    
    rt_device_read(eep_device,PresetAddress,Preset_Target_Level,12);
  
    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        Preset_Target_Level[i] = g_device_channel[i].TargetLevel;
    }
}
#endif
rt_size_t App_LDS_CMD_Ctrl_Fade_CH_Off(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND / 10;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( rcv_cmd->param[1] == g_device_channel[i].LogicChannel
            ||(0xFF == rcv_cmd->param[1]))
        {
            LDSSetChannelTargetLevel(i,DIM_LEVEL_MIN,fade_time);
        }
    }    
    return 0;
}

rt_size_t App_LDS_CMD_Ctrl_Fade_CH_On(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND / 10;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {
            LDSSetChannelTargetLevel(i,DIM_LEVEL_MAX,fade_time);
        }
    }    
    return 0;
}
#if 0
rt_size_t App_LDS_CMD_Ctrl_Stop_Fade(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND / 10;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {
            LDSSetChannelTargetLevel(i,DIM_LEVEL_MAX,fade_time);
        }
    }    
    return 0;
}
#endif
rt_size_t App_LDS_CMD_Ctrl_Fade_CH_To_Level_100ms(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      target_level;    
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    target_level = ~rcv_cmd->param[2];

    if ( 0xFF == target_level )
    {
        return 0;
    }

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND / 10;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {

            LDSSetChannelTargetLevel(i,target_level,fade_time);
        }
    }    
    return 0;
}

rt_size_t App_LDS_CMD_Ctrl_Fade_CH_To_Level_1s(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      target_level;    
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    target_level = ~rcv_cmd->param[2];

    if ( 0xFF == target_level )
    {
        return 0;
    }

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {
            LDSSetChannelTargetLevel(i,target_level,fade_time);
        }
    }    
    return 0;
}

rt_size_t App_LDS_CMD_Ctrl_Increa_CH_Level(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;  
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint16_t     target_level;      
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND / 10;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {
            target_level = g_device_channel[i].TargetLevel + rcv_cmd->param[2];
            if  ( target_level > DIM_LEVEL_MAX)
            {
                target_level = DIM_LEVEL_MAX;
            }
            LDSSetChannelTargetLevel(i,(rt_uint8_t)target_level,fade_time);
        }
    }    
    return 0;
}

rt_size_t App_LDS_CMD_Ctrl_Decrea_CH_Level(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;  
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_int16_t     target_level;      
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND / 10;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {
            target_level = g_device_channel[i].TargetLevel - rcv_cmd->param[2];
            if  ( target_level < DIM_LEVEL_MIN)
            {
                target_level = DIM_LEVEL_MIN;
            }
            LDSSetChannelTargetLevel(i,(rt_uint8_t)target_level,fade_time);
        }
    }    
    return 0;
}


rt_size_t App_LDS_CMD_Ctrl_Fade_CH_To_Level_1min(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t      i;
    rt_uint8_t      target_level;    
    rt_uint8_t      Preset_Target_Level[MAX_CHANNEL_NUM];
    rt_uint32_t     fade_time;

    st_cmd_field *rcv_cmd = (st_cmd_field *)parameter; 

    target_level = ~rcv_cmd->param[2];

    if ( 0xFF == target_level )
    {
        return 0;
    }

    fade_time = rcv_cmd->param[0] * RT_TICK_PER_SECOND * 60;
    fade_time++;    //add a tick to avoid 0ms fade time

    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {       
        if (RT_TRUE != IsChannelAreaAccept(i, rcv_cmd->area))
        {
            continue;
        }    

        if(RT_TRUE != IsChannelAppendAreaAccpet(i, rcv_cmd->append_area))
        {
            continue;
        }

        if ( (rcv_cmd->param[1] == g_device_channel[i].LogicChannel)
            ||(0xFF == rcv_cmd->param[1]))
        {
            LDSSetChannelTargetLevel(i,target_level,fade_time);
        }
    }    
    return 0;
}

const CTRL_PROCESS_HANDLER_STRU ctrl_handler_array[]=
{    
    {E_Ctrl_Opcode_Preset               ,App_LDSCmd_Ctrl_Preset},
    {E_Ctrl_Opcode_Fade_area_to_off     ,App_LDS_CMD_Ctrl_Fade_Area_Off       },
    {E_Ctrl_Opcode_Fade_to_off          ,App_LDS_CMD_Ctrl_Fade_CH_Off},
    {E_Ctrl_Opcode_Fade_to_on           ,App_LDS_CMD_Ctrl_Fade_CH_On     },
    {E_Ctrl_Opcode_Fade_CH_lvl_0_1_sec  ,App_LDS_CMD_Ctrl_Fade_CH_To_Level_100ms},
    {E_Ctrl_Opcode_Fade_CH_lvl_1_sec    ,App_LDS_CMD_Ctrl_Fade_CH_To_Level_1s},
    {E_Ctrl_Opcode_Fade_CH_lvl_min      ,App_LDS_CMD_Ctrl_Fade_CH_To_Level_1min},    
    {E_Ctrl_Opcode_Inc_level            ,App_LDS_CMD_Ctrl_Increa_CH_Level},
    {E_Ctrl_Opcode_Dec_level            ,App_LDS_CMD_Ctrl_Decrea_CH_Level},    
};
rt_size_t   ctrl_hanler_cnt = sizeof(ctrl_handler_array)/sizeof(SETTING_PROCESS_HANDLER_STRU);
rt_size_t App_LDS_Ctrl_Handler(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{
    rt_uint8_t  i;
    
    for ( i = 0 ; i < ctrl_hanler_cnt ; i++ )
    {
        if (ctrl_handler_array[i].command == parameter->option_code )
        {
            return ctrl_handler_array[i].command_handler(parameter,ack_buffer);
        }
    }
    return 0;
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
    rt_uint32_t     address;
    rt_size_t       ret;
    if ( RT_TRUE == SetDeviceSelect(parameter))
    {
        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

        if ( RT_EOK == rt_device_open(eep_device, 0))
        {   
            extern rt_uint8_t  tmp_data[4096];
            rt_device_read(eep_device, address, tmp_data, 1);
            rt_device_close(eep_device);
            memcpy(ack_buffer,parameter,6);
            memcpy(ack_buffer+6 ,tmp_data,1);
            ack_buffer[3] = E_Opcode_Echo_EEPROM;

            ret = 7;
        }
        else
        {
            ret = 0;
        }    
        SetDeviceDeselectForce();
    }
    else
    {    
        ret = 0;
    }
    return ret;
}

rt_size_t App_LDS_CMD_WriteEE(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint32_t address;

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];
        
        if ( RT_EOK == rt_device_open(eep_device, 0))
        {
            extern rt_uint8_t  tmp_data[4096];
            rt_device_write(eep_device, address, parameter->param + 2, 1);
            rt_thread_delay(RT_TICK_PER_SECOND / 100);
            
            memcpy(ack_buffer,parameter,6);
            rt_device_read(eep_device, address, tmp_data, 1);
            memcpy(ack_buffer+6 ,tmp_data,1);
            rt_device_close(eep_device);
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
rt_size_t App_LDS_CMD_BlockReadEEEnable(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint16_t address;
        
        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

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

rt_size_t App_LDS_CMD_BlockReadEEAck(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint16_t address;
        extern rt_uint8_t  tmp_data[4096];
        
        if (RT_FALSE == IsDeviceEEBlockOptionEnable() )
        {
            return 0;
        }

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];       

        SetBlockRWAddress(address);

        if ( RT_EOK == rt_device_open(eep_device, 0))
        {
           rt_device_read(eep_device, address, tmp_data, 6);
           rt_device_close(eep_device);
           
           ack_buffer[0] = 0xFC;
           memcpy(ack_buffer + 1, tmp_data,6);
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

rt_size_t App_LDS_CMD_BlockWriteEEEnable(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{    
    if ( RT_TRUE == IsDeviceSelect())
    {
        rt_uint16_t address;

        address  = (rt_uint32_t)parameter->param[0];
        address  = address<<8;
        address |= (rt_uint32_t)parameter->param[1];

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

rt_size_t App_LDS_CMD_Requset_Version(st_cmd_field *parameter,rt_uint8_t *ack_buffer)
{ 
    ack_buffer[0] = 0xFA;
    ack_buffer[1] = DEVICE_CODE;
    ack_buffer[2] = g_device_setting_ref.Device_box_num;
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

            in = (rt_uint8_t *)parameter;
            memcpy(temp,in+1,6);

            address = device_settiong_option.block_ee_address;
                
    		i = address%256;
    		j = i-(i>>4)*16+5;
    		if(j < 16)
    		{
    			rt_device_write(eep_device,address,temp,6);
    		}
    		else
    		{
    			i=j-15;
    			j=6-i;
    			rt_device_write(eep_device,address
    								,temp
    								,j);

    			rt_thread_delay(RT_TICK_PER_SECOND / 100);

    			rt_device_write(eep_device,address+j
    								,temp+j
    								,i);
    		}
                                        
            ack_buffer[0] = 0xFA;
            ack_buffer[1] = DEVICE_CODE;
            ack_buffer[2] = g_device_setting_ref.Device_box_num;
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

    init_struct.header_param[0] = 0xF5;
    init_struct.process_handler = App_LDS_Ctrl_Handler;
    Service_Rs485_CMD_Register(&init_struct);    
    
}

void App_LDS_Device_Channel_Property_Get(void)
{    
    rt_uint8_t      i;

    char            name[6] = "CH00";
    // read channel area info from ee
    {
        rt_uint8_t      area[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_AREA,area,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            if ( i == 9 )
            {
                strcpy(name,"CH10");
            }
            else
            {
                name[3]++;
            }
            
            g_device_channel[i].Area = area[i];
            g_device_channel[i].flag_bit.is_switch  = RT_TRUE;
            g_device_channel[i].action = i;
            g_device_channel[i].tim_action \
                = rt_timer_create(name, 
                                  DeviceChannelAct, &(g_device_channel[i].action), 
                                  10, 
                                  (RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER));

        }
    }
    
    {
        rt_uint8_t      area_append[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_AREA_ADDPEND,area_append,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            g_device_channel[i].AppendArea= area_append[i];
        }
    }    
    
    {
        rt_uint8_t      logic_channel[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_LOGIC_CHANNEL,logic_channel,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            g_device_channel[i].LogicChannel= logic_channel[i];
        }
    }       

    {
        rt_uint8_t      max_level[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_MAX_LEVEL,max_level,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            g_device_channel[i].MaxLevel= max_level[i];
        }
    }      
    {
        rt_uint8_t      switch_level[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_SWITCH_LEVEY,switch_level,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            g_device_channel[i].SwitchLevel= switch_level[i];
        }
    }    
    
    {
        rt_uint8_t      on_delay[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_ON_DELAY,on_delay,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            g_device_channel[i].OnDelay= on_delay[i];
        }
    }
    {
        rt_uint8_t      off_delay[MAX_CHANNEL_NUM];
        rt_device_read(eep_device,ADDRESS_OFF_DELAY,off_delay,MAX_CHANNEL_NUM);

        for ( i = 0 ; i <MAX_CHANNEL_NUM; i++ )
        {
            g_device_channel[i].OffDelay= off_delay[i];
        }
    }            

}

void App_LDS_Send_Device_ID(rt_bool_t IsReboot)
{
    rt_uint8_t      buff[8];
    msg_t           msg;
    
    buff[0] = 0xfa;
    buff[1] = DEVICE_CODE;
    buff[2] = g_device_setting_ref.Device_box_num;
    buff[3] = 0xfe;
    if (RT_TRUE == IsReboot)
    {
        buff[4] = 0x80;
    }
    else
    {
        buff[4] = 1;
    }
    buff[5] = 0;
    buff[6] = 0;
    buff[7] = (rt_uint8_t)LDSCheckSum(buff, 7);
    
    rt_ringbuffer_put(&send_buffer_rb,buff , 8);
    msg.data_ptr = &send_buffer_rb;
    msg.data_size = 8;
    rt_mq_send(mq_rs485_snd, &msg, sizeof(msg_t));
}

void App_device_service_key(void *parameter)
{    
    App_LDS_Send_Device_ID(RT_FALSE);
}

void App_device_rst_snd(void *parameter)
{
    App_LDS_Send_Device_ID(RT_TRUE);    
}

void App_Service_setting(void *parameter)
{
    rt_kprintf("setting mode \n");
}    

void App_Service_Reboot(void *parameter)
{    
    rt_kprintf("rebooting \n");
}

void App_CHxKey_Toggle_Output(void *parameter)
{    
    channelDataType *ptr;
    if ( RT_NULL == parameter )
    {
        return;
    }
    ptr = (channelDataType *)parameter;

    if( ptr->TargetLevel >= ptr->SwitchLevel)
    {
        LDSSetChannelTargetLevel(ptr->action, 0, 1);
    }
    else
    {
        LDSSetChannelTargetLevel(ptr->action, 0xff, 1);
    }

}
void APP_LDS_Device_Init(void)
{    
    rt_timer_t  timer;

    eep_device = rt_device_find("EEP");

    if ( RT_NULL == eep_device ) 
    {
        return;
    }

    rt_device_open(eep_device,0);
    rt_device_read(eep_device,0,&g_device_setting_ref,7);
    rt_device_read(eep_device,12,&(g_device_setting_ref.StartDelay),1);

    App_LDS_Device_Channel_Property_Get();
    
    App_LDS_Protocol_Register();

    timer = rt_timer_create("dev_id_snd", 
                            App_device_rst_snd, 
                            RT_NULL, 
                            RT_TICK_PER_SECOND, 
                            RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(timer);

    service_key_fucntion_register(SERVICE_KEY, E_KEY_RELEASE, App_device_service_key,RT_NULL);
    service_key_fucntion_register(SERVICE_KEY, E_KEY_LONG_PRESS, App_Service_Reboot,RT_NULL);
    service_key_fucntion_register((SERVICE_KEY | CH1_KEY), E_KEY_PRESS, App_Service_setting,RT_NULL);
    service_key_fucntion_register(CH1_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[0]);
    service_key_fucntion_register(CH2_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[1]);
    service_key_fucntion_register(CH3_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[2]);
    service_key_fucntion_register(CH4_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[3]);
    service_key_fucntion_register(CH5_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[4]);
    service_key_fucntion_register(CH6_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[5]);
    service_key_fucntion_register(CH7_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[6]);
    service_key_fucntion_register(CH8_KEY, E_KEY_RELEASE, App_CHxKey_Toggle_Output,&g_device_channel[7]);
}

