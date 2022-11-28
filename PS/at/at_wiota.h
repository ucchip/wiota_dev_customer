
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

#ifdef GATEWAY_MODE_SUPPORT
boolean at_gateway_get_reboot(void);
void at_gateway_set_reboot(boolean reboot_flag);
unsigned int at_gateway_get_wiota_id(void);
void at_gateway_set_wiota_id(unsigned int wiota_id);
void at_gateway_release_sem(void);
#endif
#endif
