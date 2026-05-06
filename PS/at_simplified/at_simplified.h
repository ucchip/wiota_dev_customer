#ifndef __AT_SIMPLIFIED_H__
#define __AT_SIMPLIFIED_H__

#define DEFAULT_SUBSYSTEMID 0x21456981 // 默认子系统id
#define DEFAULE_FREQ 502000000         // 默认频率502000000Hz
#define DEFAULE_BW_HZ 200000           // 默认带宽200000Hz
#define DEFAULE_POWER 20               // 默认功率20dbm
#define DEFAULE_RATE_MODE RATE_MODE_4  // 256+mcs2, 2.03kbps

void at_simplified_init(void);

void at_simplified_running(void);
void at_rx_notity(void);
void at_rx_complete(void);

#endif /* __AT_SIMPLIFIED_H__ */