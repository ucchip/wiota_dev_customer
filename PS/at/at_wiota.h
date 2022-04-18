
#ifndef __AT_WIOTA_H__
#define __AT_WIOTA_H__

enum at_wiota_state
{
    AT_WIOTA_DEFAULT = 0,
    AT_WIOTA_INIT,
    AT_WIOTA_RUN,
    AT_WIOTA_EXIT,
};

void at_wiota_set_state(int state);
int at_wiota_get_state(void);

#endif


