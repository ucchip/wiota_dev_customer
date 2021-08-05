#ifndef		_CTRL_CMD_H_
#define		_CTRL_CMD_H_
//#include    "typedefs.h"
//ctrl cmd exchange between stack and
//client software.
typedef enum
{
    SET_TRACE_ACTIVE = 0x1,
    SET_FILTER_MASK  = 0x2,
    SET_ACTIVE_LOG   = 0x3,
    SET_CMD_TEST     = 0x4,
    POWER_ON         = 0x80,
    POWER_OFF        = 0x81,
    LOCK_ARFCN       = 0x82,
    LOCK_BAND        = 0x83,
    SIM_SHOW         = 0x84,
    SUB_MOD_SHOW     = 0x85,
    IP_SHOW          = 0x86,
    SIB_SHOW         = 0x87,
    PDP_SHOW         = 0x88,
    STATES_SHOW      = 0x89,
    MEAS_SHOW        = 0x8a,
    GMM_SHOW         = 0x8b,
    SM_SHOW          = 0x8c,
    MM_SHOW          = 0x8d,
    RR_SHOW          = 0x8e,
    GRR_SHOW         = 0x8f,
    SYSTEM_SHOW      = 0x90,
} UC_LOG_CTRL_CMD_TYPE;

typedef struct
{
    unsigned char    cmd_type;
    unsigned char    cmd_data[8];
}UC_LOG_CTRL_CMD_FORMAT,LOG_CTRL,TracealyzerCommandType;
//}UC_LOG_CTRL_CMD_FORMAT;
//typedef UC_LOG_CTRL_CMD_FORMAT LOG_CTRL;
//typedef UC_LOG_CTRL_CMD_FORMAT TracealyzerCommandType;

void  proc_act_log_list(unsigned char* pData);
void  proc_act_all_log();
void  proc_disact_all_log();

//void  disc_log_monitor();//uart detect that client software disconnect from ps.
//void  log_cmd_proc(LOG_CTRL *ptr_Ctrl_data);

void  proc_disact_log_list(unsigned int* pData);

#endif
