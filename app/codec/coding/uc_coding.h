#ifndef _CODING_H_
#define _CODING_H_

typedef enum
{
    PRO_RESERVED = 0,
    PRO_COMPRESS_FLAG = 1,
    PRO_SEGMENT_FLAG = 2,
    PRO_RESPONSE_FLAG = 3,
    PRO_NEED_RES = 4,
    PRO_PACKET_NUM = 5,
    PRO_DEST_ADDR = 6,
    PRO_SRC_ADDR = 7,
} header_property_e;

typedef struct
{
    unsigned char reserved : 1;
    unsigned char compress_flag : 1;
    unsigned char segment_flag : 1;
    unsigned char response_flag : 1;
    unsigned char is_need_res : 1;
    unsigned char is_packet_num : 1;
    unsigned char is_dest_addr : 1;
    unsigned char is_src_addr : 1;
} app_ps_property_t;

typedef struct
{
    unsigned int src_addr;
    unsigned int dest_addr;
} app_ps_addr_t;

typedef struct
{
    unsigned char total_num;   /* 0 ~ 255 */
    unsigned char current_num; /* 0 ~ 255 */
} app_ps_segment_info_t;

typedef struct
{
    app_ps_property_t property;
    app_ps_addr_t addr;
    unsigned char packet_num; /* 0 ~ 255 */
    app_ps_segment_info_t segment_info;
    unsigned char cmd_type; /* 0 ~ 255 */
} app_ps_header_t;

#define APP_MAX_CODING_DATA_LEN (1024 - sizeof(app_ps_header_t))
#define APP_MAX_DECODING_DATA_LEN (1024 - sizeof(app_ps_header_t))

unsigned char app_packet_num(void);

/*********************************************************************************
 This function is to set property of app ps header

 param:
        in:
            bit : header_property_e, 0 ~ 7
            value : 0 or 1
        out:
            ps_property : header property
 return:NULL.
**********************************************************************************/
void app_set_header_property(header_property_e bit, unsigned char value, app_ps_property_t *ps_property);

/*********************************************************************************
 This function is to coding app data

 param:
        in:
            ps_header : ps header, user fill
            input_data : data segment
            input_data_len : length of data segment
        out:
            output_data : encoded data, need manual release after use
            output_data_len : length of encoded data
 return:
        0: coding suc
        other value: coding failed
**********************************************************************************/
int app_data_coding(app_ps_header_t *ps_header,
                    unsigned char *input_data,
                    unsigned int input_data_len,
                    unsigned char **output_data,
                    unsigned int *output_data_len);

/*********************************************************************************
 This function is to decoding app data

 param:
        in:
            input_data : data segment
            input_data_len : length of data segment
        out:
            output_data : decoded data, need manual release after use
            output_data_len : length of decoded data
            ps_header : decoded ps header
 return:
        0: decoding suc
        other value: decoding failed
**********************************************************************************/
int app_data_decoding(unsigned char *input_data,
                      unsigned int input_data_len,
                      unsigned char **output_data,
                      unsigned int *output_data_len,
                      app_ps_header_t *ps_header);
#endif
