/**
 * @file    udma.c
 * @brief   udma library.
 *
 * Provides udma fuction like configuring udma and using.
 *
 */

#include "uc_udma.h"

/*Macro Definition*/




/*Definition*/




/*Static Variable*/

/*Global Variable*/

/*Static Fuction*/



/*********************************************************
 * Function name    :void udma_init(UDMA_TypeDef *DMA,
 *                              DMA_CFG_Type *DMA_Config)
 * Description      :udma initialize function
 * Input Parameter  :UDMA_TypeDef *DMA       :select uDMA
 *                   DMA_CFG_Type *DMA_Config:configurating dma struct
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-21
 *********************************************************/
void udma_init(UDMA_TypeDef* DMA, DMA_CFG_Type* DMA_Config)
{
    /*select udma*/
//    PARAM_UDMA(DMA);

    /*select udma channel*/
    //PARAM_DMA_CHAN(DMA_Config->channel_chan_select);

    /*enable channelx interrupt*/
    DMA->INTMASK = DMA_Config->channel_interrupt_mask;

    /*select udma peripheral*/
    DMA->DMAPSR = DMA_Config->channel_chan_select;
}


/*********************************************************
 * Function name    :void udma_chan_init(UDCHAN_TypeDef *CHANx,
                                    DMACHAN_CFG_Type *CHAN_Config)
 * Description      :udma channel configurating
 * Input Parameter  :UDMA_TypeDef CHANx       :select uDMA channel(0~3)
 *                                             or channel link
 *                   DMA_CFG_Type *CHAN_Config:configurating dma struct
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-21
 *********************************************************/
void udma_chan_init(UDCHAN_TypeDef* CHANx, DMACHAN_CFG_Type* CHAN_Config)
{

//    /*channel enable*/
//    PARAM_CHAN_ENABLE(CHAN_Config->channel_enable);
//
//    /*channel pause tramsfer*/
//    PARAM_CHAN_PAUSE(CHAN_Config->channel_pause);
//
//    /*channel mode select*/
//    PARAM_CHAN_MODE(CHAN_Config->channel_mode);
//
//    /*channel reset*/
//    PARAM_CHAN_RESET(CHAN_Config->channel_reset);
//
//    /*end tramsfer*/
//    PARAM_CHAN_ENDOF(CHAN_Config->channel_endof);
//
//    /*destination increase*/
//    PARAM_CHAN_ADTYPE(CHAN_Config->channel_d_increment);
//
//    /*source increase*/
//    PARAM_CHAN_ADTYPE(CHAN_Config->channel_s_increment);
//
//    /**/
//    uint32_t temp = CHAN_Config->channel_size | CHAN_Config->channel_s_increment << 18;

    /*configuration csr*/
    CHANx->CHANCSR = CHAN_Config->channel_size
                     | CHAN_Config->channel_s_increment << 18
                     | CHAN_Config->channel_d_increment << 19
                     | CHAN_Config->channel_endof << 20
                     | CHAN_Config->channel_mode << 24
                     | CHAN_Config->channel_pause << 26
                     | CHAN_Config->channel_enable << 27
                     | CHAN_Config->channel_reset << 28;
}

/*********************************************************
 * Function name    :void udma_chan_set_source_addr(UDCHAN_TypeDef *CHANx,
                                                    uint32_t s_addr)
 * Description      :Set udma source address
 * Input Parameter  :UDMA_TypeDef CHANx      :select uDMA channel(0~3)
 *                   uint32_t s_addr         :Source_addr
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-21
 *********************************************************/
void udma_chan_set_source_addr(UDCHAN_TypeDef* CHANx, uint32_t s_addr)
{
    CHANx->CHANSAR = s_addr;
}

/*********************************************************
 * Function name    :void udma_chan_set_destina_addr(UDCHAN_TypeDef *CHANx,
                                                    uint32_t d_addr)
 * Description      :Set udma destination address
 * Input Parameter  :UDMA_TypeDef CHANx      :select uDMA channel(0~3)
 *                   uint32_t d_addr         :Destination_addr
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-21
 *********************************************************/
void udma_chan_set_destina_addr(UDCHAN_TypeDef* CHANx, uint32_t d_addr)
{
    CHANx->CHANDAR = d_addr;
}

/*********************************************************
 * Function name    :void udma_chan_set_link_addr(UDCHAN_TypeDef *CHANx,
                                                    uint32_t )
 * Description      :Set udma link address
 * Input Parameter  :UDMA_TypeDef CHANx      :select uDMA channel(0~3)
 *                   uint32_t l_addr         :Destination_addr
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-21
 *********************************************************/
void udma_chan_set_link_addr(UDCHAN_TypeDef* CHANx, uint32_t l_addr)
{
    CHANx->CHANNLD = (UDCHAN_TypeDef *)l_addr;
}



/*********************************************************
 * Function name    :void UDMA_ClearFlag(UDCHAN_TypeDef*CHANx)
 * Description      :Clear udma interrupt handle flag
 * Input Parameter  :UDMA_TypeDef CHANx      :select uDMA channel(0~3)
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-21
 *********************************************************/
void UDMA_ClearFlag(UDCHAN_TypeDef* CHANx)
{


    /*clear flag of interrupt*/
    CHANx->CHANCSR |= 1 << 12;
    /*clear udma interrupt pending*/
    ICP |= 1 << 3;
}

/*********************************************************
 * Function name    :FunctionalState UDMA_GetITState(
 *                              uDMA_ITCH_Type UdmaChannel)
 * Description      :aquire udma interrupt handle flag
 * Input Parameter  :UDMA_TypeDef UdmaChannel   :select uDMA interrupt flag
 * Outout Parameter :FuctionaState state        :interrupt state
 * Return           :FuctionaState state
 * Author           :yjxiong
 * establish time   :2021-04-22
 *********************************************************/
FunctionalState UDMA_GetITState(uDMA_ITCH_Type UdmaChannel)
{

    FunctionalState state   =   DISABLE;

    if (UdmaChannel ==  UDMA_CHAN0_IT_TC)
    {
        if (UC_DMA->INTSTATE ==  CHAN0_TCIF)
        {
            state = ENABLE;
        }
    }
    else if (UdmaChannel ==  UDMA_CHAN1_IT_TC)
    {
        if (UC_DMA->INTSTATE ==  CHAN1_TCIF)
        {
            state = ENABLE;
        }
    }
    else if (UdmaChannel ==  UDMA_CHAN2_IT_TC)
    {
        if (UC_DMA->INTSTATE ==  CHAN2_TCIF)
        {
            state = ENABLE;
        }
    }
    else if (UdmaChannel ==  UDMA_CHAN3_IT_TC)
    {
        if (UC_DMA->INTSTATE ==  CHAN3_TCIF)
        {
            state = ENABLE;
        }
    }

    return state;
}



