/******************************************************************************
* Chongqing UCchip InfoTech Co.,Ltd
* Copyright (c) 2022 UCchip
*
* @file    uc_wiota_static.h
* @brief   Static data application program interface.
*
* @author  lujun
* @email   lujun@ucchip.cn
* @data    2022-06-06
* @license ???
******************************************************************************/
#ifndef _UC_WIOTA_STATIC_H_
#define _UC_WIOTA_STATIC_H_
#include "uc_wiota_api.h"

#ifdef __cplushplus
extern "C"
{
#endif

#define FLASH_OPEN_START_ADDRESS     0x0        	// 0
#define FLASH_OPEN_END_ADDRESS       0x7E000        // (512-8)*1024, last 8KB is static info space


/*
* @brief Data transfer unit.
*/
typedef struct DtuInfoT
{
	unsigned char  reserved[2];        /* reserved */
	unsigned char  dtu_status;         /* status: 0 or 1 */
	unsigned char  dtu_at_show;        /* show AT format: 0 or 1 */
	unsigned short dtu_timeout;        /* send timeout */
	unsigned short dtu_wait;           /* wait time */
	unsigned char  dtu_exit[8];        /* exit string */
	unsigned char  na[24];             /* undefined */
} dtu_info_t;


/*
* @brief   Initialize static data.
* @note    Must initialize first.
*/
void uc_wiota_static_data_init(void);

/*
* @brief   Set user id.
* @param   id:  User ID.
* @param   len: Length of user ID.
* @return  0 on success, otherwise 1.
* @note    Length may be 2, 4, 6, 8 bytes.
*/
int uc_wiota_set_userid(unsigned int* id, unsigned char len);

/*
* @brief   Get user id.
* @param   id:  User ID.
* @param   len: Length of user ID.
* @note    Length may be 2, 4, 6, 8 bytes.
*/
void uc_wiota_get_userid(unsigned int* id, unsigned char* len);

/*
* @brief   Get device name.
* @param   name: Device name.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_dev_name(unsigned char* name);

/*
* @brief   Get device serial.
* @param   serial: Device serial.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_dev_serial(unsigned char* serial);

/*
* @brief   Get software version.
* @param   hardware_ver: Software version.
* @note    The maximum length is 16 bytes.
*/
//void uc_wiota_get_software_ver(unsigned char* software_ver);

/*
* @brief   Get manufacture name.
* @param   name: Manufacture name.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_manufacture_name(unsigned char* name);

/*
* @brief   Get hardware version.
* @param   hardware_ver: Hardware version.
* @note    The maximum length is 16 bytes.
*/
void uc_wiota_get_hardware_ver(unsigned char* hardware_ver);

/*
* @brief   Set oscillator flag.
* @param   is_osc: Oscillator flag (0 or 1).
*/
void uc_wiota_set_is_osc(unsigned char is_osc);

/*
* @brief   Get oscillator flag.
* @return  Oscillator flag: 0 or 1.
*/
unsigned char uc_wiota_get_is_osc(void);

/*
* @brief   Get auto connect flag.
* @return  Auto connect flag: 0 or 1.
*/
unsigned char uc_wiota_get_auto_connect_flag(void);

/*
* @brief   Get DTU config.
* @param   cfg: DTU config information.
*/
void uc_wiota_get_dtu_config(dtu_info_t *cfg);

/*
* @brief   Set frequency point list.
* @param   freq_list: Frequency point list.
* @param   num:       Number of frequency point.
* @return  0 on success, otherwise 1.
* @note    The maximum mumber of frequency point is 16.
*/
int uc_wiota_set_freq_list(unsigned char* list, unsigned char num);

/*
* @brief   Get frequency point list.
* @param   freq_list: frequency point list.
*/
void uc_wiota_get_freq_list(unsigned char* list);

/*
* @brief   Get the first address of user defined data.
* @return  The first address of user defined data.
* @note    The maximum length is 256 bytes. If the pointer is out of range,
*          serious consequences will result.
*/
unsigned char* uc_wiota_get_user_info(void);

/*
* @brief   Save static data to flash.
* @param
*/
void uc_wiota_save_static_info();

/*
* @brief   erase 4KB flash, set all bits 1
* @return  result
* @note    flash_addr in 4k, erase this 4K, flash_addr is (no need) the start addr of 4K
*          serious consequences will result.
*/
unsigned int uc_wiota_flash_erase_4K(unsigned int flash_addr);

// write source_addr to dest_addr, without erase
unsigned int uc_wiota_flash_write(unsigned char* data_addr, unsigned int flash_addr, unsigned short length);

// read flash_addr to data_addr
unsigned int uc_wiota_flash_read(unsigned char* data_addr, unsigned int flash_addr, unsigned short length);


#ifdef __cplushplus
}
#endif // !__cplushplus

#endif // !_UC_WIOTA_STATIC_H_
