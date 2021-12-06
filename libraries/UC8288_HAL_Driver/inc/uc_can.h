#ifndef _CAN_H_
#define _CAN_H_

#include "uc_pulpino.h"
#include <stdint.h>


#define PARAM_UCAN(x)         (x==CAN_BASE_ADDR)


typedef struct
{
  uint16_t CAN_Prescaler;   /*!< Specifies the length of a time quantum. It ranges from 1 to 1024. */
  
  uint8_t CAN_Mode;         /*!< Specifies the CAN operating mode.
                                 This parameter can be a value of @ref CAN_operating_mode */

  uint8_t CAN_SJW;          /*!< Specifies the maximum number of time quanta the CAN hardware
                                 is allowed to lengthen or shorten a bit to perform resynchronization.
                                 This parameter can be a value of @ref CAN_synchronisation_jump_width */

  uint8_t CAN_BS1;          /*!< Specifies the number of time quanta in Bit Segment 1.
                                 This parameter can be a value of @ref CAN_time_quantum_in_bit_segment_1 */

  uint8_t CAN_BS2;          /*!< Specifies the number of time quanta in Bit Segment 2.
                                 This parameter can be a value of @ref CAN_time_quantum_in_bit_segment_2 */
								 
  FunctionalState CAN_AWUM;  /*!< Enable or disable the automatic wake-up mode. 
                                 This parameter can be set either to ENABLE or DISABLE. */

  FunctionalState CAN_NART;  /*!< Enable or disable the no-automatic retransmission mode.
                                 This parameter can be set either to ENABLE or DISABLE. */

  FunctionalState CAN_RFLM;  /*!< Enable or disable the Receive FIFO Locked mode.
                                 This parameter can be set either to ENABLE or DISABLE. */

} CAN_InitTypeDef;

typedef struct
{
  uint16_t CAN_FilterIdHigh;         /*!< Specifies the filter identification number (MSBs for a 32-bit
                                              configuration, first one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterIdLow;          /*!< Specifies the filter identification number (LSBs for a 32-bit
                                              configuration, second one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterMaskIdHigh;     /*!< Specifies the filter mask number or identification number,
                                              according to the mode (MSBs for a 32-bit configuration,
                                              first one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterMaskIdLow;      /*!< Specifies the filter mask number or identification number,
                                              according to the mode (LSBs for a 32-bit configuration,
                                              second one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterFIFOAssignment; /*!< Specifies the FIFO (0 or 1) which will be assigned to the filter.
                                              This parameter can be a value of @ref CAN_filter_FIFO */
  
  uint8_t CAN_FilterNumber;          /*!< Specifies the filter which will be initialized. It ranges from 0 to 13. */

  uint8_t CAN_FilterMode;            /*!< Specifies the filter mode to be initialized.
                                              This parameter can be a value of @ref CAN_filter_mode */

  uint8_t CAN_FilterScale;           /*!< Specifies the filter scale.
                                              This parameter can be a value of @ref CAN_filter_scale */

  FunctionalState CAN_FilterActivation; /*!< Enable or disable the filter.
                                              This parameter can be set either to ENABLE or DISABLE. */
} CAN_FilterInitTypeDef;

typedef struct
{
  uint32_t StdId;  /*!< Specifies the standard identifier.
                        This parameter can be a value between 0 to 0x7FF. */

  uint32_t ExtId;  /*!< Specifies the extended identifier.
                        This parameter can be a value between 0 to 0x1FFFFFFF. */

  uint8_t IDE;     /*!< Specifies the type of identifier for the message that 
                        will be transmitted. This parameter can be a value 
                        of @ref CAN_identifier_type */

  uint8_t RTR;     /*!< Specifies the type of frame for the message that will 
                        be transmitted. This parameter can be a value of 
                        @ref CAN_remote_transmission_request */

  uint8_t DLC;     /*!< Specifies the length of the frame that will be 
                        transmitted. This parameter can be a value between 
                        0 to 8 */

  uint8_t Data[8]; /*!< Contains the data to be transmitted. It ranges from 0 
                        to 0xFF. */
} CanTxMsg;

