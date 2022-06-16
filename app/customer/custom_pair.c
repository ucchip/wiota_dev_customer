#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "custom_pair.h"
#include "light_ctrl.h"
#include "switch_ctrl.h"
#include "uc_wiota_static.h"

static pair_info_t *p_light_pair_info = NULL;
static unsigned char g_light_pair_info_count = 0;

static pair_info_t *p_switch_pair_info[SWITCH_COUNT_MAX];
static unsigned char g_switch_pair_info_count[SWITCH_COUNT_MAX];

void custom_light_pair_info_init(void)
{
    p_light_pair_info = NULL;
    g_light_pair_info_count = 0;

    p_light_pair_info = rt_malloc(8 * sizeof(pair_info_t));
    if (p_light_pair_info == NULL)
    {
        return;
    }
    custom_get_pair_list(0, p_light_pair_info, &g_light_pair_info_count);
    if (g_light_pair_info_count == 0)
    {
        if (p_light_pair_info != NULL)
        {
            rt_free(p_light_pair_info);
            p_light_pair_info = NULL;
        }
    }
}

void custom_clear_light_pair_info(void)
{
    rt_kprintf("custom_clear_light_pair_info start\r\n");
    if (p_light_pair_info != NULL)
    {
        rt_free(p_light_pair_info);
        p_light_pair_info = NULL;
    }
    g_light_pair_info_count = 0;

    custom_set_pair_list(0, NULL, 0);
    
    uc_wiota_save_static_info(0);
    rt_kprintf("custom_clear_light_pair_info over\r\n");
}

int custom_set_light_pair_info(pair_info_t *pair_info, unsigned int pair_info_count)
{
    int result = 0;

    custom_clear_light_pair_info();
    rt_kprintf("custom_set_light_pair_info pair_info_count = %d\r\n", pair_info_count);

    p_light_pair_info = rt_malloc(pair_info_count * sizeof(pair_info_t));
    if (p_light_pair_info == NULL)
    {
        result = -1;
        goto __err;
    }
    memcpy(p_light_pair_info, pair_info, pair_info_count * sizeof(pair_info_t));
    g_light_pair_info_count = pair_info_count;

    custom_set_pair_list(0, p_light_pair_info, g_light_pair_info_count);
    
    uc_wiota_save_static_info(0);
    rt_kprintf("custom_set_light_pair_info over\r\n");

    return 0;

__err:
    custom_clear_light_pair_info();

    return result;
}

int custom_check_light_pair_info(pair_info_t *pair_info)
{
    unsigned int index = 0;

    for (index = 0; index < g_light_pair_info_count; index++)
    {
        if ((pair_info->address == p_light_pair_info[index].address)
            && (pair_info->dev_type == p_light_pair_info[index].dev_type)
            && (pair_info->index == p_light_pair_info[index].index))
        {
            return 0;
        }
    }

    return -1;
}

int custom_compare_light_pair_info(pair_info_t *pair_info, unsigned int pair_info_count)
{
    unsigned int index = 0;

    if (pair_info_count != g_light_pair_info_count)
    {
        return -1;
    }

    for (index = 0; index < g_light_pair_info_count; index++)
    {
        if (custom_check_light_pair_info(&pair_info[index]) != 0)
        {
            return -2;
        }
    }

    return 0;
}

unsigned int custom_get_light_pair_info_count(void)
{
    return g_light_pair_info_count;
}

int custom_get_light_pair_info(unsigned int pair_info_index, pair_info_t *pair_info)
{
    if (pair_info_index >= g_light_pair_info_count)
    {
        return -1;
    }

    memcpy(pair_info, &p_light_pair_info[pair_info_index], sizeof(pair_info_t));

    return 0;
}

void custom_switch_pair_info_init(void)
{
    for (unsigned char index = 0; index < switch_get_count(); index++)
    {
        p_switch_pair_info[index] = NULL;
        g_switch_pair_info_count[index] = 0;

        p_switch_pair_info[index] = rt_malloc(8 * sizeof(pair_info_t));
        if (p_switch_pair_info[index] == NULL)
        {
            return;
        }
        custom_get_pair_list(index, p_switch_pair_info[index], &g_switch_pair_info_count[index]);
        if (g_switch_pair_info_count[index] == 0)
        {
            if (p_switch_pair_info[index] != NULL)
            {
                rt_free(p_switch_pair_info[index]);
                p_switch_pair_info[index] = NULL;
            }
        }
    }
}

