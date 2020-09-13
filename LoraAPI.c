/* -----------------------------------------------------------------
LoRaRead(): Reads from selected register and returns value
Inputs:
- (uint8_t) RegisterAddress_8bit: value of register you wish to read from
Outputs:
- (uint8_t) Value of register being read from
Notes:
- uses PA3 as Chip select manually, PA3 must be configured as GPIO output pin
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* -----------------------------------------------------------------
 * loRaWrite(): writes to selected register with given data
 * Inputs:
 *   - (uint8_t) RegisterAddress_8bit: value of register you wish to write to
 *   - (uint8_t) data8Bit: data to be writen to register
 * Outputs:
 *   - n/a
 * Notes:
 *   - uses PA3 as Chip select manually, PA3 must be configured as GPIO output pin
 * -------------------------------------------------------------- */
/* -----------------------------------------------------------------
 * LoraInitTx(): Initilizes LoRa to Tx mode (Tx FIFO ptr, TX highpower mode, HighFreq) 
 * and puts tranciver into sleep mode.
 * Inputs:
 *   - n/a
 * Outputs:
 *   - (bool) successfull or not
 * Notes:
 *   - uses LoRaRead() and LoRaWrite() so their requirements must be met
 * -------------------------------------------------------------- */
/* -----------------------------------------------------------------
 * LoraSendMessage(): send message via LoRa
 * Inputs:
 *   - (uint8_t) Message[]: to be sent (in byte array)
 *   - (uint8_t) number_of_bytes: number of bytes in Message
 * Outputs:
 *   - (bool) successfull or not (message sent)
 * Notes:
 *   - LoraInitTx OR LoraInitRx should have been ran at some time
 *   - uses LoRaRead() and LoRaWrite() so their requirements must be met
 * Issues:
 *   - Will remain in function untill TxDone interrupt is generated (ie. can get stuck here!!)
 * -------------------------------------------------------------- */
/* -----------------------------------------------------------------
 * LoraInitRx(): Initilizes LoRa to Rx mode (RX FIFO ptr, TX highpower mode, HighFreq) 
 * and puts tranciver into sleep mode.
 * Inputs:
 *   - n/a
 * Outputs:
 *   - (bool) successfull or not
 * Notes:
 *   - uses LoRaRead() and LoRaWrite() so their requirements must be met
 * -------------------------------------------------------------- */


/*********************************************************************
*
*   NAME:
*       loraAPI.c
*
*   DESCRIPTION:
*       API for interfacing Tiva launchpad with LoRa 
*       requires tiva board support packet files as listed in general
*       includes
*
*   Copyright 2020 Nate Lenze
*
*********************************************************************/

/*--------------------------------------------------------------------
                           GENERAL INCLUDES
--------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"

/*--------------------------------------------------------------------
                          LITERAL CONSTANTS
--------------------------------------------------------------------*/
#define             CONFIG_REG_ADDR     0x01   /* Config Register Address */
#define             LORA_FIFO_ADDR      0x00   /* LoRa FIFO Address       */

/*--------------------------------------------------------------------
                                TYPES
--------------------------------------------------------------------*/
typedef uint8 lora_modes;       /* Operating Modes                  */
enum 
    {
    SLEEP,                      /* sleep mode                       */
    STBY,                       /* standby mode                     */
    FSTX,                       /* frequency sythesis transmit mode 
                                   NOT IMPLEMENTED                  */
    TX,                         /* transmit mode                    */
    FSRX,                       /* frequency sythesis receive mode
                                   NOT IMPLEMENTED                  */
    RXCONTINUOUS,               /* continious receive mode          */
    RXSINGLE,                   /* single receive mode              */
    CAD                         /* preamble detect mode             
                                   NOT IMPLEMENTED                  */
    }; 

