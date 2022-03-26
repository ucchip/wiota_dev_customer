#include "rtconfig.h"
#include "uc_event.h"
#include "uc_gpio.h"
#include "uc_can.h"

#ifdef RT_USING_CAN
#define MCR_DBF      ((uint32_t)0x00010000) /* software master reset */

/* CAN Filter Master Register bits */
#define FMR_FINIT    ((uint32_t)0x00000001) /* Filter init mode */

/* CAN Mailbox Transmit Request */
#define TMIDxR_TXRQ  ((uint32_t)0x00000001) /* Transmit mailbox request */

/* Time out for INAK bit */
#define INAK_TIMEOUT        ((uint32_t)0x0000FFFF)

/* Flags in TSR register */
#define CAN_FLAGS_TSR              ((uint32_t)0x08000000) 
/* Flags in RF1R register */
#define CAN_FLAGS_RF1R             ((uint32_t)0x04000000) 
/* Flags in RF0R register */
#define CAN_FLAGS_RF0R             ((uint32_t)0x02000000) 
/* Flags in MSR register */
#define CAN_FLAGS_MSR              ((uint32_t)0x01000000) 
/* Flags in ESR register */
#define CAN_FLAGS_ESR              ((uint32_t)0x00F00000) 

/* Mailboxes definition */
#define CAN_TXMAILBOX_0                   ((uint8_t)0x00)
#define CAN_TXMAILBOX_1                   ((uint8_t)0x01)
#define CAN_TXMAILBOX_2                   ((uint8_t)0x02) 

void CAN_Setup(void)
{
	gpio_set_pin_direction(UC_GPIO,GPIO_PIN_10,GPIO_DIR_IN);
	gpio_set_pin_mux(UC_GPIO_CFG,GPIO_PIN_10,GPIO_FUNC_2);
	
	gpio_set_pin_direction(UC_GPIO,GPIO_PIN_11,GPIO_DIR_OUT);
	gpio_set_pin_mux(UC_GPIO_CFG,GPIO_PIN_11,GPIO_FUNC_2);
	
	gpio_set_pin_value(UC_GPIO,GPIO_PIN_15,GPIO_VALUE_LOW);
}

void CAN_Init(CAN_CTRL_TypeDef* can_ctrl,CAN_InitTypeDef* can_initstruct){
//	uint8_t InitStatus = CAN_InitStatus_Success;
//	uint32_t wait_ack = 0x00000000;
	
	can_ctrl->CAN_MCR &= (~(uint32_t)CAN_MCR_SLEEP);
	
	can_ctrl->CAN_MCR |= CAN_MCR_INRQ;
	
//	while(((can_ctrl->CAN_MSR & CAN_MSR_INAK) != CAN_MSR_INAK) && (wait_ack != INAK_TIMEOUT)){
//		
//		wait_ack++;
//		
//	}
	
//	if((can_ctrl->CAN_MSR & CAN_MSR_INAK) != CAN_MSR_INAK){
//		
//		InitStatus = CAN_InitStatus_Failed;
//		
//	}
//	else{
		
		/* Set the automatic wake-up mode */
		if (can_initstruct->CAN_AWUM == ENABLE)
		{
		  can_ctrl->CAN_MCR |= CAN_MCR_AWUM;
		}
		else
		{
		  can_ctrl->CAN_MCR &= ~(uint32_t)CAN_MCR_AWUM;
		}		
		
		/* Set the no automatic retransmission */
		if (can_initstruct->CAN_NART == ENABLE)
		{
		  can_ctrl->CAN_MCR |= CAN_MCR_NART;
		}
		else
		{
		  can_ctrl->CAN_MCR &= ~(uint32_t)CAN_MCR_NART;
		}
		
		/* Set the receive FIFO locked mode */
		if (can_initstruct->CAN_RFLM == ENABLE)
		{
		  can_ctrl->CAN_MCR |= CAN_MCR_RFLM;
		}
		else
		{
		  can_ctrl->CAN_MCR &= ~(uint32_t)CAN_MCR_RFLM;
		}		
		
		   /* Set the bit timing register */
		can_ctrl->CAN_BTR = (uint32_t)((uint32_t)can_initstruct->CAN_Mode << 30) | \
							((uint32_t)can_initstruct->CAN_SJW << 24) | \
							((uint32_t)can_initstruct->CAN_BS1 << 16) | \
							((uint32_t)can_initstruct->CAN_BS2 << 20) | \
							((uint32_t)can_initstruct->CAN_Prescaler);
		
			
		/* Wait the acknowledge */
//		wait_ack = 0;				
//		while(((can_ctrl->CAN_MSR & CAN_MSR_INAK) != CAN_MSR_INAK) && (wait_ack != INAK_TIMEOUT)){
//		
//			wait_ack++;
//		
//		}		
//		
//		/* ...and check acknowledged */
//		if ((can_ctrl->CAN_MSR & CAN_MSR_INAK) == CAN_MSR_INAK)
//		{
//		  InitStatus = CAN_InitStatus_Failed;
//		}
//		else
//		{
//		  InitStatus = CAN_InitStatus_Success ;
//		}		
		
//	}
	
	
//	return InitStatus;
}


