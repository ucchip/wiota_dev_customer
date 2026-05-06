#include <rtthread.h>
#include <rtdevice.h>
#include "wiota_app.h"
#include "uc_example_app.h"
#include "uc_wiota_static.h"
#ifdef _AT_SIMPLIFIED_
#include "at_simplified.h"
#endif

static int memory_show(void)
{
    rt_size_t total;
    rt_size_t used;
    rt_size_t max_used;

    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("total %d used %d maxused %d\n", total, used, max_used);

    return 0;
}

int main(void)
{
    uc_wiota_static_data_init(); // must call and place first!!!

    uc_peripheral_example();
    wiota_app_init();

    unsigned int count = 0;
    while (1)
    {
#ifdef _AT_SIMPLIFIED_
        at_simplified_running();
#else
        rt_thread_mdelay(2000);
#endif
        if (count++ % 4 == 0)
        {
            memory_show();
        }
    }
    return 0;
}
