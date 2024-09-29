#ifndef _UC_OTA_FLASH_H_
#define _UC_OTA_FLASH_H_

int uc_wiota_ota_flash_read(unsigned int offset, unsigned char *buf, unsigned int size);
void uc_wiota_ota_flash_write(unsigned char *data_buf, unsigned int flash_addr, unsigned int length);
void uc_wiota_ota_flash_erase(unsigned int start_addr, unsigned int erase_size);
int uc_wiota_ota_check_flash_data(unsigned int flash_addr, unsigned int flash_size, char *md5);
void uc_wiota_ota_jump_program(unsigned int file_size, unsigned char upgrade_type);

#endif