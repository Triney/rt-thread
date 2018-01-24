/******************************************************************************

  Copyright (C), 2001-2019, ENIX

 ******************************************************************************
  File Name     : App_LDS_Protocol.h
  Version       : Initial Draft
  Author        : Enix
  Created       : 2017/9/27
  Last Modified :
  Description   : App_LDS_Protocol.c header file
  Function List :
  History       :
  1.Date        : 2017/9/27
    Author      : Enix
    Modification: Created file

******************************************************************************/
#ifndef __APP_LDS_PROTOCOL_H__
#define __APP_LDS_PROTOCOL_H__

#include "Service_rs485.h"
#include "Service_key.h"
#include "App_test_drv.h"
/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
typedef struct DEVICE_SETTING_FLAG
{
    rt_uint16_t     reserved:14;
    rt_uint16_t     is_device_selected:1;
    rt_uint16_t     is_device_EE_BlockOption:1;

    rt_uint16_t     block_ee_address;
}DEVICE_SETTING_FLAG_STRU;

typedef struct DEVICE_PRIORITY_REF
{
    rt_uint8_t      Device_code;
    rt_uint8_t      Device_box_num;
    rt_uint16_t     Firmware_version;

    rt_uint16_t     MaxByteNum;
    rt_uint8_t      MaxPresetNum;
    rt_uint8_t      StartDelay;
    
    rt_uint8_t      PresetOffset;
}DEVICE_PRIORITY_REF_STRU;
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
extern DEVICE_PRIORITY_REF_STRU    g_device_setting_ref;
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
    E_Ctrl_Opcode_Fade_CH_lvl_0_1_sec                   = 0xd3,
    E_Ctrl_Opcode_Fade_CH_lvl_1_sec                     = 0xd2,
    E_Ctrl_Opcode_Fade_CH_lvl_min                       = 0xd1,
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
    E_Opcode_Request_Firmware_version        = 0xba,
    E_Opcode_Request_Physical_Channel_Level  = 0xb8,
    E_Opcode_Set_Physical_Channel_Level      = 0xb7,
    E_Opcode_DM320_dali_cmd                  = 0x6f
}SETTING_OPCODE_ENUM;

typedef enum DIM_DIR
{
    E_DIM_DOWN  = 0,
    E_DIM_UP    = 1,
}DIM_DIR_ENUM;
/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/
#define LDS_COMMAND_MAX_LEN 8
#define MAX_CHANNEL_NUM     12
#define DEVICE_CODE         0x94

#define DIM_LEVEL_MAX       0xFE
#define DIM_LEVEL_MIN       0x00

#define ADDRESS_SWITCH          0x07
#define ADDRESS_DUPLICATE       0x09
#define ADDRESS_START_DELAY     0x0b
#define ADDRESS_START_PRESET    0x0C
#define ADDRESS_AUX_PRESS       0x50
#define ADDRESS_AUX_PRESS_PRESET    0x51
#define ADDRESS_AUX_RELEASE         0x50
#define ADDRESS_AUX_RELEASE_PRESET  0x51
#define ADDRESS_AREA            0x60
#define ADDRESS_AREA_ADDPEND    0x6C
#define ADDRESS_LOGIC_CHANNEL   0x78
#define ADDRESS_MAX_LEVEL       0x84
#define ADDRESS_CURRENT_PRESET  0x90
#define ADDRESS_ON_DELAY        0x9C
#define ADDRESS_OFF_DELAY       0xA8
#define ADDRESS_SWITCH_LEVEY    0xB4
#define ADDRESS_AREA_LINK       0xC0
#define ADDRESS_PRESET_START    0xF0

#define ADDRESS_LAST_SCENSE          ADDRESS_CURRENT_PRESET

#define LDS_TRACE   trace
/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern void App_LDS_Protocol_Register(void);
void App_Service_setting(void *parameter);
void APP_LDS_Device_Init(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __APP_LDS_PROTOCOL_H__ */