typedef struct
{
  uint32_t StdId;  /*!< Specifies the standard identifier.
                        This parameter can be a value between 0 to 0x7FF. */

  uint32_t ExtId;  /*!< Specifies the extended identifier.
                        This parameter can be a value between 0 to 0x1FFFFFFF. */

  uint8_t IDE;     /*!< Specifies the type of identifier for the message that 
                        will be received. This parameter can be a value of 
                        @ref CAN_identifier_type */

  uint8_t RTR;     /*!< Specifies the type of frame for the received message.
                        This parameter can be a value of 
                        @ref CAN_remote_transmission_request */

  uint8_t DLC;     /*!< Specifies the length of the frame that will be received.
                        This parameter can be a value between 0 to 8 */

  uint8_t Data[8]; /*!< Contains the data to be received. It ranges from 0 to 
                        0xFF. */

  uint8_t FMI;     /*!< Specifies the index of the filter the message stored in 
                        the mailbox passes through. This parameter can be a 
                        value between 0 to 0xFF */
} CanRxMsg;

/*!< CAN control and status registers */
/*******************  Bit definition for CAN_MCR register  ********************/
typedef enum{
  CAN_MCR_INRQ      =                  ((uint16_t)0x0001),            /*!< Initialization Request */
  CAN_MCR_SLEEP     =                  ((uint16_t)0x0002),            /*!< Sleep Mode Request */
  CAN_MCR_TXFP      =                  ((uint16_t)0x0004),            /*!< Transmit FIFO Priority */
  CAN_MCR_RFLM      =                  ((uint16_t)0x0008),            /*!< Receive FIFO Locked Mode */
  CAN_MCR_NART      =                  ((uint16_t)0x0010),            /*!< No Automatic Retransmission */
  CAN_MCR_AWUM      =                  ((uint16_t)0x0020),            /*!< Automatic Wakeup Mode */
  CAN_MCR_ABOM      =                  ((uint16_t)0x0040),            /*!< Automatic Bus-Off Management */
  CAN_MCR_TTCM      =                  ((uint16_t)0x0080),            /*!< Time Triggered Communication Mode */
  CAN_MCR_RESET		=				   ((uint16_t)0x8000) 			  /*!< CAN software master reset */
	
}CAN_MCR_Typedef;

/*******************  Bit definition for CAN_MSR register  ********************/
typedef enum{
	
  CAN_MSR_INAK      =                  ((uint16_t)0x0001),            /*!< Initialization Acknowledge */
  CAN_MSR_SLAK      =                  ((uint16_t)0x0002),            /*!< Sleep Acknowledge */
  CAN_MSR_ERRI      =                  ((uint16_t)0x0004),            /*!< Error Interrupt */
  CAN_MSR_WKUI      =                  ((uint16_t)0x0008),            /*!< Wakeup Interrupt */
  CAN_MSR_SLAKI     =                  ((uint16_t)0x0010),            /*!< Sleep Acknowledge Interrupt */
  CAN_MSR_TXM       =                  ((uint16_t)0x0100),            /*!< Transmit Mode */
  CAN_MSR_RXM       =                  ((uint16_t)0x0200),            /*!< Receive Mode */
  CAN_MSR_SAMP      =                  ((uint16_t)0x0400),            /*!< Last Sample Point */
  CAN_MSR_RX        =                  ((uint16_t)0x0800)            /*!< CAN Rx Signal */	
	
}CAN_MSR_Typedef;

