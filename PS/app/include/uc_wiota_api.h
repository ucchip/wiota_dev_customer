
#ifndef _UC_WIOTA_API_H__
#define _UC_WIOTA_API_H__

#include "uctypes.h"

#define MAX_USER_ID_LEN 8

#define FREQ_MAX_POINT 200
//470M
#define WIOTA_BASE_FREQUENCE 47000
//200 k
#define WIOTA_FREQUENCE_STEP 20
//470M ~ 510M 
#define WIOTA_FREQUENCE_POINT(freqIdx)  (WIOTA_BASE_FREQUENCE + freqIdx  * WIOTA_FREQUENCE_STEP)
//0 - 200
#define WIOTA_FREQUENCE_INDEX(freq)  ((freq - WIOTA_BASE_FREQUENCE)/WIOTA_FREQUENCE_STEP)


typedef enum {
    UC_RECV_MSG = 0,
    UC_RECV_OTA,
    
}UC_RECV_DATA_TYPE;


typedef enum {
    UC_OP_SUC = 0,
    UC_OP_TIMEOUT,
    UC_OP_FAIL,    
}UC_OP_RESULT;

typedef struct 
{
     u32_t         rssi;
     u32_t         ber;
     u32_t         snr;
}t_radio_info;


typedef struct {
        u8_t  sub_sys_u;
        u8_t  id_len;
        u8_t  pn_num;           // 0: 1, 1: 2, 2: 4, 3: not use 
        u8_t  symbol_length;    //128,256,512,1024
        u8_t  dlul_ratio;      //0 1:1,  1 1:2
        u8_t  btvalue;         //bt from rf 1: 0.3, 0: 1.2
        u8_t  group_number;     //frame ul group number: 1,2,4,8
        u8_t  reserved;
        u32_t systemid;
        u32_t subsystemid;
        u8_t  na[48];
}t_sub_system_config;


typedef struct {
    u16_t result;  
}UcSendBack_T,*UcSendBack_P;

typedef struct {
    u8_t type;
    u8_t result;   
    u16_t data_len;
    u8_t* data;
}uc_receive_back_t,*uc_receive_back_p;


typedef struct {
    void *sem;
    u16_t result;   
}uc_send_result_t,*uc_send_result_p;


typedef int (*uc_receive)(uc_receive_back_p recv_data);
typedef void (*uc_send)(UcSendBack_P send_result);

void uc_wiota_set_freq_info(u8_t freqIdx);

int uc_wiota_get_freq_info(void);

int uc_wiota_set_userid(u8_t* id,u8_t id_len);

void uc_wiota_get_userid(u8_t* id,u8_t *id_len);

void uc_wiota_set_system_config(t_sub_system_config *config);

void uc_wiota_init(void);

void uc_wiota_run(void);

void uc_wiota_get_radio_info(t_radio_info *radio);

UC_OP_RESULT uc_wiota_send_data(u8_t* data, u16_t dataLen,  u16_t timeout, uc_send callback);

int uc_wiota_register_recv_data(uc_receive callback);

#endif

