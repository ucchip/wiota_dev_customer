
#ifndef __AT_WIOTA_H__
#define __AT_WIOTA_H__
#include "uc_wiota_api.h"
typedef enum
{
    AT_WIOTA_DEFAULT = 0,
    AT_WIOTA_INIT,
    AT_WIOTA_RUN,
    AT_WIOTA_EXIT,
} at_wiota_state_e;

void at_wiota_set_state(at_wiota_state_e state);
at_wiota_state_e at_wiota_get_state(void);
void at_wiota_awaken_notice(void);
void at_wiota_manager(void);

#ifdef _QUICK_CONNECT_
void wiota_recv_callback(uc_recv_back_p data);
#endif
#endif