/*******************  Bit definition for CAN_TSR register  ********************/
typedef enum{
	
  CAN_TSR_RQCP0         =              ((uint32_t)0x00000001),        /*!< Request Completed Mailbox0 */
  CAN_TSR_TXOK0         =              ((uint32_t)0x00000002),        /*!< Transmission OK of Mailbox0 */
  CAN_TSR_ALST0         =              ((uint32_t)0x00000004),        /*!< Arbitration Lost for Mailbox0 */
  CAN_TSR_TERR0         =              ((uint32_t)0x00000008),        /*!< Transmission Error of Mailbox0 */
  CAN_TSR_ABRQ0         =              ((uint32_t)0x00000080),        /*!< Abort Request for Mailbox0 */
  CAN_TSR_RQCP1         =              ((uint32_t)0x00000100),        /*!< Request Completed Mailbox1 */
  CAN_TSR_TXOK1         =              ((uint32_t)0x00000200),        /*!< Transmission OK of Mailbox1 */
  CAN_TSR_ALST1         =              ((uint32_t)0x00000400),        /*!< Arbitration Lost for Mailbox1 */
  CAN_TSR_TERR1         =              ((uint32_t)0x00000800),        /*!< Transmission Error of Mailbox1 */
  CAN_TSR_ABRQ1         =              ((uint32_t)0x00008000),        /*!< Abort Request for Mailbox 1 */
  CAN_TSR_RQCP2         =              ((uint32_t)0x00010000),        /*!< Request Completed Mailbox2 */
  CAN_TSR_TXOK2         =              ((uint32_t)0x00020000),        /*!< Transmission OK of Mailbox 2 */
  CAN_TSR_ALST2         =              ((uint32_t)0x00040000),        /*!< Arbitration Lost for mailbox 2 */
  CAN_TSR_TERR2         =              ((uint32_t)0x00080000),        /*!< Transmission Error of Mailbox 2 */
  CAN_TSR_ABRQ2         =              ((uint32_t)0x00800000),        /*!< Abort Request for Mailbox 2 */
  CAN_TSR_CODE          =              ((uint32_t)0x03000000),        /*!< Mailbox Code */

  CAN_TSR_TME           =              ((uint32_t)0x1C000000),        /*!< TME[2:0] bits */
  CAN_TSR_TME0          =              ((uint32_t)0x04000000),        /*!< Transmit Mailbox 0 Empty */
  CAN_TSR_TME1          =              ((uint32_t)0x08000000),        /*!< Transmit Mailbox 1 Empty */
  CAN_TSR_TME2          =              ((uint32_t)0x10000000),        /*!< Transmit Mailbox 2 Empty */

  CAN_TSR_LOW           =              ((uint32_t)0xE0000000),        /*!< LOW[2:0] bits */
  CAN_TSR_LOW0          =              ((uint32_t)0x20000000),        /*!< Lowest Priority Flag for Mailbox 0 */
  CAN_TSR_LOW1          =              ((uint32_t)0x40000000),        /*!< Lowest Priority Flag for Mailbox 1 */
  CAN_TSR_LOW2          =              ((uint32_t)0x80000000)        /*!< Lowest Priority Flag for Mailbox 2 */	
	
}CAN_TSR_Typedef;

/*******************  Bit definition for CAN_RF0R register  *******************/
typedef enum{

  CAN_RFR_RCNT      =                 ((uint8_t)0x0F),               /*!< FIFO  Message Pending */
  CAN_RFR_FULL      =                 ((uint8_t)0x10),               /*!< FIFO  Full */
  CAN_RFR_FOVR      =                 ((uint8_t)0x20),               /*!< FIFO  Overrun */
  CAN_RFR_RFOM      =                 ((uint8_t)0x40)               /*!< Release FIFO  Output Mailbox */
	
}CAN_RFR_Typedef;

/********************  Bit definition for CAN_IER register  *******************/
typedef enum{

  CAN_IER_TMEIE       =                ((uint32_t)0x00000001),        /*!< Transmit Mailbox Empty Interrupt Enable */
  CAN_IER_FMPIE       =                ((uint32_t)0x00000002),        /*!< FIFO Message Pending Interrupt Enable */
  CAN_IER_FFIE        =                ((uint32_t)0x00000004),        /*!< FIFO Full Interrupt Enable */
  CAN_IER_FOVIE       =                ((uint32_t)0x00000008),        /*!< FIFO Overrun Interrupt Enable */
  CAN_IER_EWGIE       =                ((uint32_t)0x00000100),        /*!< Error Warning Interrupt Enable */
  CAN_IER_EPVIE       =                ((uint32_t)0x00000200),        /*!< Error Passive Interrupt Enable */
  CAN_IER_BOFIE       =                ((uint32_t)0x00000400),        /*!< Bus-Off Interrupt Enable */
  CAN_IER_LECIE       =                ((uint32_t)0x00000800),        /*!< Last Error Code Interrupt Enable */
  CAN_IER_ERRIE       =                ((uint32_t)0x00008000),        /*!< Error Interrupt Enable */
  CAN_IER_WKUIE       =                ((uint32_t)0x00010000),        /*!< Wakeup Interrupt Enable */
  CAN_IER_SLKIE       =                ((uint32_t)0x00020000)         /*!< Sleep Interrupt Enable */

}CAN_IER_Typedef;

/********************  Bit definition for CAN_ESR register  *******************/
typedef enum{

  CAN_ESR_EWGF        =                ((uint32_t)0x00000001),        /*!< Error Warning Flag */
  CAN_ESR_EPVF        =                ((uint32_t)0x00000002),        /*!< Error Passive Flag */
  CAN_ESR_BOFF        =                ((uint32_t)0x00000004),        /*!< Bus-Off Flag */

  CAN_ESR_LEC         =                ((uint32_t)0x00000070),        /*!< LEC[2:0] bits (Last Error Code) */
  CAN_ESR_LEC_0       =                ((uint32_t)0x00000010),        /*!< Bit 0 */
  CAN_ESR_LEC_1       =                ((uint32_t)0x00000020),        /*!< Bit 1 */
  CAN_ESR_LEC_2       =                ((uint32_t)0x00000040),        /*!< Bit 2 */

  CAN_ESR_TEC         =                ((uint32_t)0x0001FF00),        /*!< Least significant byte of the 9-bit Transmit Error Counter */
  CAN_ESR_REC         =                ((uint32_t)0x1FF00000)         /*!< Receive Error Counter */

}CAN_ESR_Typedef;

