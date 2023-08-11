#ifndef _AT_WIOTA_GPIO_REPORT_H
#define _AT_WIOTA_GPIO_REPORT_H

enum _e_wiota_gpio_mode
{
    WIOTA_MODE_OUT_UART = 0,
    WIOTA_MODE_STORE,
};

int wiota_gpio_mode_get(void);
void wiota_gpio_mode_set(int mode);
void wiota_data_insert(uc_recv_back_t *data);

#endif // ~#ifndef _AT_WIOTA_GPIO_REPORT_H

