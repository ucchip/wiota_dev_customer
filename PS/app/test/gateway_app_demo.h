#ifndef __GATEWAY_APP_DEMO__
#define __GATEWAY_APP_DEMO__

typedef void (*send_result_callback)(int result);

typedef struct
{
    void *data;
    int data_len;
    int timeout;
    send_result_callback callback; // according to the result, handle resend,etc.
} user_ul_data_t;

void gateway_app_demo_init(void);

int gateway_app_send_data(user_ul_data_t *ul_data); // you can also directly use the uc_wiota_gateway_send_data interface

#endif