/*******************  Bit definition for CAN_BTR register  ********************/
typedef enum{
	
  CAN_BTR_BRP         =                ((uint32_t)0x000003FF),        /*!< Baud Rate Prescaler */
  CAN_BTR_TS1         =                ((uint32_t)0x000F0000),        /*!< Time Segment 1 */
  CAN_BTR_TS2         =                ((uint32_t)0x00700000),        /*!< Time Segment 2 */
  CAN_BTR_SJW         =                ((uint32_t)0x03000000),        /*!< Resynchronization Jump Width */
  CAN_BTR_LBKM        =                ((uint32_t)0x40000000),        /*!< Loop Back Mode (Debug) */
  CAN_BTR_SILM        =                ((uint32_t)0x80000000)         /*!< Silent Mode */
  
}CAN_BTR_Typedef;

/*!< CAN filter registers */
/*******************  Bit definition for CAN_FMR register  ********************/
typedef enum{

  CAN_FMR_FINIT       =                ((uint8_t)0x01)               /*!< Filter Init Mode */

}CAN_FMR_Typedef;

/*******************  Bit definition for CAN_FM1R register  *******************/
typedef enum{

  CAN_FM1R_FBM        =                ((uint16_t)0x3FFF),            /*!< Filter Mode */
  CAN_FM1R_FBM0       =                ((uint16_t)0x0001),            /*!< Filter Init Mode bit 0 */
  CAN_FM1R_FBM1       =                ((uint16_t)0x0002),            /*!< Filter Init Mode bit 1 */
  CAN_FM1R_FBM2       =                ((uint16_t)0x0004),            /*!< Filter Init Mode bit 2 */
  CAN_FM1R_FBM3       =                ((uint16_t)0x0008),            /*!< Filter Init Mode bit 3 */
  CAN_FM1R_FBM4       =                ((uint16_t)0x0010),            /*!< Filter Init Mode bit 4 */
  CAN_FM1R_FBM5       =                ((uint16_t)0x0020),            /*!< Filter Init Mode bit 5 */
  CAN_FM1R_FBM6       =                ((uint16_t)0x0040),            /*!< Filter Init Mode bit 6 */
  CAN_FM1R_FBM7       =                ((uint16_t)0x0080),            /*!< Filter Init Mode bit 7 */
  CAN_FM1R_FBM8       =                ((uint16_t)0x0100),            /*!< Filter Init Mode bit 8 */
  CAN_FM1R_FBM9       =                ((uint16_t)0x0200),            /*!< Filter Init Mode bit 9 */
  CAN_FM1R_FBM10      =                ((uint16_t)0x0400),            /*!< Filter Init Mode bit 10 */
  CAN_FM1R_FBM11      =                ((uint16_t)0x0800),            /*!< Filter Init Mode bit 11 */
  CAN_FM1R_FBM12      =                ((uint16_t)0x1000),            /*!< Filter Init Mode bit 12 */
  CAN_FM1R_FBM13      =                ((uint16_t)0x2000)             /*!< Filter Init Mode bit 13 */

}CAN_FM1R_Typedef;

