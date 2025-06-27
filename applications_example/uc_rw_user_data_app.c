#include <rtthread.h>
#include "uc_wiota_static.h"
#include "uc_rw_user_data_app.h"

#ifdef APP_EXAMPLE_RW

#define DEFAULT_DATA_FLAG 0x0A

static user_config_data_t *g_config = RT_NULL;
//set user static data default value interface
static void user_cfg_set_def(void)
{
    user_config_data_t default_user_data =
        {
            .flag = DEFAULT_DATA_FLAG,
            .mode = 1,
        };
    rt_memcpy(g_config, &default_user_data, sizeof(default_user_data));
}

//init interface
static void user_config_init(void)
{
    //gain user static data head addr
    g_config = (user_config_data_t *)uc_wiota_get_user_info();

    if (RT_NULL == g_config)
    {
        rt_kprintf("static abnormal,reboot\n");
        //static abnormal,reboot
        extern void rt_hw_cpu_reset();
        rt_hw_cpu_reset();
    }

    //judege whether there default flag value
    if (g_config->flag != DEFAULT_DATA_FLAG)
    {
        rt_kprintf("set default user data\n");
        //restore default static data
        user_cfg_set_def();
        //save static data to flash
        uc_wiota_save_static_info();
    }
}

//get user static data interface
unsigned int uc_get_user_mode(void)
{
    return g_config->mode;
}

//set user static data interface
void uc_set_user_mode(unsigned int mode)
{
    g_config->mode = mode;
    //save static data to flash
    uc_wiota_save_static_info();
}

//demo,include init,get,set
void uc_static_data_example(void)
{
    user_config_data_t set_config = {0};
    set_config.mode = 0;
    user_config_init();                //init inerface
    uc_get_user_mode();                //get user static data mode
    uc_set_user_mode(set_config.mode); //set user static data mode
}

#endif
