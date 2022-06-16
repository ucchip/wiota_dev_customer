#ifndef _CUSTOMER_MANAGER_H_
#define _CUSTOMER_MANAGER_H_

#include "light_ctrl.h"

#define CUSTOM_REPORT_STATE_CYCLE_TIME      120000

enum custom_logic_cmd
{
    CUSTOM_LOGIC_REPORT_DEVSTATE = 0,
};

typedef struct app_costom_pro
{
    int next_process;
} t_costom_manager;

int custom_manager_create_queue(void);
int manager_sendqueue_custom(int src_task, int cmd, void *data);
void custom_manager_task(void *pPara);

void custom_report_light_property(void);
void custom_report_light_state(void);
void custom_request_light_pair(void);
void custom_cancel_light_pair(void);

#endif