/*******************  Bit definition for CAN_FS1R register  *******************/
typedef enum{

  CAN_FS1R_FSC       =                 ((uint16_t)0x3FFF),            /*!< Filter Scale Configuration */
  CAN_FS1R_FSC0      =                 ((uint16_t)0x0001),            /*!< Filter Scale Configuration bit 0 */
  CAN_FS1R_FSC1      =                 ((uint16_t)0x0002),            /*!< Filter Scale Configuration bit 1 */
  CAN_FS1R_FSC2      =                 ((uint16_t)0x0004),            /*!< Filter Scale Configuration bit 2 */
  CAN_FS1R_FSC3      =                 ((uint16_t)0x0008),            /*!< Filter Scale Configuration bit 3 */
  CAN_FS1R_FSC4      =                 ((uint16_t)0x0010),            /*!< Filter Scale Configuration bit 4 */
  CAN_FS1R_FSC5      =                 ((uint16_t)0x0020),            /*!< Filter Scale Configuration bit 5 */
  CAN_FS1R_FSC6      =                 ((uint16_t)0x0040),            /*!< Filter Scale Configuration bit 6 */
  CAN_FS1R_FSC7      =                 ((uint16_t)0x0080),            /*!< Filter Scale Configuration bit 7 */
  CAN_FS1R_FSC8      =                 ((uint16_t)0x0100),            /*!< Filter Scale Configuration bit 8 */
  CAN_FS1R_FSC9      =                 ((uint16_t)0x0200),            /*!< Filter Scale Configuration bit 9 */
  CAN_FS1R_FSC10     =                 ((uint16_t)0x0400),            /*!< Filter Scale Configuration bit 10 */
  CAN_FS1R_FSC11     =                 ((uint16_t)0x0800),            /*!< Filter Scale Configuration bit 11 */
  CAN_FS1R_FSC12     =                 ((uint16_t)0x1000),            /*!< Filter Scale Configuration bit 12 */
  CAN_FS1R_FSC13     =                 ((uint16_t)0x2000)             /*!< Filter Scale Configuration bit 13 */
  
}CAN_FS1R_Typedef;

/******************  Bit definition for CAN_FFA1R register  *******************/
typedef enum{

  CAN_FA1R_FACT       =                ((uint16_t)0x3FFF),            /*!< Filter FIFO Assignment */
  CAN_FA1R_FACT0      =                ((uint16_t)0x0001),            /*!< Filter FIFO Assignment for Filter 0 */
  CAN_FA1R_FACT1      =                ((uint16_t)0x0002),            /*!< Filter FIFO Assignment for Filter 1 */
  CAN_FA1R_FACT2      =                ((uint16_t)0x0004),            /*!< Filter FIFO Assignment for Filter 2 */
  CAN_FA1R_FACT3      =                ((uint16_t)0x0008),            /*!< Filter FIFO Assignment for Filter 3 */
  CAN_FA1R_FACT4      =                ((uint16_t)0x0010),            /*!< Filter FIFO Assignment for Filter 4 */
  CAN_FA1R_FACT5      =                ((uint16_t)0x0020),            /*!< Filter FIFO Assignment for Filter 5 */
  CAN_FA1R_FACT6      =                ((uint16_t)0x0040),            /*!< Filter FIFO Assignment for Filter 6 */
  CAN_FA1R_FACT7      =                ((uint16_t)0x0080),            /*!< Filter FIFO Assignment for Filter 7 */
  CAN_FA1R_FACT8      =                ((uint16_t)0x0100),            /*!< Filter FIFO Assignment for Filter 8 */
  CAN_FA1R_FACT9      =                ((uint16_t)0x0200),            /*!< Filter FIFO Assignment for Filter 9 */
  CAN_FA1R_FACT10     =                ((uint16_t)0x0400),            /*!< Filter FIFO Assignment for Filter 10 */
  CAN_FA1R_FACT11     =                ((uint16_t)0x0800),            /*!< Filter FIFO Assignment for Filter 11 */
  CAN_FA1R_FACT12     =                ((uint16_t)0x1000),            /*!< Filter FIFO Assignment for Filter 12 */
  CAN_FA1R_FACT13     =                ((uint16_t)0x2000)             /*!< Filter FIFO Assignment for Filter 13 */
  
}CAN_FA1R_Typedef;

/*******************  Bit definition for CAN_FiRx register  *******************/
typedef enum{

  CAN_FiRx_FB0       =                 ((uint32_t)0x00000001),        /*!< Filter bit 0 */
  CAN_FiRx_FB1       =                 ((uint32_t)0x00000002),        /*!< Filter bit 1 */
  CAN_FiRx_FB2       =                 ((uint32_t)0x00000004),        /*!< Filter bit 2 */
  CAN_FiRx_FB3       =                 ((uint32_t)0x00000008),        /*!< Filter bit 3 */
  CAN_FiRx_FB4       =                 ((uint32_t)0x00000010),        /*!< Filter bit 4 */
  CAN_FiRx_FB5       =                 ((uint32_t)0x00000020),        /*!< Filter bit 5 */
  CAN_FiRx_FB6       =                 ((uint32_t)0x00000040),        /*!< Filter bit 6 */
  CAN_FiRx_FB7       =                 ((uint32_t)0x00000080),        /*!< Filter bit 7 */
  CAN_FiRx_FB8       =                 ((uint32_t)0x00000100),        /*!< Filter bit 8 */
  CAN_FiRx_FB9       =                 ((uint32_t)0x00000200),        /*!< Filter bit 9 */
  CAN_FiRx_FB10      =                 ((uint32_t)0x00000400),        /*!< Filter bit 10 */
  CAN_FiRx_FB11      =                 ((uint32_t)0x00000800),        /*!< Filter bit 11 */
  CAN_FiRx_FB12      =                 ((uint32_t)0x00001000),         /*!< Filter bit 12 */
  CAN_FiRx_FB13      =                 ((uint32_t)0x00002000)         /*!< Filter bit 13 */

}CAN_FiRx_Typedef;

