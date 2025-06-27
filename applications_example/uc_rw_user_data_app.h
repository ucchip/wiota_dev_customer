#ifdef APP_EXAMPLE_RW

#ifndef __UC_RW_USER_DATA_APP_H__
#define __UC_RW_USER_DATA_APP_H__

typedef struct user_config_data
{
    unsigned char flag;
    unsigned int mode;
} user_config_data_t;

void uc_static_data_example(void);
void uc_set_user_mode(unsigned int mode);
unsigned int uc_get_user_mode(void);

#endif

#endif /* APP_EXAMPLE_RW */