typedef uint8_t lora_registers; /* lora registers                   */
enum
    {
    LORA_REGISTER_OP_MODE = 0x01,  /* operating modes register      */
    LORA_REGISTER_FIFO    = 0x00,  /* fifo register                 */
    LORA_REGISTER_POWER   = 0x09,  /* power configuration register  */
    LORA_FIFO_ADDR_PTR    = 0x0D,  /* pointer to fifo buffer        */
    LORA_TX_FIFO_ADDR     = 0x0E,  /* base addrees for tx fifo      */
    LORA_RX_FIFO_ADDR     = 0x0F,  /* base address for rx fifo      */
    LOROA_RX_CURR_ADDR    = 0x10   /* current address in buffer for
                                      last rx'ed msg                */
    };
/*--------------------------------------------------------------------
                           MEMORY CONSTANTS
--------------------------------------------------------------------*/

/*--------------------------------------------------------------------
                              VARIABLES
--------------------------------------------------------------------*/

/*--------------------------------------------------------------------
                                MACROS
--------------------------------------------------------------------*/

/*--------------------------------------------------------------------
                              PROCEDURES
--------------------------------------------------------------------*/
uint32_t loRa_read_register
    (
    lora_registers register_address             /* register address */
    );

void loRa_write_register
    (
    lora_registers  register_address,           /* register address */
    uint8_t         register_data               /* register data    */
    );


/*********************************************************************
*
*   PROCEDURE NAME:
*       loRa_read_register
*
*   DESCRIPTION:
*       Reads from selected register and returns value
*
*********************************************************************/
uint32_t loRa_read_register
    (
    lora_registers register_address             /* register address */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint32_t    message_return;  /* value of register             */
uint8_t     number_in_fifo;  /* how many items remain in fifo */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
message_return   = 0xFF;
number_in_fifo   = 0;

/*----------------------------------------------------------
Read from fifo until empty
----------------------------------------------------------*/
while ( number_in_fifo != 0x00 )
    {
    number_in_fifo = SSIDataGetNonBlocking( SSI0_BASE, &message_return );
    }

/*----------------------------------------------------------
Toggle CS low and put request
----------------------------------------------------------*/
GPIO_PORTA_DATA_R &= ~( 0x8 );                      //~(1<<3);//toggle A3 low
SSIDataPut( SSI0_BASE, register_address );

/*----------------------------------------------------------
Wait for operation to complete
----------------------------------------------------------*/
while( SSIBusy( SSI0_BASE ) )
    {
    }

/*----------------------------------------------------------
Put empty data to fill in timing gap and empty fifo
----------------------------------------------------------*/
SSIDataPut( SSI0_BASE, 0x00 );
SSIDataGet( SSI0_BASE, &message_return );

/*----------------------------------------------------------
Wait for operation to complete
----------------------------------------------------------*/
while( SSIBusy( SSI0_BASE ) )
    {
    }

/*----------------------------------------------------------
Pull from fifo and toggle CS
----------------------------------------------------------*/
SSIDataGet( SSI0_BASE, &message_return );
GPIO_PORTA_DATA_R |= 0x08;                              //toggle A3 high

return message_return;

} /* loRa_read_register() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       loRa_write_register
*
*   DESCRIPTION:
*       writes to a selected register
*
*********************************************************************/
void loRa_write_register
    (
    lora_registers  register_address,           /* register address */
    uint8_t         register_data               /* register data    */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint32_t    message_return;  /* value of register             */
uint8_t     number_in_fifo;  /* how many items remain in fifo */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
message_return   = 0xFF;
number_in_fifo   = 0;

/*----------------------------------------------------------
Read from fifo until empty
----------------------------------------------------------*/
while ( number_in_fifo != 0x00 )
    {
    number_in_fifo = SSIDataGetNonBlocking( SSI0_BASE, &message_return );
    }
/*----------------------------------------------------------
Toggle CS low and put request
----------------------------------------------------------*/
GPIO_PORTA_DATA_R &= ~(0x8);                                 //~(1<<3);//toggle A3 low
SSIDataPut(SSI0_BASE, (0x80 | register_address));            //put register value (OR w/ x80 for write)

/*----------------------------------------------------------
Wait for operation to complete
----------------------------------------------------------*/
while( SSIBusy( SSI0_BASE ) )
    {
    }

/*----------------------------------------------------------
Write data to register
----------------------------------------------------------*/
SSIDataPut( SSI0_BASE, register_data );//put blank data for timing

/*----------------------------------------------------------
Wait for operation to complete
----------------------------------------------------------*/
while( SSIBusy( SSI0_BASE ) )
    {
    }

/*----------------------------------------------------------
Toggle CS
----------------------------------------------------------*/
GPIO_PORTA_DATA_R |= 0x08;                                  //toggle A3 high

} /* loRa_write_register() */