/*!< Mailbox registers */
/******************  Bit definition for CAN_TIxR register  ********************/
typedef enum{

  CAN_TIxR_TXRQ      =                 ((uint32_t)0x00000001),        /*!< Transmit Mailbox Request */
  CAN_TIxR_RTR       =                 ((uint32_t)0x00000002),        /*!< Remote Transmission Request */
  CAN_TIxR_IDE       =                 ((uint32_t)0x00000004),        /*!< Identifier Extension */
  CAN_TIxR_EXID      =                 ((uint32_t)0x001FFFF8),        /*!< Extended Identifier */
  CAN_TIxR_STID      =                 ((uint32_t)0xFFE00000)         /*!< Standard Identifier or Extended Identifier */
  
}CAN_TIxR_Typedef;

/******************  Bit definition for CAN_TDTxR register  *******************/
typedef enum{

  CAN_TDTxR_DLC      =                 ((uint32_t)0x0000000F),        /*!< Data Length Code */
  CAN_TDTxR_TIME     =                 ((uint32_t)0xFFFF0000)         /*!< Message Time Stamp */

}CAN_TDTxR_Typedef;

/******************  Bit definition for CAN_TDLxR register  *******************/
typedef enum{

  CAN_TDLxR_DATA0   =                  ((uint32_t)0x000000FF),        /*!< Data byte 0 */
  CAN_TDLxR_DATA1   =                  ((uint32_t)0x0000FF00),        /*!< Data byte 1 */
  CAN_TDLxR_DATA2   =                  ((uint32_t)0x00FF0000),        /*!< Data byte 2 */
  CAN_TDLxR_DATA3   =                  ((uint32_t)0xFF000000)         /*!< Data byte 3 */

}CAN_TDLxR_Typedef;

/******************  Bit definition for CAN_TDH0R register  *******************/
typedef enum{

  CAN_TDH0R_DATA4   =                  ((uint32_t)0x000000FF),        /*!< Data byte 4 */
  CAN_TDH0R_DATA5   =                  ((uint32_t)0x0000FF00),        /*!< Data byte 5 */
  CAN_TDH0R_DATA6   =                  ((uint32_t)0x00FF0000),        /*!< Data byte 6 */
  CAN_TDH0R_DATA7   =                  ((uint32_t)0xFF000000)         /*!< Data byte 7 */

}CAN_TDHxR_Typedef;

/*******************  Bit definition for CAN_RIR register  *******************/
typedef enum{

  CAN_RIR_RTR      =                  ((uint32_t)0x00000002),        /*!< Remote Transmission Request */
  CAN_RIR_IDE      =                  ((uint32_t)0x00000004),        /*!< Identifier Extension */
  CAN_RIR_EXID     =                  ((uint32_t)0x001FFFF8),        /*!< Extended identifier */
  CAN_RIR_STID     =                  ((uint32_t)0xFFE00000)         /*!< Standard Identifier or Extended Identifier */

}CAN_RIR_Typedef;

/*******************  Bit definition for CAN_RDTR register  ******************/
typedef enum{

  CAN_RDTR_DLC    =                   ((uint32_t)0x0000000F),        /*!< Data Length Code */
  CAN_RDTR_FMI    =                   ((uint32_t)0x0000FF00),        /*!< Filter Match Index */
  CAN_RDTR_TIME   =                   ((uint32_t)0xFFFF0000)         /*!< Message Time Stamp */

}CAN_RDTR_Typedef;

/*******************  Bit definition for CAN_RDLR register  ******************/
typedef enum{

  CAN_RDLR_DATA0    =                 ((uint32_t)0x000000FF),        /*!< Data byte 0 */
  CAN_RDLR_DATA1    =                 ((uint32_t)0x0000FF00),        /*!< Data byte 1 */
  CAN_RDLR_DATA2    =                 ((uint32_t)0x00FF0000),        /*!< Data byte 2 */
  CAN_RDLR_DATA3    =                 ((uint32_t)0xFF000000)         /*!< Data byte 3 */

}CAN_RDLR_Typedef;