void custom_clear_switch_pair_info(unsigned char sw_index)
{
    rt_kprintf("custom_clear_switch_pair_info start\r\n");
    if (sw_index >= switch_get_count())
    {
        return;
    }
    if (p_switch_pair_info[sw_index] != NULL)
    {
        rt_free(p_switch_pair_info[sw_index]);
        p_switch_pair_info[sw_index] = NULL;
    }
    g_switch_pair_info_count[sw_index] = 0;

    custom_set_pair_list(sw_index, NULL, 0);
    
    uc_wiota_save_static_info(0);
    rt_kprintf("custom_clear_switch_pair_info over\r\n");
}

int custom_set_switch_pair_info(unsigned char sw_index, pair_info_t *pair_info, unsigned int pair_info_count)
{
    int result = 0;

    rt_kprintf("custom_set_switch_pair_info sw_index = %d, pair_info_count = %d\r\n", sw_index, pair_info_count);
    if (sw_index >= switch_get_count())
    {
        return -1;
    }
    custom_clear_switch_pair_info(sw_index);

    p_switch_pair_info[sw_index] = rt_malloc(pair_info_count * sizeof(pair_info_t));
    if (p_switch_pair_info[sw_index] == NULL)
    {
        result = -1;
        goto __err;
    }
    memcpy(p_switch_pair_info[sw_index], pair_info, pair_info_count * sizeof(pair_info_t));
    g_switch_pair_info_count[sw_index] = pair_info_count;

    custom_set_pair_list(sw_index, p_switch_pair_info[sw_index], g_switch_pair_info_count[sw_index]);
    
    uc_wiota_save_static_info(0);
    rt_kprintf("custom_set_switch_pair_info over\r\n");

    return 0;

__err:
    custom_clear_switch_pair_info(sw_index);

    return result;
}

int custom_check_switch_pair_info(unsigned char sw_index, pair_info_t *pair_info)
{
    unsigned int index = 0;

    if (sw_index >= switch_get_count())
    {
        return -1;
    }
    for (index = 0; index < g_switch_pair_info_count[sw_index]; index++)
    {
        if ((pair_info->address == p_switch_pair_info[sw_index][index].address)
            && (pair_info->dev_type == p_switch_pair_info[sw_index][index].dev_type)
            && (pair_info->index == p_switch_pair_info[sw_index][index].index))
        {
            return 0;
        }
    }

    return -1;
}

int custom_compare_switch_pair_info(unsigned char sw_index, pair_info_t *pair_info, unsigned int pair_info_count)
{
    unsigned int index = 0;

    if (sw_index >= switch_get_count())
    {
        return -1;
    }
    if (pair_info_count != g_switch_pair_info_count[sw_index])
    {
        return -1;
    }

    for (index = 0; index < g_switch_pair_info_count[sw_index]; index++)
    {
        if (custom_check_light_pair_info(&pair_info[index]) != 0)
        {
            return -2;
        }
    }

    return 0;
}

unsigned int custom_get_switch_pair_info_count(unsigned char sw_index)
{
    if (sw_index >= switch_get_count())
    {
        return 0;
    }
    return g_switch_pair_info_count[sw_index];
}

int custom_get_switch_pair_info(unsigned char sw_index, unsigned int pair_info_index, pair_info_t *pair_info)
{
    if (sw_index >= switch_get_count())
    {
        return -1;
    }
    if (pair_info_index >= g_switch_pair_info_count[sw_index])
    {
        return -1;
    }

    memcpy(pair_info, &p_switch_pair_info[sw_index][pair_info_index], sizeof(pair_info_t));

    return 0;
}

unsigned int custom_get_switch_pair_light_address(unsigned char sw_index, unsigned int *get_light_addr, unsigned int light_max_count)
{
    unsigned int get_count = 0;

    if (sw_index >= switch_get_count())
    {
        return 0;
    }
    if (light_max_count == 0)
    {
        return 0;
    }

    for (unsigned int index = 0; index < g_switch_pair_info_count[sw_index]; index++)
    {
        if (p_switch_pair_info[sw_index][index].dev_type == DEV_TYPE_LIGHT)
        {
            get_light_addr[get_count++] = p_switch_pair_info[sw_index][index].address;
            if (get_count >= light_max_count)
            {
                break;
            }
        }
    }

    return get_count;
}

#endif
