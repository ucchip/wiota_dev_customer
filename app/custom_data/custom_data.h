/*
 * Copyright (c) 2022, Chongqing UCchip InfoTech Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief Custom data application program interface.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-06     Lujun        the first version
 * 2022-08-03     Lujun        replace memcpy and memset with rt_memcpy and rt_memset
 */

#ifndef _CUSTOM_DATA_H_
#define _CUSTOM_DATA_H_

/**
 * equipment type definition
 */
#define DEV_TYPE_LIGHT   1
#define DEV_TYPE_SWITCH  2

#define DEV_TYPE_AP      101
#define DEV_TYPE_SERVER  102


#ifdef __cplushplus
extern "C"
{
#endif

/**
 * @brief pair information
 *
 */
typedef struct PairInfoT
{
    unsigned char  dev_type;           /**< type */
    unsigned char  index;              /**< index */
    unsigned char  reserved[2];        /**< reserved */
    unsigned int   address;            /**< address */
} pair_info_t;

/**
 * @brief custom data
 *
 * @note  not more than 256 bytes
 */
typedef struct CustomDataT
{
    unsigned int   dev_id;             /**< device ID */
    unsigned char  dev_type;           /**< device type */
    unsigned char  count;              /**< count (0-4) */
    unsigned char  reserved[2];        /**< reserved */
    unsigned int   multicast_addr[4];  /**< multicast address */
    unsigned char  pair_dev_type[4][8];/**< pair device type */
    unsigned char  pair_index[4][8];   /**< pair index */
    unsigned int   pair_address[4][8]; /**< pair address */
} custom_data_t;


/**
 * @brief  get device ID
 *
 * @return the device ID
 */
unsigned int custom_get_device_id(void);

/**
 * @brief  set device information
 *
 * @param  dev_type the device type
 * @param  count the count of pair device
 * @note   the maximum count of pair device is 4
 */
void custom_set_devinfo(unsigned char dev_type, unsigned char count);

/**
 * @brief   get device information
 *
 * @param  dev_type the device type
 * @param  count the count of pair device
 * @note   the maximum count of pair device is 4
 */
void custom_get_devinfo(unsigned char *dev_type, unsigned char *count);

/**
 * @brief   set multicast address
 *
 * @param   addr_list the multicast address list
 * @param   num the number of multicast address
 * @return  0: if successfull
 *          !0: otherwise
 * @note    clear the multicast address list when 'num' is 0
 *          the maximum number of multicast address is 4
 */
int custom_set_multicast_addr(unsigned int *addr_list, unsigned char num);

/**
 * @brief  get multicast address
 *
 * @param  addr_list the multicast address list
 * @param  num the number of multicast address
 * @note   the maximum number of multicast address is 4
 */
void custom_get_multicast_addr(unsigned int *addr_list, unsigned char *num);

/**
 * @brief  set pair information
 *
 * @param  index the pair index (0-3)
 * @param  pair the pair information
 * @param  num the number of pair
 * @return 0: if successful
 *         !0: otherwise
 * @note   the maximum number of pairs is 8
 */
int custom_set_pair_list(unsigned char index, pair_info_t *pair, unsigned char num);

/**
 * @brief  get pair information.
 * @param  index the pair index (0-3)
 * @param  pair the pair information
 * @param  num the number of pair
 * @return 0: if successful
 *         !0: otherwise
 * @note   the maximum number of pairs is 8
 */
int custom_get_pair_list(unsigned char index, pair_info_t *pair, unsigned char *num);

#ifdef __cplushplus
}
#endif // __cplushplus

#endif // _CUSTOM_DATA_H_
