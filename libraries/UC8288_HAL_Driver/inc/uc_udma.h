
/**
 * @file    udma.h
 * @brief   udma library.
 *
 * Provides udma fuction like configuring udma and using.
 *
 */
#ifndef _UDMA_H_
#define _UDMA_H_

#include <uc_pulpino.h>
#include "uc_event.h"


typedef enum
{
    FIFO_ADDR   = 0,
    MEMORY      = 1,
} CHAN_ADDR_Type;

typedef enum
{
    CHAN0_SOR = (0x1 << 0 ) | (0x0 << 2 ),
    CHAN0_DIS = (0x0 << 0 ) | (0x1 << 2 ),
    CHAN1_SOR = (0x1 << 4 ) | (0x0 << 6 ),
    CHAN1_DIS = (0x0 << 4 ) | (0x1 << 6 ),
    CHAN2_SOR = (0x1 << 8 ) | (0x0 << 10),
    CHAN2_DIS = (0x0 << 8 ) | (0x1 << 10),
    CHAN3_SOR = (0x1 << 12) | (0x0 << 14),
    CHAN3_DIS = (0x0 << 12) | (0x1 << 14)
} CHAN_SELECT_Type;

typedef enum
{
    CHAN0_INT_DISABLE   =   1 << 0,
    CHAN1_INT_DISABLE   =   1 << 1,
    CHAN2_INT_DISABLE   =   1 << 2,
    CHAN3_INT_DISABLE   =   1 << 3,
    CHAN_INT_ENABLE     =   0
} CHAN_INT_DISABLE_Type;

typedef enum
{
    CHAN0_TCIF  =   1 << 0,
    CHAN1_TCIF  =   1 << 1,
    CHAN2_TCIF  =   1 << 2,
    CHAN3_TCIF  =   1 << 3
} uDMA_TCIF_Type;

typedef enum
{
    UDMA_CHAN0_IT_TC    =   0,
    UDMA_CHAN1_IT_TC,
    UDMA_CHAN2_IT_TC,
    UDMA_CHAN3_IT_TC
} uDMA_ITCH_Type;

typedef struct
{
    CHAN_INT_DISABLE_Type   channel_interrupt_mask;
    CHAN_SELECT_Type        channel_chan_select;
} DMA_CFG_Type;



typedef enum
{
    LINK_BD_MODE    =   0x1,
    NORMAL_MODE     =   0x0,
    HANDSHACK_MODE  =   0x2
} DMACHAN_MODE_Type;

typedef struct
{
    FunctionalState   channel_enable     ;
    DMACHAN_MODE_Type channel_mode       ;
    FunctionalState   channel_pause      ;
    FunctionalState   channel_reset      ;
    FunctionalState   channel_endof      ;
    CHAN_ADDR_Type    channel_d_increment;
    CHAN_ADDR_Type    channel_s_increment;
    uint16_t          channel_size       ;
} DMACHAN_CFG_Type;




/**/
#define PARAM_UDMA(x)         (x==UC_DMA)

/**/
#define PARAM_CHAN_ENABLE(x)  ((x==ENABLE)||(x==DISABLE))

/**/
#define PARAM_CHAN_MODE(x)    ((x>=0)&&(x<=3))

/**/
#define PARAM_CHAN_PAUSE(x)   ((x==ENABLE)||(x==DISABLE))

/**/
#define PARAM_CHAN_ENDOF(x)   ((x==ENABLE)||(x==DISABLE))

/**/
#define PARAM_CHAN_RESET(x)   ((x==ENABLE)||(x==DISABLE))

/**/
#define PARAM_CHAN_ADTYPE(x)  ((x==FIFO_ADDR)||(x==MEMORY))

/*udma initialize*/
void udma_init(UDMA_TypeDef* DMA, DMA_CFG_Type* DMA_Config);

/*udma channel initialize*/
void udma_chan_init(UDCHAN_TypeDef* CHANx, DMACHAN_CFG_Type* CHAN_Config);

/*channel source address set*/
void udma_chan_set_source_addr(UDCHAN_TypeDef* CHANx, uint32_t s_addr);

/*channel distination address set*/
void udma_chan_set_destina_addr(UDCHAN_TypeDef* CHANx, uint32_t d_addr);

/*channel link address set*/
void udma_chan_set_link_addr(UDCHAN_TypeDef* CHANx, uint32_t l_addr);

/*clear interrupt handing flag*/
void UDMA_ClearFlag(UDCHAN_TypeDef* CHANx);

/*aquire interrupt handing flag*/
FunctionalState UDMA_GetITState(uDMA_ITCH_Type UdmaChannel);





#endif // _SPI_H_