/*********************************************************************
*
*   PROCEDURE NAME:
*       lora_init_continious_tx
*
*   DESCRIPTION:
*       runs the proper steps to initilize lora into TX 
*       continious mode
*
*********************************************************************/
bool lora_init_continious_tx
    (
    void
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t config_register_data; /* configuration data       */
uint8_t return_data;          /* return data              */
uint8_t tx_fifo_ptr;          /* tx fifo pointer          */
uint8_t power_modes;          /* power modes data         */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
/*--------------------------------
config register bit defintions:
    0-2 - operating mode
    3   - frequency register select
    4-5 - reserved
    6   - Register select (Lora/FSK)
    7   - long range mode (Lora/FSK)

0x80 --> LoRa mode, sleep mode 
--------------------------------*/
config_register_data  = 0x80;
tx_fifo_ptr           = 0x00;
power_modes           = 0x00;

/*----------------------------------------------------------
Configure into LoRa sleep mode and verify
----------------------------------------------------------*/
loRaWrite( LORA_REGISTER_OP_MODE, config_register_data );

configRegisterData = loRa_read_register( LORA_REGISTER_OP_MODE ); 

if ( configRegisterData != 0x80 )
    {
    return false;
    }

/*----------------------------------------------------------
Configure high power TX mode and verify
----------------------------------------------------------*/
loRaWrite( LORA_REGISTER_POWER, 0xFF );
power_modes = loRa_read_register( LORA_REGISTER_POWER );

if( power_modes != 0xFF )
    {
    return false;
    }

/*----------------------------------------------------------
Configure TX fifo pointers and verify
----------------------------------------------------------*/
loRa_write_register( LORA_FIFO_ADDR_PTR, 0x00);
tx_fifo_ptr = loRa_read_register( LORA_FIFO_ADDR_PTR );

if( tx_fifo_ptr != 0x80 ){
    return false;
}

return true;

} /* lora_init_continious_tx() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       lora_init_rx
*
*   DESCRIPTION:
*       runs the proper steps to initilize lora into RX 
*       continious mode
*
*********************************************************************/
bool lora_init_rx
    (
    void
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t config_register_data; /* configuration data       */
uint8_t return_data;          /* return data              */
uint8_t rx_fifo_ptr;          /* rx fifo pointer          */
uint8_t power_modes;          /* power modes data         */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
/*--------------------------------
config register bit defintions:
    0-2 - operating mode
    3   - frequency register select
    4-5 - reserved
    6   - Register select (Lora/FSK)
    7   - long range mode (Lora/FSK)

0x80 --> LoRa mode, sleep mode 
--------------------------------*/
config_register_data  = 0x80;
tx_fifo_ptr           = 0x00;
power_modes           = 0x00;

/*----------------------------------------------------------
Configure into LoRa sleep mode and verify
----------------------------------------------------------*/
loRaWrite( LORA_REGISTER_OP_MODE, config_register_data );

configRegisterData = loRa_read_register( LORA_REGISTER_OP_MODE ); 

if ( configRegisterData != 0x80 )
    {
    return false;
    }

/*----------------------------------------------------------
Configure high power TX mode and verify
----------------------------------------------------------*/
loRaWrite( LORA_REGISTER_POWER, 0xFF );
power_modes = loRa_read_register( LORA_REGISTER_POWER );

if( power_modes != 0xFF )
    {
    return false;
    }

/*----------------------------------------------------------
Configure RX fifo pointers and verify
----------------------------------------------------------*/
loRa_write_register( LORA_FIFO_ADDR_PTR, 0x00);
tx_fifo_ptr = loRa_read_register( LORA_FIFO_ADDR_PTR );

if( tx_fifo_ptr != 0x80 ){
    return false;
}

return true;

} /* lora_init_rx() */