void CAN_FilterInit(CAN_FILTER_TypeDef* CAN_Filter,CAN_FilterRegister_TypeDef* CAN_FR,CAN_FilterInitTypeDef* CAN_FilterInitStruct){


		uint32_t filter_number_bit_pos = 0;
		
		/* Initialisation mode for the filter */
		CAN_Filter->CAN_FMR |= FMR_FINIT;
		
		/* Filter Deactivation */
		CAN_Filter->CAN_FA1R &= (uint32_t)filter_number_bit_pos;
		
		/* Filter Scale */
		if(CAN_FilterInitStruct->CAN_FilterScale == CAN_FilterScale_16bit){
			
			CAN_Filter->CAN_FS1R &= ~(uint32_t)filter_number_bit_pos;
			CAN_FR->FR0 = 
			((0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdHigh) << 16) |
			 (0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdLow);
			
		}
		
		if(CAN_FilterInitStruct->CAN_FilterScale == CAN_FilterScale_32bit){
			filter_number_bit_pos = 0x3;
			CAN_Filter->CAN_FS1R |= (uint32_t)filter_number_bit_pos;
			CAN_FR->FR0 = 
				((0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdHigh) << 16) |
				 (0x0000FFFF & (uint32_t)CAN_FilterInitStruct->CAN_FilterIdLow);

		}
		
		/* Filter Mode */
		if (CAN_FilterInitStruct->CAN_FilterMode == CAN_FilterMode_IdMask){
			/*Id/Mask mode for the filter*/
//			CAN_Filter->CAN_FM1R &= ~(uint32_t)filter_number_bit_pos;
			CAN_Filter->CAN_FM1R = 0x0;
		}
		else{/* CAN_FilterInitStruct->CAN_FilterMode == CAN_FilterMode_IdList */
			/*Identifier list mode for the filter*/
			CAN_Filter->CAN_FM1R |= (uint32_t)filter_number_bit_pos;
		}

		/* Filter activation */
		if (CAN_FilterInitStruct->CAN_FilterActivation == ENABLE){
			filter_number_bit_pos = 0x0;
			CAN_Filter->CAN_FA1R = filter_number_bit_pos;

		}

		/* Leave the initialisation mode for the filter */
		CAN_Filter->CAN_FMR &= ~FMR_FINIT;
	
}

void CAN_StructInit(CAN_InitTypeDef* can_initstruct)
{
  /* Reset CAN init structure parameters values */
  

  /* Initialize the automatic wake-up mode */
  can_initstruct->CAN_AWUM = DISABLE;
  
  /* Initialize the no automatic retransmission */
  can_initstruct->CAN_NART = DISABLE;
  
  /* Initialize the receive FIFO locked mode */
  can_initstruct->CAN_RFLM = DISABLE;
  
  /* Initialize the CAN_Mode member */
  can_initstruct->CAN_Mode = CAN_Mode_Normal;
  
  /* Initialize the CAN_SJW member */
  can_initstruct->CAN_SJW = CAN_SJW_1tq;
  
  /* Initialize the CAN_BS1 member */
  can_initstruct->CAN_BS1 = CAN_TS1_4tq;
  
  /* Initialize the CAN_BS2 member */
  can_initstruct->CAN_BS2 = CAN_TS2_3tq;
  
  /* Initialize the CAN_Prescaler member */
  can_initstruct->CAN_Prescaler = 1;
}

uint8_t CAN_Transmit(CAN_CTRL_TypeDef* can_ctrl,CAN_TXMailBox_TypeDef* CAN_Tx,CanTxMsg* TxMessage){
	
	uint8_t transmit_mailbox = 0;
		
		/* Set up the DLC */
		TxMessage->DLC	&= (uint8_t)0x0000000F;
		CAN_Tx->TDTR &= 0x00000000;
		CAN_Tx->TDTR |= TxMessage->DLC;
		
		/* Set up the data field */
		CAN_Tx->TDLR = (
						 ((uint32_t)TxMessage->Data[3] << 24) | 
						 ((uint32_t)TxMessage->Data[2] << 16) |
						 ((uint32_t)TxMessage->Data[1] << 8)  | 
						 ((uint32_t)TxMessage->Data[0])
						 );
		CAN_Tx->TDHR = (
						 ((uint32_t)TxMessage->Data[7] << 24) | 
						 ((uint32_t)TxMessage->Data[6] << 16) |
						 ((uint32_t)TxMessage->Data[5] << 8)  |
						 ((uint32_t)TxMessage->Data[4])
						 );
											
		/* Request transmission */
		if(TxMessage->IDE == CAN_Id_Standard){
			CAN_Tx->TIR = ((TxMessage->StdId << 21) |
						   (CAN_Id_Standard|TxMessage->RTR)
						  );
		}
		else{
			CAN_Tx->TIR = ((TxMessage->ExtId << 3)	|
						   (TxMessage->IDE|TxMessage->RTR)
						  );
		}

	return transmit_mailbox;

}

