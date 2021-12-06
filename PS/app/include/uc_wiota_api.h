
#ifndef _UC_WIOTA_API_H__
#define _UC_WIOTA_API_H__

typedef unsigned long long  u64_t;
typedef signed long long  s64_t;
typedef unsigned long  ul32_t;
typedef signed long  sl32_t;
typedef signed int  s32_t;
typedef unsigned int  u32_t;
typedef signed short  s16_t;
typedef unsigned short  u16_t;
typedef char n8_t;
typedef signed char  s8_t;
typedef unsigned char  u8_t;
typedef unsigned char boolean;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

#define MAX_USER_ID_LEN 8

//200 k
#define WIOTA_FREQUENCE_STEP 20
//470M ~ 510M
#define WIOTA_FREQUENCE_POINT(freq_idx,base_freq)  (base_freq + freq_idx * WIOTA_FREQUENCE_STEP)
//0 - 200
#define WIOTA_FREQUENCE_INDEX(freq,base_freq)  ((freq - base_freq)/WIOTA_FREQUENCE_STEP)

#define CRC32_LEN 4


typedef enum {
    UC_RECV_MSG = 0,    // normal msg from ap
    UC_RECV_BC,         // broadcast msg from ap
    UC_RECV_OTA,        // ota msg from ap
    UC_RECV_SCAN_FREQ,  // result of freq scan by riscv
    UC_RECV_SYNC_LOST,  // sync lost notify by riscv, need scan freq
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
     unsigned char         rssi;        // absolute value, 0~150, always negative
     unsigned char         ber;
     signed char           snr;
     signed char           power;
}radio_info_t;


typedef struct {
        unsigned char   ap_max_pow;         // 21, 30,
        unsigned char   id_len;           // 0: 2, 1: 4, 2: 6, 3: 8
        unsigned char   pn_num;           // 0: 1, 1: 2, 2: 4, 3: not use
        unsigned char   symbol_length;    //128,256,512,1024
        unsigned char   dlul_ratio;       // 0 1:1,  1 1:2
        unsigned char   btvalue;          //bt from rf 1: 0.3, 0: 1.2
        unsigned char   group_number;     //frame ul group number: 0,1,2,3: 1,2,4,8
        unsigned char   spectrum_idx;      //default 3, 470M~510M;
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
    signed char     snr;
    signed char     rssi;
    unsigned char   is_synced;
}uc_freq_scan_result_t,*uc_freq_scan_result_p;


typedef void (*uc_recv)(uc_recv_back_p recv_data);
typedef void (*uc_send)(uc_send_back_p send_result);

void uc_wiota_get_version(u8_t *version, u8_t *time);

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

void uc_wiota_scan_freq(unsigned char* data, unsigned short len, unsigned int timeout, uc_recv callback, uc_recv_back_p recv_result);

void uc_wiota_flash_backup_init(void);

void uc_wiota_set_is_gating(boolean is_gating);

#endif
