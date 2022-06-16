#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include <board.h>
#include "manager_list.h"

void init_manager_list(t_manager_list *freq_list)
{
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}

void add_manager_list(t_manager_list *freq_list, t_manager_list *node)
{
    //t_manager_list *tmp = freq_list->next;

    rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

    if (freq_list->next == freq_list)
    {
        freq_list->next = node;
        freq_list->pre = node;
        node->next = freq_list;
        node->pre = freq_list;
        //rt_kprintf("%s line %d node 0x%x, freq_list->next 0x%x\n", __FUNCTION__, __LINE__, node, freq_list->next);
    }
    else
    {
        node->next = freq_list->next;
        node->pre = freq_list;
        freq_list->next->pre = node;
        freq_list->next = node;
        //rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);
    }
}

void clean_manager_list(t_manager_list *freq_list)
{
    t_manager_list *op_node = freq_list->next;
    while (op_node != freq_list)
    {
        t_manager_list *tmp = op_node;
        op_node = op_node->next;
        rt_free(tmp);
        tmp = RT_NULL;
    }
    freq_list->next = freq_list;
    freq_list->pre = freq_list;
}
#endif