#ifndef _MANAGER_LIST_H_
#define _MANAGER_LIST_H_

typedef struct manager_list
{
    void *data;
    struct manager_list *next;
    struct manager_list *pre;
} t_manager_list;

void init_manager_list(t_manager_list *freq_list);

void add_manager_list(t_manager_list *freq_list, t_manager_list *node);

void clean_manager_list(t_manager_list *freq_list);

#endif