// /* -----------------------------------------------------------------
//  * LoRaMode(): sets configuration register to specificed mode
//  * Inputs:
//  *   - (uint8_t) mode: what mode to set, (See enum 'modes' at top of file)
//  * Outputs:
//  *   - (bool) successfull or not
//  * Notes:
//  *   - uses LoRaRead() and LoRaWrite() so their requirements must be met
//  * -------------------------------------------------------------- */
// bool LoRaMode(uint8_t mode){
// 	uint8_t configReg = 0x00;
// 	if(mode == SLEEP){
// 		configReg = loRaRead(0x01);
// 		configReg &= ~(0x7);
// 		loRaWrite(0x01, configReg);
// 		if(loRaRead(0x01) != configReg){
// 			return false;
// 		}
// 		return true;
// 	}else if(mode == STBY){
// 		configReg = loRaRead(0x01);
// 		configReg &= ~(0x7);
// 		configReg |= 0x01;
// 		loRaWrite(0x01, configReg);
// 		if(loRaRead(0x01) != configReg){
// 			return false;
// 		}
// 		return true;
// 	}else if(mode == TX){
// 		configReg = loRaRead(0x01);
// 		configReg &= ~(0x7);
// 		configReg |= 0x03;
// 		loRaWrite(0x01, configReg);
// 		if(loRaRead(0x01) != configReg){
// 			return false;
// 		}
// 		return true;
// 	}else if(mode == RXCONTINUOUS){
// 		configReg = loRaRead(0x01);
// 		configReg &= ~(0x7);
// 		configReg |= 0x05;
// 		loRaWrite(0x01, configReg);
// 		if(loRaRead(0x01) != configReg){
// 			return false;
// 		}
// 		return true;
// 	}else if(mode == RXSINGLE){
// 		configReg = loRaRead(0x01);
// 		configReg &= ~(0x7);
// 		configReg |= 0x06;
// 		loRaWrite(0x01, configReg);
// 		if(loRaRead(0x01) != configReg){
// 			return false;
// 		}
// 		return true;
// 	}else{
// 		return false;
// 	}
// }

/*********************************************************************
*
*   PROCEDURE NAME:
*       lora_send_message
*
*   DESCRIPTION:
*       send message
*
*********************************************************************/
bool lora_send_message(uint8_t Message[], uint8_t number_of_bytes){
	//local variables
	uint8_t configRegisterData = 0x00;
	uint8_t FifoPtrAddr = 0x00;
	int i;
	
	//Put Tranciver in stby mode to fill fifo
	configRegisterData = loRaRead(CONFIG_REG_ADDR);
	configRegisterData |= 0x1;
	loRaWrite(CONFIG_REG_ADDR, configRegisterData);
	
	//reset Tx FifoPtr
	FifoPtrAddr = loRaRead(0x0E); //(TX fifo Base)
	loRaWrite(0x0D,FifoPtrAddr);//point 0x00 FIFO to TX location
	if(loRaRead(0x0D) != FifoPtrAddr){
		return false;
	}
	
	//Fill fifo
	for(i = 0; i < number_of_bytes; i++){
		loRaWrite(0x00, Message[i]);
	}
	
	//set payload length to numBytes
	loRaWrite(0x22,number_of_bytes);
	if(loRaRead(0x22) != number_of_bytes){
		return false;
	}
	
	//Set to Tx Mode
	configRegisterData = 0x83; //set last two bits high (Tx mode)
	loRaWrite(CONFIG_REG_ADDR,configRegisterData);
	if(loRaRead(CONFIG_REG_ADDR) != 0x83){
		return false;
	}
	
	//wait until TxDone sent
	while((loRaRead(0x12) & 0x08) != 0x08)
		{
	  }
	
	//Clear flags
	loRaWrite(0x12, 0x08); //clear IRQ
	if(loRaRead(0x12)!= 0x00){
		return false;
	}

	return true;
}

