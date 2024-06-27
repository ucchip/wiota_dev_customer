#ifndef __UC_SPIM_API_H__
#define __UC_SPIM_API_H__

void uc_spim_init(void);

void uc_spim_read(unsigned int remote_addr, unsigned char *buf, unsigned int buf_len);

void uc_spim_write(unsigned int remote_addr, unsigned char *buf, unsigned int buf_len);

void spim_wr_sample(void);

#endif // __UC_SPIM_API_H__