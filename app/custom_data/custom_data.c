/******************************************************************************
* Chongqing UCchip InfoTech Co.,Ltd
* Copyright (c) 2022 UCchip
*
* @file    custom_data.c
* @brief   Custom data application program interface.
*
* @author  lujun
* @email   lujun@ucchip.cn
* @data    2022-06-06
* @license ???
******************************************************************************/
#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <string.h>
#include "uc_wiota_static.h"
#include "custom_data.h"


// Set device information.
void custom_set_devinfo(unsigned char dev_type, unsigned char count)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Set device type.
    custom_data->dev_type = dev_type;
    // Set count.
    custom_data->count = count;
}

// Get device information.
void custom_get_devinfo(unsigned char* dev_type, unsigned char* count)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Get device type.
    *dev_type = custom_data->dev_type;
    // Get count.
    *count = custom_data->count;
}

// Set multicast address.
int custom_set_multicast_addr(unsigned int* addr_list, unsigned char num)
{
    custom_data_t* custom_data;
    if (num > 4)
    {
        rt_kprintf("error num! num = %d\n", num);
        return 1;
    }
    custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Clear multicast address.
    memset(custom_data->multicast_addr, 0, sizeof(unsigned int) * 4);
    // Set multicast address.
    if (num > 0)
        memcpy(custom_data->multicast_addr, addr_list, sizeof(unsigned int) * num);
    return 0;
}

// Get multicast address.
void custom_get_multicast_addr(unsigned int* addr_list, unsigned char* num)
{
    custom_data_t* custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Get multicast address.
    memcpy(addr_list, custom_data->multicast_addr, sizeof(unsigned int) * 4);
    // Calculate the number of multicast address list.
    *num = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (addr_list[i] == 0 || addr_list[i] == 0xFFFFFFFF)
            break;
        *num += 1;
    }
}

// Set pair information.
int custom_set_pair_list(unsigned char index, pair_info_t* pair, unsigned char num)
{
    custom_data_t* custom_data;
    if (index >= 4 || num > 8)
    {
        rt_kprintf("error index or num! index = %d, num = %d\n", index, num);
        return 1;
    }
    custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Clear pair information.
    memset(custom_data->pair_dev_type[index], 0, 8);
    memset(custom_data->pair_index[index],    0, 8);
    memset(custom_data->pair_address[index],  0, sizeof(unsigned int) * 8);
    // Set pair information.
    for (unsigned char i = 0; i < num; ++i)
    {
        custom_data->pair_dev_type[index][i] = pair[i].dev_type;
        custom_data->pair_index[index][i]    = pair[i].index;
        custom_data->pair_address[index][i]  = pair[i].address;
    }
    return 0;
}

// Get pair information.
int custom_get_pair_list(unsigned char index, pair_info_t* pair, unsigned char* num)
{
    custom_data_t* custom_data;
    if (index >= 4)
    {
        rt_kprintf("error index! index = %d", index);
        return 1;
    }
    custom_data = (custom_data_t*)uc_wiota_get_user_info();
    // Get pair information.
    *num = 0;
    for (unsigned char i = 0; i < 8; ++i)
    {
        if (custom_data->pair_address[index][i] == 0 || custom_data->pair_address[index][i] == 0xFFFFFFFF)
            break;
        pair[i].dev_type = custom_data->pair_dev_type[index][i];
        pair[i].index    = custom_data->pair_index[index][i];
        pair[i].address  = custom_data->pair_address[index][i];
        *num += 1;
    }
    return 0;
}

#endif // WIOTA_APP_DEMO