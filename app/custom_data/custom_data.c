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

#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include "uc_wiota_static.h"
#include "custom_data.h"


/**
 * @brief  get device ID
 *
 * @return the device ID
 */
unsigned int custom_get_device_id(void)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    return custom_data->dev_id;
}

/**
 * @brief  set device information
 *
 * @param  dev_type the device type
 * @param  count the count of pair device
 * @note   the maximum count of pair device is 4
 */
void custom_set_devinfo(unsigned char dev_type, unsigned char count)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // set device type
    custom_data->dev_type = dev_type;
    // set count
    custom_data->count = count;
}

/**
 * @brief   get device information
 *
 * @param  dev_type the device type
 * @param  count the count of pair device
 * @note   the maximum count of pair device is 4
 */
void custom_get_devinfo(unsigned char *dev_type, unsigned char *count)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // Get device type.
    *dev_type = custom_data->dev_type;
    // Get count.
    *count = custom_data->count;
}

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
int custom_set_multicast_addr(unsigned int *addr_list, unsigned char num)
{
    custom_data_t *custom_data;
    if (num > 4)
    {
        rt_kprintf("error num! num = %d\n", num);
        return 1;
    }
    custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // clear multicast address
    rt_memset(custom_data->multicast_addr, 0, sizeof(unsigned int) * 4);
    // set multicast address
    if (num > 0)
        rt_memcpy(custom_data->multicast_addr, addr_list, sizeof(unsigned int) * num);
    return 0;
}

/**
 * @brief  get multicast address
 *
 * @param  addr_list the multicast address list
 * @param  num the number of multicast address
 * @note   the maximum number of multicast address is 4
 */
void custom_get_multicast_addr(unsigned int *addr_list, unsigned char *num)
{
    custom_data_t *custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // get multicast address
    rt_memcpy(addr_list, custom_data->multicast_addr, sizeof(unsigned int) * 4);
    // calculate the number of multicast address list
    *num = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (addr_list[i] == 0 || addr_list[i] == 0xFFFFFFFF)
            break;
        *num += 1;
    }
}

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
int custom_set_pair_list(unsigned char index, pair_info_t *pair, unsigned char num)
{
    custom_data_t *custom_data;
    if (index >= 4 || num > 8)
    {
        rt_kprintf("error index or num! index = %d, num = %d\n", index, num);
        return 1;
    }
    custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // clear pair information
    rt_memset(custom_data->pair_dev_type[index], 0, 8);
    rt_memset(custom_data->pair_index[index], 0, 8);
    rt_memset(custom_data->pair_address[index], 0, sizeof(unsigned int) * 8);
    // set pair information
    for (unsigned char i = 0; i < num; ++i)
    {
        custom_data->pair_dev_type[index][i] = pair[i].dev_type;
        custom_data->pair_index[index][i] = pair[i].index;
        custom_data->pair_address[index][i] = pair[i].address;
    }
    return 0;
}

/**
 * @brief  get pair information
 * @param  index the pair index (0-3)
 * @param  pair the pair information
 * @param  num the number of pair
 * @return 0: if successful
 *         !0: otherwise
 * @note   the maximum number of pairs is 8
 */
int custom_get_pair_list(unsigned char index, pair_info_t *pair, unsigned char *num)
{
    custom_data_t *custom_data;
    if (index >= 4)
    {
        rt_kprintf("error index! index = %d", index);
        return 1;
    }
    custom_data = (custom_data_t *)uc_wiota_get_user_info();
    // get pair information
    *num = 0;
    for (unsigned char i = 0; i < 8; ++i)
    {
        if (custom_data->pair_address[index][i] == 0 || custom_data->pair_address[index][i] == 0xFFFFFFFF)
            break;
        pair[i].dev_type = custom_data->pair_dev_type[index][i];
        pair[i].index = custom_data->pair_index[index][i];
        pair[i].address = custom_data->pair_address[index][i];
        *num += 1;
    }
    return 0;
}

#endif // WIOTA_APP_DEMO
