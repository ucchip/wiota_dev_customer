/******************************************************************************
* Chongqing UCchip InfoTech Co.,Ltd
* Copyright (c) 2022 UCchip
*
* @file    custom_data.h
* @brief   Custom data application program interface.
*
* @author  lujun
* @email   lujun@ucchip.cn
* @data    2022-06-06
* @license ???
******************************************************************************/
#ifndef _CUSTOM_DATA_H_
#define _CUSTOM_DATA_H_

/*
* Equipment type definition
*/
#define DEV_TYPE_LIGHT   1
#define DEV_TYPE_SWITCH  2

#define DEV_TYPE_AP      101
#define DEV_TYPE_SERVER  102


#ifdef __cplushplus
extern "C"
{
#endif

/*
* @brief Pair information.
*/
typedef struct PAIR_INFO_T
{
	unsigned char dev_type;            /* type */
	unsigned char index;               /* index */
	unsigned char reserved[2];         /* reserved */
	unsigned int  address;             /* address */
} pair_info_t;

/*
* @brief Custom data.
* @note  Not more than 256 bytes.
*/
typedef struct CUSTOM_DATA_T
{
	unsigned char dev_type;            /* device type */
	unsigned char count;               /* count (1-4) */
	unsigned char reserved[2];         /* reserved */
	unsigned int  multicast_addr[4];   /* multicast address */
	unsigned char pair_dev_type[4][8]; /* pair device type */
	unsigned char pair_index[4][8];    /* pair index */
	unsigned int  pair_address[4][8];  /* pair address */
} custom_data_t;


/*
* @brief   Set device information.
* @param   dev_type: Device type.
* @param   count:    Count.
* @note    Count may be 1-4.
*/
void custom_set_devinfo(unsigned char dev_type, unsigned char count);

/*
* @brief   Get device information.
* @param   dev_type: Device type.
* @param   count:    Count.
* @note    Count may be 1-4.
*/
void custom_get_devinfo(unsigned char* dev_type, unsigned char* count);

/*
* @brief   Set multicast address.
* @param   addr_list: Multicast address list.
* @param   num:       Number of multicast address.
* @return  0 on success, otherwise 1.
* @note    The maximum number of multicast address is 4.
*/
int custom_set_multicast_addr(unsigned int* addr_list, unsigned char num);

/*
* @brief   Get multicast address.
* @param   addr_list: Multicast address list.
* @param   num:       Number of multicast address.
* @note    The maximum number of multicast address is 4.
*/
void custom_get_multicast_addr(unsigned int* addr_list, unsigned char* num);

/*
* @brief   Set pair information.
* @param   index: Pair index.
* @param   pair:  Pair information.
* @param   num:   Number of pairs.
* @return  0 on success, otherwise 1.
* @note    The maximum number of pairs is 8.
*/
int custom_set_pair_list(unsigned char index, pair_info_t* pair, unsigned char num);

/*
* @brief   Get pair information.
* @param   index: Pair index.
* @param   pair: Pair information.
* @param   num:  Number of pairs.
* @return  0 on success, otherwise 1.
* @note    The maximum number of pairs is 8.
*/
int custom_get_pair_list(unsigned char index, pair_info_t* pair, unsigned char* num);

#ifdef __cplushplus
}
#endif // !__cplushplus

#endif // !_CUSTOM_DATA_H_