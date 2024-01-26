
#ifndef __AT_WIOTA_H__
#define __AT_WIOTA_H__

typedef enum
{
    AT_WIOTA_DEFAULT = 0,
    AT_WIOTA_INIT,
    AT_WIOTA_RUN,
    AT_WIOTA_EXIT,
} at_wiota_state_e;

void at_wiota_set_state(at_wiota_state_e state);
at_wiota_state_e at_wiota_get_state(void);

#endif