/*******************  Bit definition for CAN_RDHR register  ******************/
typedef enum{

  CAN_RDH1R_DATA4     =                ((uint32_t)0x000000FF),        /*!< Data byte 4 */
  CAN_RDH1R_DATA5     =                ((uint32_t)0x0000FF00),        /*!< Data byte 5 */
  CAN_RDH1R_DATA6     =                ((uint32_t)0x00FF0000),        /*!< Data byte 6 */
  CAN_RDH1R_DATA7     =                ((uint32_t)0xFF000000)         /*!< Data byte 7 */

}CAN_RDHR_Typedefe;

typedef enum{
	
	CAN_InitStatus_Failed   =  ((uint8_t)0x00),	/*!< CAN initialization failed */
	CAN_InitStatus_Success  =  ((uint8_t)0x01)	/*!< CAN initialization OK */
	
}CAN_InitStatus_Typedef;

typedef enum{
	
 CAN_Mode_Normal           =   ((uint8_t)0x00),  /*!< normal mode */
 CAN_Mode_LoopBack     	   =   ((uint8_t)0x01),  /*!< loopback mode */
 CAN_Mode_Silent           =   ((uint8_t)0x02),  /*!< silent mode */
 CAN_Mode_Silent_LoopBack  =   ((uint8_t)0x03)   /*!< loopback combined with silent mode */
	
}CAN_Mode_Typedef;

typedef enum{
	
 CAN_OperatingMode_Initialization  =	((uint8_t)0x00), /*!< Initialization mode */
 CAN_OperatingMode_Normal          =	((uint8_t)0x01), /*!< Normal mode */
 CAN_OperatingMode_Sleep           =	((uint8_t)0x02)  /*!< sleep mode */
	
}CAN_OperatingMode_Typedef;

typedef enum{
	
 CAN_ModeStatus_Failed    =	((uint8_t)0x00),   /*!< CAN entering the specific mode failed */
 CAN_ModeStatus_Success   =	((uint8_t)0x01)    /*!< CAN entering the specific mode Succeed */
	
}CAN_ModeStatus_Typedef;

typedef enum{
	
 CAN_SJW_1tq       =          ((uint8_t)0x00),  /*!< 1 time quantum */
 CAN_SJW_2tq       =          ((uint8_t)0x01),  /*!< 2 time quantum */
 CAN_SJW_3tq       =          ((uint8_t)0x02),  /*!< 3 time quantum */
 CAN_SJW_4tq       =          ((uint8_t)0x03)   /*!< 4 time quantum */
 
}CAN_SJW_Typedef;

typedef enum{
	
 CAN_TS1_1tq      =           ((uint8_t)0x00),  /*!< 1 time quantum */
 CAN_TS1_2tq      =           ((uint8_t)0x01),  /*!< 2 time quantum */
 CAN_TS1_3tq      =           ((uint8_t)0x02),  /*!< 3 time quantum */
 CAN_TS1_4tq      =           ((uint8_t)0x03),  /*!< 4 time quantum */
 CAN_TS1_5tq      =           ((uint8_t)0x04),  /*!< 5 time quantum */
 CAN_TS1_6tq      =           ((uint8_t)0x05),  /*!< 6 time quantum */
 CAN_TS1_7tq      =           ((uint8_t)0x06),  /*!< 7 time quantum */
 CAN_TS1_8tq      =           ((uint8_t)0x07),  /*!< 8 time quantum */
 CAN_TS1_9tq      =           ((uint8_t)0x08),  /*!< 9 time quantum */
 CAN_TS1_10tq     =           ((uint8_t)0x09),  /*!< 10 time quantum */
 CAN_TS1_11tq     =           ((uint8_t)0x0A),  /*!< 11 time quantum */
 CAN_TS1_12tq     =           ((uint8_t)0x0B),  /*!< 12 time quantum */
 CAN_TS1_13tq     =           ((uint8_t)0x0C),  /*!< 13 time quantum */
 CAN_TS1_14tq     =           ((uint8_t)0x0D),  /*!< 14 time quantum */
 CAN_TS1_15tq     =           ((uint8_t)0x0E),  /*!< 15 time quantum */
 CAN_TS1_16tq     =           ((uint8_t)0x0F)   /*!< 16 time quantum */	
	
}CAN_TS1_Typedef;

