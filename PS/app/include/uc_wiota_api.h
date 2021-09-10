
#ifndef _UC_WIOTA_API_H__
#define _UC_WIOTA_API_H__

#define MAX_USER_ID_LEN 8

//200 k
#define WIOTA_FREQUENCE_STEP 20
//470M ~ 510M 
#define WIOTA_FREQUENCE_POINT(freq_idx,base_freq)  (base_freq + freq_idx * WIOTA_FREQUENCE_STEP)
//0 - 200
#define WIOTA_FREQUENCE_INDEX(freq,base_freq)  ((freq - base_freq)/WIOTA_FREQUENCE_STEP)


typedef enum {
    UC_RECV_MSG = 0,
    UC_RECV_BC,
    UC_RECV_OTA,
    UC_RECV_SCAN_FREQ,
}UC_RECV_DATA_TYPE;


typedef enum {
    UC_STATUS_NULL = 0,
    UC_STATUS_SYNC,
    UC_STATUS_SLEEP,
    UC_STATUS_ERROR,
}UC_WIOTA_STATUS;


typedef enum {
    UC_OP_SUCC = 0,
    UC_OP_TIMEOUT,
    UC_OP_FAIL,    
}UC_OP_RESULT;


typedef struct {
     unsigned int          rssi;
     unsigned int          ber;
     char                  snr;
     char                  power;
}radio_info_t;


typedef struct {
        unsigned char   apMaxPow;         // 21, 30,
        unsigned char   id_len;           // 0: 2, 1: 4, 2: 6, 3: 8 
        unsigned char   pn_num;           // 0: 1, 1: 2, 2: 4, 3: not use 
        unsigned char   symbol_length;    //128,256,512,1024
        unsigned char   dlul_ratio;       // 0 1:1,  1 1:2
        unsigned char   btvalue;          //bt from rf 1: 0.3, 0: 1.2
        unsigned char   group_number;     //frame ul group number: 0,1,2,3: 1,2,4,8
        unsigned char   spectrumIdx;      //default 3, 470M~510M;
        unsigned int    systemid;
        unsigned int    subsystemid;
        unsigned char   na[48];
}sub_system_config_t;


typedef struct {
    unsigned short result;  
}uc_send_back_t,*uc_send_back_p;


typedef struct {
    void *sem;
    unsigned short result;   
}uc_send_result_t,*uc_send_result_p;


typedef struct {
    unsigned char   result;   
    unsigned char   type;
    unsigned short  data_len;
    unsigned char * data;
}uc_recv_back_t,*uc_recv_back_p;


typedef struct {
    void *sem;
    unsigned char   result;  
    unsigned char   type;
    unsigned short  data_len;
    unsigned char * data;
}uc_recv_result_t,*uc_recv_result_p;


typedef struct {
    unsigned char   freq_idx;  
}uc_freq_scan_req_t,*uc_freq_scan_req_p;


typedef struct {
    unsigned char   freq_idx;  
    char            snr;
    short           rssi;
}uc_freq_scan_result_t,*uc_freq_scan_result_p;


typedef void (*uc_recv)(uc_recv_back_p recv_data);
typedef void (*uc_send)(uc_send_back_p send_result);


void uc_wiota_init(void);

void uc_wiota_run(void);

void uc_wiota_exit(void);

void uc_wiota_connect(void);

void uc_wiota_disconnect(void);

UC_WIOTA_STATUS uc_wiota_get_state(void);

void uc_wiota_set_dcxo(unsigned int  dcxo);

void uc_wiota_set_freq_info(unsigned char freq_idx);

unsigned char uc_wiota_get_freq_info(void);

int uc_wiota_set_userid(unsigned int * id,unsigned char id_len);

void uc_wiota_get_userid(unsigned int * id,unsigned char *id_len);

void uc_wiota_set_activetime(unsigned int active_s);

unsigned int uc_wiota_get_activetime(void);

void uc_wiota_set_system_config(sub_system_config_t *config);

void uc_wiota_get_system_config(sub_system_config_t *config);

void uc_wiota_get_radio_info(radio_info_t *radio);

UC_OP_RESULT uc_wiota_send_data(unsigned char* data, unsigned short len, unsigned short timeout, uc_send callback);

void uc_wiota_recv_data(uc_recv_back_p recv_result, unsigned short timeout, uc_recv callback);

void uc_wiota_register_recv_data(uc_recv callback);

void uc_wiota_scan_freq(unsigned char* data, unsigned short len, unsigned short timeout, uc_recv callback, uc_recv_back_p recv_result);

#endif

