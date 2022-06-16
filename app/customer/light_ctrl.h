#ifndef _LIGHT_CTRL_H_
#define _LIGHT_CTRL_H_

typedef enum light_state
{
    LIGHT_OFF = 0,
    LIGHT_ON = 1,
} e_light_state;

typedef enum light_ctrl
{
    CTRL_LIGHT_OFF = 0,
    CTRL_LIGHT_ON = 1,
    CTRL_LIGHT_REVERSE = 2,
} e_light_ctrl;

void light_ctrl_init(void);
void light_ctrl_output(e_light_ctrl ctrl_out);
e_light_state light_ctrl_get_state(void);

#endif