typedef enum{
	
 CAN_TS2_1tq      =           ((uint8_t)0x00),  /*!< 1 time quantum */
 CAN_TS2_2tq      =           ((uint8_t)0x01),  /*!< 2 time quantum */
 CAN_TS2_3tq      =           ((uint8_t)0x02), /*!< 3 time quantum */
 CAN_TS2_4tq      =           ((uint8_t)0x03),  /*!< 4 time quantum */
 CAN_TS2_5tq      =           ((uint8_t)0x04),  /*!< 5 time quantum */
 CAN_TS2_6tq      =           ((uint8_t)0x05),  /*!< 6 time quantum */
 CAN_TS2_7tq      =           ((uint8_t)0x06),  /*!< 7 time quantum */
 CAN_TS2_8tq      =           ((uint8_t)0x07)   /*!< 8 time quantum */

}CAN_TS2_Typedef;

typedef enum{
	
 CAN_FilterMode_IdMask   =    ((uint8_t)0x00),  /*!< identifier/mask mode */
 CAN_FilterMode_IdList   =    ((uint8_t)0x01)   /*!< identifier list mode */
 
}CAN_FilterMode_Typedef;

typedef enum{
	
 CAN_FilterScale_16bit  =     ((uint8_t)0x00), /*!< Two 16-bit filters */
 CAN_FilterScale_32bit  =     ((uint8_t)0x01)  /*!< One 32-bit filter */	
	
}CAN_FilterScale_Typedef;

typedef enum{
	
 CAN_Id_Standard     =        ((uint32_t)0x00000000),  /*!< Standard Id */
 CAN_Id_Extended     =        ((uint32_t)0x00000004)   /*!< Extended Id */	
	
}CAN_Id_Typedef;

typedef enum{
	
 CAN_RTR_Data     =           ((uint32_t)0x00000001),  /*!< Data frame */
 CAN_RTR_Remote   =           ((uint32_t)0x00000003)   /*!< Remote frame */
	
}CAN_RTR_Typedef;

typedef enum{
	
 CAN_Sleep_Failed  =     ((uint8_t)0x00), /*!< CAN did not enter the sleep mode */
 CAN_Sleep_Ok	   =     ((uint8_t)0x01)  /*!< CAN entered the sleep mode */
	
}CAN_Sleep_Typedef;

typedef enum{
	
 CAN_WakeUp_Failed  =     ((uint8_t)0x00), /*!< CAN did not leave the sleep mode */
 CAN_WakeUp_Ok	    =     ((uint8_t)0x01)   /*!< CAN leaved the sleep mode */
	
}CAN_WakeUp_Typedef;


typedef enum{
	
 CAN_TxStatus_Failed    =     ((uint8_t)0x00), /*!< CAN transmission failed */
 CAN_TxStatus_Ok        =     ((uint8_t)0x01), /*!< CAN transmission succeeded */
 CAN_TxStatus_Pending   =     ((uint8_t)0x02), /*!< CAN transmission pending */
 CAN_TxStatus_NoMailBox =     ((uint8_t)0x04)  /*!< CAN cell did not provide an empty mailbox */	
	
}CAN_TxStatus_Typedef;


void CAN_Setup(void);
void  CAN_Init(CAN_CTRL_TypeDef* can_ctrl,CAN_InitTypeDef* can_initstruct);
void CAN_StructInit(CAN_InitTypeDef* can_initstruct);
uint8_t CAN_Transmit(CAN_CTRL_TypeDef* can_ctrl,CAN_TXMailBox_TypeDef* CAN_Tx,CanTxMsg* TxMessage);
void CAN_FilterInit(CAN_FILTER_TypeDef* CAN_Filter,CAN_FilterRegister_TypeDef* CAN_FR,CAN_FilterInitTypeDef* CAN_FilterInitStruct);
uint8_t CAN_TransmitStatus(CAN_CTRL_TypeDef* can_ctrl, uint8_t TransmitMailbox);
void CAN_CancelTransmit(CAN_CTRL_TypeDef* can_ctrl,uint8_t Mailbox);
void CAN_Receive(CAN_CTRL_TypeDef* can_ctrl,CanRxMsg* RxMessage);
uint8_t CAN_MessagePending(CAN_CTRL_TypeDef* can_ctrl);
void CAN_Reset(CAN_CTRL_TypeDef* can_ctrl);

void can_int_enable(CAN_CTRL_TypeDef* can_int);
void can_int_disable(CAN_CTRL_TypeDef* can_int);
void can_sleep(void);
void can_wakeup(void);


#endif
