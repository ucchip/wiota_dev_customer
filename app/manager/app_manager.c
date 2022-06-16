#include <rtthread.h>
#ifdef WIOTA_APP_DEMO
#include <rtdevice.h>
#include "manager_queue.h"
#include "app_manager.h"
#include "custom_manager.h"
#include "app_manager_logic.h"
#include "app_manager_freq.h"
#include "app_manager_cfg.h"

static int manager_thread_create_task(void **thread,
                                      char *name, void (*entry)(void *parameter),
                                      void *parameter, unsigned int stack_size,
                                      unsigned char priority,
                                      unsigned int tick)
{
    *thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack || RT_NULL == *thread)
    {
        return 1;
    }

    if (RT_EOK != rt_thread_init(*thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}

static void app_manager_task(void *pPara)
{
    manager_config_init();
    manager_send_lock_init();
    manager_recv_package_list_init();
    rt_kprintf("app manager task enter\n");
    while (1)
    {
        // wiota freq, frequency point strategy.
        app_manager_freq();

        // recv queue from send/recv or bussiness.
        app_manager_logic();

        // wiota exit
        app_manager_exit_wiota();
    }
}

void app_manager_enter(void)
{
    void *app_manager_handle = NULL;
    void *app_custom_handle = NULL;

    rt_kprintf("app manager enter\n");
    app_manager_create_logic_queue();

    // create wiota app manager task. wiota business logic management.
    if (0 != manager_thread_create_task(&app_manager_handle, "app_manager", app_manager_task, NULL, 512, 3, 3))
    {
        rt_kprintf("manager_thread_create_task error\n");
        return;
    }

    custom_manager_create_queue();

    if (0 != manager_thread_create_task(&app_custom_handle, "cusom_manag", custom_manager_task, NULL, 512, 4, 3))
    {
        rt_kprintf("custom_manager_task create error\n");
        return;
    }

    // start tasks
    rt_thread_startup((rt_thread_t)app_manager_handle);
    rt_thread_startup((rt_thread_t)app_custom_handle);
}

#endif
