#ifndef _SWITCH_CTRL_H_
#define _SWITCH_CTRL_H_

#define SWITCH_COUNT_MAX 5

typedef enum sw_state
{
    SW_UP = 0,
    SW_DOWN = 1,
} e_sw_state;

void switch_ctrl_init(void);
unsigned char switch_get_count(void);
int switch_get_state(unsigned char index);
int switch_get_all_state(unsigned char *get_state_mask);
unsigned char switch_get_down_event_mask(void);

#endif
