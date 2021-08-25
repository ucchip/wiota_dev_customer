
#ifdef UC8288_MODULE
#ifdef UC8288_FACTORY

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "at.h"

enum factory_command_type
{
    FACTORY_WIOTA = 0, 
    FACTORY_GPIO,
    FACTORY_I2C,
    FACTORY_ADC,
};

static at_result_t at_factory_setup(const char* args)
{
    int argc = 0, type = 0, data = 0;
    const char* req_expr = "=%d,%d";

    argc = at_req_parse_args(args, req_expr, &type, &data);
    if (argc < 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    rt_kprintf("argc = %d, type = %d,data=%d\n", type, data, argc);
    switch(type)
    {
        case FACTORY_WIOTA:
            break;
            
        case FACTORY_GPIO:
            
            break;
        case FACTORY_I2C:
            break;
        case FACTORY_ADC:
            break;
        default:
            return AT_RESULT_REPETITIVE_FAILE;
    }
    
    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+FACTORY", "=<type>,<data>", RT_NULL, RT_NULL, at_factory_setup, RT_NULL);


#endif
#endif