uint8_t CAN_TransmitStatus(CAN_CTRL_TypeDef* can_ctrl, uint8_t TransmitMailbox){
	
	uint32_t state = 0;
	
	switch(TransmitMailbox){
		
		case (CAN_TXMAILBOX_0):
				state = can_ctrl->CAN_TSR & (CAN_TSR_RQCP0	| CAN_TSR_TXOK0 | CAN_TSR_TME0);
		break;
		case (CAN_TXMAILBOX_1):
				state = can_ctrl->CAN_TSR & (CAN_TSR_RQCP1	| CAN_TSR_TXOK1 | CAN_TSR_TME1);
		break;
		case (CAN_TXMAILBOX_2):
				state = can_ctrl->CAN_TSR & (CAN_TSR_RQCP2	| CAN_TSR_TXOK2 | CAN_TSR_TME2);
		break;
		default:
				state = CAN_TxStatus_Failed;
		break;
	}
	
	switch(state){
		
		/* transmit pending  */
		case (0x0): 
			state = CAN_TxStatus_Pending;
		break;
		/* transmit failed  */
		case (CAN_TSR_RQCP0 | CAN_TSR_TME0):
			state = CAN_TxStatus_Failed;
		break;
		case (CAN_TSR_RQCP1 | CAN_TSR_TME1):
			state = CAN_TxStatus_Failed;
		break;
		case (CAN_TSR_RQCP2 | CAN_TSR_TME2):
			state = CAN_TxStatus_Failed;
		break;		
        /* transmit succeeded  */
		case (CAN_TSR_RQCP0 | CAN_TSR_TXOK0 | CAN_TSR_TME0):
			state = CAN_TxStatus_Ok;
		break;
		case (CAN_TSR_RQCP1 | CAN_TSR_TXOK1 | CAN_TSR_TME1):
			state = CAN_TxStatus_Ok;
		break;
		case (CAN_TSR_RQCP2 | CAN_TSR_TXOK2 | CAN_TSR_TME2):
			state = CAN_TxStatus_Ok;
		break;
		default: 
			state = CAN_TxStatus_Failed;
		break;		

	}
	
	return (uint8_t)state;
}

void CAN_CancelTransmit(CAN_CTRL_TypeDef* can_ctrl,uint8_t Mailbox){
	
	switch(Mailbox){
		
		case (CAN_TXMAILBOX_0):
				can_ctrl->CAN_TSR |= CAN_TSR_ABRQ0;
		break;
		case (CAN_TXMAILBOX_1):
				can_ctrl->CAN_TSR |= CAN_TSR_ABRQ1;
		break;
		case (CAN_TXMAILBOX_2):
				can_ctrl->CAN_TSR |= CAN_TSR_ABRQ2;
		break;
		default:
				
		break;
		
	}
	
}

void CAN_Receive(CAN_CTRL_TypeDef* can_ctrl,CanRxMsg* RxMessage){
	
	RxMessage->IDE = (uint8_t)0x04 &  UC_CAN_RX_FIFO->RIR;
	
	if(RxMessage->IDE == CAN_Id_Standard){
	
		RxMessage->StdId = (uint32_t)0x000007FF &  (UC_CAN_RX_FIFO->RIR >>21 );
	
	}
	else{
		
		RxMessage->ExtId = (uint32_t)0x1FFFFFFF & (UC_CAN_RX_FIFO->RIR >> 3);
		
	}
	
	RxMessage->RTR = (uint8_t)0x02 & UC_CAN_RX_FIFO->RIR;
	
	RxMessage->DLC = (uint8_t)0x0F & UC_CAN_RX_FIFO->RDTR;
	
	RxMessage->FMI = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDTR >> 8);
	
	/* Get the data field */
	RxMessage->Data[0] = (uint8_t)0xFF & UC_CAN_RX_FIFO->RDLR;
	RxMessage->Data[1] = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDLR >> 8);
	RxMessage->Data[2] = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDLR >> 16);
	RxMessage->Data[3] = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDLR >> 24);
	RxMessage->Data[4] = (uint8_t)0xFF & UC_CAN_RX_FIFO->RDHR;
	RxMessage->Data[5] = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDHR >> 8);
	RxMessage->Data[6] = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDHR >> 16);
	RxMessage->Data[7] = (uint8_t)0xFF & (UC_CAN_RX_FIFO->RDHR >> 24);
	
	can_ctrl->CAN_RFR |= CAN_RFR_RFOM;
	
}

uint8_t CAN_MessagePending(CAN_CTRL_TypeDef* can_ctrl){
	
	uint8_t message_pending=0;
	
	message_pending = (uint8_t)(can_ctrl->CAN_RFR &(uint32_t)0x03);
	
	return message_pending;
}

void CAN_Reset(CAN_CTRL_TypeDef* can_ctrl){
	
	/* Request leave initialisation */			
	can_ctrl->CAN_MCR = 0x18000;
	
}
#endif
