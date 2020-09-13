/*********************************************************************
*
*   NAME:
*       loraAPI.c
*
*   DESCRIPTION:
*       API for interfacing Tiva launchpad with LoRa. 
*       requires tiva board support package files as listed in general
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
/*----------------------------------
A3 is used as chip select, to change
what pin is used, modify DATA_PIN
and DATA_PIN_GROUP defines
----------------------------------*/
#define  DATA_PIN            0x08                /* pin 3             */
#define  DATA_PORT_GROUP     GPIO_PORTA_DATA_R   /* pin register A    */


#define  LORA_MAX_POWER_MODE 0xFF                /* max power output  */

#define  LORA_BASE_FIFO_ADD  0x00                /* base fifo address */

#define LORA_SLEEP_MODE      0x80                /* config register 
                                                    sleep mode        */

#define LORA_STBY_MODE       0x01                /* config register 
                                                    standby mode      */



/*--------------------------------------------------------------------
                                TYPES
--------------------------------------------------------------------*/
typedef uint8_t lora_modes;       /* Operating Modes                  */
enum 
    {
    SLEEP,                      /* sleep mode                       */
    STBY,                       /* standby mode                     */
    FSTX,                       /* frequency sythesis transmit mode */
    TX,                         /* transmit mode                    */
    FSRX,                       /* frequency sythesis receive mode  */
    RXCONTINUOUS,               /* continious receive mode          */
    RXSINGLE,                   /* single receive mode              */
    CAD                         /* preamble detect mode             */
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
    LORA_RX_CURR_ADDR     = 0x10,  /* current address in buffer for
                                      last rx'ed msg                */
    LORA_FLAGS_MASK       = 0x11,  /* masks for flag register       */
    LORA_REGISTER_FLAGS   = 0x12,  /* flags register                */
    LORA_PAYLOAD_SIZE     = 0x22   /* rx payload size register      */
           
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
DATA_PORT_GROUP &= ~( DATA_PIN );
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
DATA_PORT_GROUP |= DATA_PIN;

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
DATA_PORT_GROUP &= ~(DATA_PIN);
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
DATA_PORT_GROUP |= DATA_PIN;

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
uint8_t tx_fifo_ptr_verify;   /* tx fifo pointer verify   */
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
config_register_data  = LORA_SLEEP_MODE;
tx_fifo_ptr           = 0x00;
tx_fifo_ptr_verify    = 0x00;
power_modes           = 0x00;

/*----------------------------------------------------------
Configure into LoRa sleep mode and verify
----------------------------------------------------------*/
loRa_write_register( LORA_REGISTER_OP_MODE, config_register_data );

config_register_data = loRa_read_register( LORA_REGISTER_OP_MODE ); 

if ( config_register_data != LORA_SLEEP_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Configure high power TX mode and verify
----------------------------------------------------------*/
loRa_write_register( LORA_REGISTER_POWER, LORA_MAX_POWER_MODE );
power_modes = loRa_read_register( LORA_REGISTER_POWER );

if( power_modes != LORA_MAX_POWER_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Configure TX fifo pointers and verify

Register 0x00 (fifo) can be configured to point to specific
sections of the fifo buffer. By default tx points to 0x80.
three registers control this function:
    - LORA_FIFO_ADDR_PTR: address register 0x00 points to
    - LORA_TX_FIFO_ADDR: address where TX data starts
    - LORA_RX_FIFO_ADDR: address where RX data starts

In case LORA_TX_FIFO_ADDR is modified, we always read it
and set LORA_FIFO_ADDR_PTR accordingly.
----------------------------------------------------------*/
tx_fifo_ptr = loRa_read_register ( LORA_TX_FIFO_ADDR );
loRa_write_register( LORA_FIFO_ADDR_PTR, tx_fifo_ptr );

tx_fifo_ptr_verify = loRa_read_register( LORA_FIFO_ADDR_PTR );
if( tx_fifo_ptr != tx_fifo_ptr_verify )
    {
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
uint8_t rx_fifo_ptr_verify;   /* rx fifo pointer verify   */
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
config_register_data  = LORA_SLEEP_MODE;
rx_fifo_ptr           = 0x00;
rx_fifo_ptr_verify    = 0x00;
power_modes           = 0x00;

/*----------------------------------------------------------
Configure into LoRa sleep mode and verify
----------------------------------------------------------*/
loRa_write_register( LORA_REGISTER_OP_MODE, config_register_data );

config_register_data = loRa_read_register( LORA_REGISTER_OP_MODE ); 

if ( config_register_data != LORA_SLEEP_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Configure high power TX mode and verify
----------------------------------------------------------*/
loRa_write_register( LORA_REGISTER_POWER, LORA_MAX_POWER_MODE );
power_modes = loRa_read_register( LORA_REGISTER_POWER );

if( power_modes != LORA_MAX_POWER_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Configure RX fifo pointers and verify

Register 0x00 (fifo) can be configured to point to specific
sections of the fifo buffer. By default rx points to 0x00.
three registers control this function:
    - LORA_FIFO_ADDR_PTR: address register 0x00 points to
    - LORA_TX_FIFO_ADDR: address where TX data starts
    - LORA_RX_FIFO_ADDR: address where RX data starts

In case LORA_RX_FIFO_ADDR is modified, we always read it
and set LORA_FIFO_ADDR_PTR accordingly.
----------------------------------------------------------*/
rx_fifo_ptr = loRa_read_register ( LORA_RX_FIFO_ADDR );
loRa_write_register( LORA_FIFO_ADDR_PTR, rx_fifo_ptr );

rx_fifo_ptr_verify = loRa_read_register( LORA_FIFO_ADDR_PTR );
if( rx_fifo_ptr != rx_fifo_ptr_verify )
    {
    return false;
    }

return true;

} /* lora_init_rx() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       lora_send_message
*
*   DESCRIPTION:
*       send message
*
*********************************************************************/
bool lora_send_message
    (
    uint8_t Message[],                    /* array of bytes to send */
    uint8_t number_of_bytes               /* size of array          */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t config_register_data;   /* config register data   */
uint8_t fifo_ptr_address;       /* fifo pointer address   */
int i;                          /* interator              */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
config_register_data  = 0x00;
fifo_ptr_address      = 0x00;
i                     = 0x00;
	
/*----------------------------------------------------------
Put into standby mode to fill fifo
----------------------------------------------------------*/
config_register_data = loRa_read_register( LORA_REGISTER_OP_MODE );
config_register_data |= LORA_STBY_MODE;
loRa_write_register( LORA_REGISTER_OP_MODE, config_register_data);

/*----------------------------------------------------------
Reset TX fifo 
----------------------------------------------------------*/
fifo_ptr_address = loRa_read_register( LORA_TX_FIFO_ADDR );
loRa_write_register( LORA_FIFO_ADDR_PTR, fifo_ptr_address );

if( loRa_read_register( LORA_FIFO_ADDR_PTR ) != fifo_ptr_address )
    {
    return false;
    }

/*----------------------------------------------------------
Fill in fifo
----------------------------------------------------------*/
for( i = 0; i < number_of_bytes; i++ )
    {
    loRa_write_register( LORA_REGISTER_FIFO, Message[i] );
    }

/*----------------------------------------------------------
Set payload length to numBytes and verify
----------------------------------------------------------*/
loRa_write_register( LORA_PAYLOAD_SIZE, number_of_bytes );
if( loRa_read_register( LORA_PAYLOAD_SIZE ) != number_of_bytes )
    {
    return false;
    }

/*----------------------------------------------------------
Set into TX mode
----------------------------------------------------------*/
config_register_data = 0x83;
loRa_write_register( LORA_REGISTER_OP_MODE, config_register_data );
if( loRa_read_register( LORA_REGISTER_OP_MODE ) != 0x83 )
    {
    return false;
    }

/*----------------------------------------------------------
Wait for TX to complete
----------------------------------------------------------*/
while( ( loRa_read_register( LORA_REGISTER_FLAGS ) & 0x08 ) != 0x08)
    {
    }
/*----------------------------------------------------------
Clear IRQ flags
----------------------------------------------------------*/
loRa_write_register( LORA_REGISTER_FLAGS, 0x08 );
if( loRa_read_register( LORA_REGISTER_FLAGS ) != 0x00 )
    {
    return false;
    }

return true;
} /* lora_send_message() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       lora_rx_message
*
*   DESCRIPTION:
*       recive message
*
*********************************************************************/
