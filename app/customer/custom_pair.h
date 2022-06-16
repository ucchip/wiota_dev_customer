#ifndef _CUSTOM_PAIR_H_
#define _CUSTOM_PAIR_H_

#include "custom_data.h"


void custom_light_pair_info_init(void);
void custom_clear_light_pair_info(void);
int custom_set_light_pair_info(pair_info_t *pair_info, unsigned int pair_info_count);
int custom_check_light_pair_info(pair_info_t *pair_info);
int custom_compare_light_pair_info(pair_info_t *pair_info, unsigned int pair_info_count);
unsigned int custom_get_light_pair_info_count(void);
int custom_get_light_pair_info(unsigned int pair_info_index, pair_info_t *pair_info);

void custom_switch_pair_info_init(void);
void custom_clear_switch_pair_info(unsigned char sw_index);
int custom_set_switch_pair_info(unsigned char sw_index, pair_info_t *pair_info, unsigned int pair_info_count);
int custom_check_switch_pair_info(unsigned char sw_index, pair_info_t *pair_info);
int custom_compare_switch_pair_info(unsigned char sw_index, pair_info_t *pair_info, unsigned int pair_info_count);
unsigned int custom_get_switch_pair_info_count(unsigned char sw_index);
int custom_get_switch_pair_info(unsigned char sw_index, unsigned int pair_info_index, pair_info_t *pair_info);
unsigned int custom_get_switch_pair_light_address(unsigned char sw_index, unsigned int *get_light_addr, unsigned int light_max_count);

#endif
