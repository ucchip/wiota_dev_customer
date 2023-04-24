/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief Static data application program interface.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-06     Lujun        the first version
 * 2022-07-04     Zhujiejing   add flash read/write interface
 * 2022-08-03     Lujun        replace memcpy and memset with rt_memcpy and rt_memset
 */

#ifndef _UC_WIOTA_STATIC_H_
#define _UC_WIOTA_STATIC_H_

#ifdef __cplushplus
extern "C"
{
#endif

#define FLASH_OPEN_START_ADDRESS     0x0        	// 0
#define FLASH_OPEN_END_ADDRESS       0x7E000        // (512-8)*1024, last 8KB is static info space


/**
 * @brief data transfer unit.
 *
 */
typedef struct DtuInfoT
{
    unsigned char  reserved[2];        /**< reserved */
    unsigned char  dtu_status;         /**< status: 0 or 1 */
    unsigned char  dtu_at_show;        /**< show AT format: 0 or 1 */
    unsigned short dtu_timeout;        /**< send timeout */
    unsigned short dtu_wait;           /**< wait time */
    unsigned char  dtu_exit[8];        /**< exit string */
    unsigned char  na[24];             /**< undefined */
} dtu_info_t;


/**
 * @brief  initialize static data
 *
 * @note   must initialize first
 */
void uc_wiota_static_data_init(void);

void uc_wiota_set_dev_addr(unsigned int dev_addr);

/**
 * @brief  set user ID
 *
 * @param  id the user ID
 * @param  len the length of user ID
 * @return 0: if seccessful
 *         !0: otherwise
 * @note   the length may be 2, 4, 6, 8 bytes
 */
int uc_wiota_set_userid(unsigned int *id, unsigned char len);

/**
 * @brief  get user ID
 *
 * @param  id the user ID
 * @param  len the length of user ID
 * @note   the length may be 2, 4, 6, 8 bytes
 */
void uc_wiota_get_userid(unsigned int *id, unsigned char *len);

/**
 * @brief  get device name
 *
 * @param  name the device name
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_dev_name(unsigned char *name);

/**
 * @brief  get device serial
 *
 * @param  serial the device serial
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_dev_serial(unsigned char *serial);

/**
 * @brief  get software version
 *
 * @param  software_ver the software version
 * @note   the maximum length is 16 bytes
 */
// void uc_wiota_get_software_ver(unsigned char *software_ver);

/**
 * @brief  get manufacture name
 *
 * @param  name: Manufacture name
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_manufacture_name(unsigned char *name);

/**
 * @brief  get hardware version
 *
 * @param  hardware_ver the hardware version
 * @note   the maximum length is 16 bytes
 */
void uc_wiota_get_hardware_ver(unsigned char *hardware_ver);

/**
 * @brief  set oscillator flag
 *
 * @param  is_osc: the oscillator flag (0 or 1)
 */
void uc_wiota_set_is_osc(unsigned char is_osc);

/**
 * @brief  get oscillator flag
 *
 * @return the oscillator flag (0 or 1)
 */
unsigned char uc_wiota_get_is_osc(void);

/**
 * @brief  get auto connect flag
 *
 * @return the auto connect flag (0 or 1)
 */
unsigned char uc_wiota_get_auto_connect_flag(void);

/**
 * @brief  get DTU config
 *
 * @param  cfg the DTU config information
 */
void uc_wiota_get_dtu_config(dtu_info_t *cfg);

/**
 * @brief  set frequency point list
 *
 * @param  freq_list the frequency point list
 * @param  num the number of frequency point
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum mumber of frequency point is 16.
 */
int uc_wiota_set_freq_list(unsigned char *freq_list, unsigned char num);

/**
 * @brief  set subsystemid list
 *
 * @param  subsystemid list
 * @param  num the number of subsystemid
 * @return 0: if successfull
 *         !0: otherwise
 * @note   the maximum mumber of subsystemid is 6.
 */
void uc_wiota_get_subsystemid_list(unsigned int *subsystemid_list);

/**
 * @brief  get frequency point list
 *
 * @param  freq_list the frequency point list
 */
void uc_wiota_get_freq_list(unsigned char *freq_list);

/**
 * @brief  get the first address of user defined data
 *
 * @return the first address of user defined data
 * @note   the maximum length is 256 bytes
 */
unsigned char* uc_wiota_get_user_info(void);

/**
 * @brief  save static data to flash
 *
 */
void uc_wiota_save_static_info(void);

/**
 * @brief  erase 4KB flash with 0xFF
 *
 * @param  flash_addr the flash address
 * @return 0: if successfull
 *         !0: otherwise
 */
unsigned int uc_wiota_flash_erase_4K(unsigned int flash_addr);

/**
 * @brief  write flash without erase
 *
 * @param  data_addr the write data
 * @param  flash_addr the flash address
 * @param  length the length of write data
 * @return 0: if successfull
 *         !0: otherwise
 */
unsigned int uc_wiota_flash_write(unsigned char *data_addr, unsigned int flash_addr, unsigned short length);

/**
 * @brief  read flash
 *
 * @param  data_addr the read data
 * @param  flash_addr the flash address
 * @param  length the length of read data
 * @return 0: if successfull
 *         !0: otherwise
 */
unsigned int uc_wiota_flash_read(unsigned char *data_addr, unsigned int flash_addr, unsigned short length);

void uc_wiota_set_download_file_size(int filesize);

#ifdef __cplushplus
}
#endif // __cplushplus

#endif // _UC_WIOTA_STATIC_H_
