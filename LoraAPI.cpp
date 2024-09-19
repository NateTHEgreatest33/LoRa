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
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "LoraAPI.hpp"

/*--------------------------------------------------------------------
                          LITERAL CONSTANTS
--------------------------------------------------------------------*/
#define LORA_MAX_POWER_MODE     ( 0xFF )               /* max power output  */

#define LORA_BASE_FIFO_ADD      ( 0x00 )               /* base fifo address */

#define LORA_REGISTER_SELECT    ( 0x80 )               /* select lora
                                                           registers        */

#define LORA_SLEEP_MODE         ( LORA_REGISTER_SELECT | MODE_SLEEP )
                                                       /* config register 
                                                          sleep mode        */

#define LORA_STBY_MODE          ( LORA_REGISTER_SELECT | MODE_STBY )
                                                       /* config register 
                                                          standby mode      */

#define LORA_TX_MODE            ( LORA_REGISTER_SELECT | MODE_TX )
                                                       /* config register 
                                                          tx mode           */

#define LORA_RX_CONT_MODE       ( LORA_REGISTER_SELECT | MODE_RXCONTINUOUS )
                                                       /* config register rx
                                                          continious mode   */

#define LORA_TX_DONE_MASK       ( 0x08 )               /* tx done mask      */

#define LORA_VALID_HEADER_MASK  ( 0x10 )               /* valid header mask */

#define LORA_CRC_ERROR_MASK     ( 0x20 )               /* CRC error    mask */

#define LORA_RX_DONE_MASK       ( 0x40 )               /* rx done mask      */

#define LORA_RX_TIMEOUT_MASK    ( 0x80 )               /* rx timeout mask   */

#define LORA_CLR_RX_ERR_FLAGS   ( LORA_RX_TIMEOUT_MASK | LORA_CRC_ERROR_MASK )
                                                       /* clear error flags 
                                                                       mask */

#define LORA_CLR_RX_FLAG        ( LORA_RX_DONE_MASK | LORA_VALID_HEADER_MASK )
                                                       /* clear rx flags 
                                                                       mask */

#define SPI_WRITE_DATA_FLAG     ( 0x80 )               /* SPI write flag    */

#define LORA_FIFO_SIZE          ( 0x10 )              /* buffer size is 128 
                                                                      bytes */

/*--------------------------------------------------------------------
                                TYPES
--------------------------------------------------------------------*/

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

/*********************************************************************
*
*   PROCEDURE NAME:
*       loraInterface()
*
*   DESCRIPTION:
*       Constructor for loraInterface class
*
*********************************************************************/
core::loraInterface::loraInterface
    (
    spi_inst_t *spi,                /* SPI Interface info  */
    core::console& c_ref            /* reference to global console */
    ) :
    p_console( c_ref ),
    p_spi_port( spi )
{

/*----------------------------------------------------------
Setup port
----------------------------------------------------------*/
spi_init(p_spi_port, 100000 );
spi_set_format(p_spi_port, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI );

/*----------------------------------------------------------
Initilize last_fifo_ptr to base of FIFO
----------------------------------------------------------*/
p_last_fifo_ptr = 0x00;

} /* core::loraInterface::loraInterface() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       ~loraInterface()
*
*   DESCRIPTION:
*       Deconstructor for loraInterface class
*
*********************************************************************/
core::loraInterface::~loraInterface
    (
    void
    )
{
} /* core::loraInterface::~loraInterface() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::init_tx
*
*   DESCRIPTION:
*       runs the proper steps to initilize lora into TX 
*       continious mode
*
*********************************************************************/
bool core::loraInterface::init_tx
    (
    void
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t volatile config_register_data; /* configuration 
                                          data            */
uint8_t volatile tx_fifo_ptr;          /* tx fifo pointer */
uint8_t volatile return_value_verify;  /* verification 
                                          value           */
uint8_t volatile power_modes;          /* power modes 
                                          data            */
 
/*----------------------------------------------------------
Initilize local/static variables
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
return_value_verify   = 0x00;
power_modes           = 0x00;

/*----------------------------------------------------------
Configure into LoRa sleep mode and verify
----------------------------------------------------------*/
write_register( LORA_REGISTER_OP_MODE, config_register_data );

config_register_data = read_register( LORA_REGISTER_OP_MODE ); 

if ( config_register_data != LORA_SLEEP_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Configure high power TX mode and verify
----------------------------------------------------------*/
write_register( LORA_REGISTER_POWER, LORA_MAX_POWER_MODE );
power_modes = read_register( LORA_REGISTER_POWER );

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
tx_fifo_ptr = read_register ( LORA_TX_FIFO_ADDR );
write_register( LORA_FIFO_ADDR_PTR, tx_fifo_ptr );

/*----------------------------------------------------------
Assert if TX fifo address is not as expected.
----------------------------------------------------------*/
p_console.add_assert( "tx/rx fifo's are not set-up as expected. This will cause issues if larger messages are tx'ed or rx'ed" , (tx_fifo_ptr != MAX_LORA_MSG_SIZE) );

return_value_verify = read_register( LORA_FIFO_ADDR_PTR );
if( tx_fifo_ptr != return_value_verify )
    {
    return false;
    }

/*----------------------------------------------------------
Set into TX mode and verify

NOTE:
    We very mode is TX or Standby as after a succuessfull tx,
		we will enter standby mode
----------------------------------------------------------*/
write_register(LORA_REGISTER_OP_MODE, LORA_TX_MODE);
return_value_verify = read_register( LORA_REGISTER_OP_MODE );
		
if( return_value_verify !=  LORA_TX_MODE && 
    return_value_verify !=  LORA_STBY_MODE  )
    {
    return false;
    }
		
return true;
} /* core::loraInterface::init_tx() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::init_continious_rx
*
*   DESCRIPTION:
*       runs the proper steps to initilize lora into RX 
*       continious mode
*
*********************************************************************/
bool core::loraInterface::init_continious_rx
    (
    void
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t volatile config_register_data; /* configuration 
                                          data            */
uint8_t volatile rx_fifo_ptr;          /* rx fifo pointer */
uint8_t volatile return_value_verify;  /* verification 
                                          value           */
uint8_t volatile power_modes;          /* pwr modes data  */

/*----------------------------------------------------------
Initilize local/static variables
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
return_value_verify   = 0x00;
power_modes           = 0x00;
p_last_fifo_ptr       = 0x00; //need to verify this is needed after tx/rx transfer

/*----------------------------------------------------------
Configure into LoRa sleep mode and verify
----------------------------------------------------------*/
write_register( LORA_REGISTER_OP_MODE, config_register_data );
		
config_register_data = read_register( LORA_REGISTER_OP_MODE );		

if ( config_register_data != LORA_SLEEP_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Configure high power TX mode and verify
----------------------------------------------------------*/
write_register( LORA_REGISTER_POWER, LORA_MAX_POWER_MODE );
power_modes = read_register( LORA_REGISTER_POWER );

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
rx_fifo_ptr = read_register ( LORA_RX_FIFO_ADDR );
write_register( LORA_FIFO_ADDR_PTR, rx_fifo_ptr );

return_value_verify = read_register( LORA_FIFO_ADDR_PTR );
if( rx_fifo_ptr != return_value_verify )
    {
    return false;
    }

/*----------------------------------------------------------
Set into RX continious mode and verify 
----------------------------------------------------------*/
write_register(LORA_REGISTER_OP_MODE, LORA_RX_CONT_MODE);
return_value_verify = read_register( LORA_REGISTER_OP_MODE );

if( return_value_verify !=  LORA_RX_CONT_MODE )
    {
    return false;
    }

return true;

} /* core::loraInterface::init_continious_rx() */




/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::send_message
*
*   DESCRIPTION:
*       send message
*
*********************************************************************/
bool core::loraInterface::send_message
    (
    uint8_t Message[],                    /* array of bytes to send */
    uint8_t number_of_bytes               /* size of array          */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t volatile fifo_ptr_address; /* fifo ptr address    */
int i;                             /* interator           */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
fifo_ptr_address      = 0x00;
i                     = 0x00;
	
/*----------------------------------------------------------
Put into standby mode to fill fifo
----------------------------------------------------------*/
write_register( LORA_REGISTER_OP_MODE, LORA_STBY_MODE);

/*----------------------------------------------------------
Reset TX fifo 
----------------------------------------------------------*/
fifo_ptr_address = read_register( LORA_TX_FIFO_ADDR );

write_register( LORA_FIFO_ADDR_PTR, fifo_ptr_address );

if( read_register( LORA_FIFO_ADDR_PTR ) != fifo_ptr_address )
    {
    return false;
    }

/*----------------------------------------------------------
Fill in fifo
----------------------------------------------------------*/
for( i = 0; i < number_of_bytes; i++ )
    {
    write_register( LORA_REGISTER_FIFO, Message[i] );
    }

/*----------------------------------------------------------
Set payload length to numBytes and verify
----------------------------------------------------------*/
write_register( LORA_PAYLOAD_SIZE, number_of_bytes );

if( read_register( LORA_PAYLOAD_SIZE ) != number_of_bytes )
    {
    return false;
    }

/*----------------------------------------------------------
Set into TX mode
----------------------------------------------------------*/
write_register( LORA_REGISTER_OP_MODE, LORA_TX_MODE );

if( read_register( LORA_REGISTER_OP_MODE ) != LORA_TX_MODE )
    {
    return false;
    }

/*----------------------------------------------------------
Wait for TX to complete
----------------------------------------------------------*/
while( ( read_register( LORA_REGISTER_FLAGS ) & LORA_TX_DONE_MASK ) != LORA_TX_DONE_MASK )
    {
    }
/*----------------------------------------------------------
Clear IRQ flags
----------------------------------------------------------*/
write_register( LORA_REGISTER_FLAGS, LORA_TX_DONE_MASK );

if( read_register( LORA_REGISTER_FLAGS ) != 0x00 )
    {
    return false;
    }

return true;

} /* core::loraInterface::send_message() */

/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::get_last_message
*
*   DESCRIPTION:
*       retrive the last message received from the lora fifo
*
*********************************************************************/
bool core::loraInterface::get_last_message
    (
    uint8_t *message,                  /* pointer to return message */
    uint8_t size_of_message,           /* array size of message[]   */
    uint8_t *size,                     /* size of return message    */
    lora_errors *error                 /* pointer to error variable */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t volatile flag_register_data; /* data of flag 
                                        register          */
uint8_t volatile rx_fifo_ptr;        /* rx fifo pointer   */
int i;                               /* interator         */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
flag_register_data  = 0x00;
rx_fifo_ptr         = 0x00;
i                   = 0x00;

/*----------------------------------------------------------
Initilize variables
----------------------------------------------------------*/
*error    = RX_NO_ERROR;
*size           = 0;

/*----------------------------------------------------------
Determine status of Rx
----------------------------------------------------------*/
flag_register_data = read_register( LORA_REGISTER_FLAGS );

/*----------------------------------------------------------
Determine if error is present
----------------------------------------------------------*/

if ( ( flag_register_data & LORA_CRC_ERROR_MASK ) == LORA_CRC_ERROR_MASK )
    {
    *error = RX_CRC_ERROR;
    }
else if ( ( flag_register_data & LORA_RX_TIMEOUT_MASK ) == LORA_RX_TIMEOUT_MASK )
    {
    *error = RX_TIMEOUT;
    }
/*----------------------------------------------------------
If errors are present, clear

-------->>>>>>NEED TO VERIFY single write does not clear all flags

----------------------------------------------------------*/
if ( *error != RX_NO_ERROR )
    {
    write_register( LORA_REGISTER_FLAGS, LORA_CLR_RX_ERR_FLAGS );
    }

/*----------------------------------------------------------
Determine if message has been received
----------------------------------------------------------*/
if ( ( flag_register_data & LORA_RX_DONE_MASK ) == LORA_RX_DONE_MASK )
    {
    /*----------------------------------------------------------
    Verify header
    ----------------------------------------------------------*/
    if ( ( flag_register_data & LORA_VALID_HEADER_MASK ) != LORA_VALID_HEADER_MASK )
        {
        *error = RX_INVALID_HEADER;
        }
    /*----------------------------------------------------------
    get size
    ----------------------------------------------------------*/
    *size = read_register( LORA_RX_COUNT );

    /*----------------------------------------------------------
    Clear header and rx flag
    ----------------------------------------------------------*/
    write_register( LORA_REGISTER_FLAGS, LORA_CLR_RX_FLAG );

    /*----------------------------------------------------------
    Get last rx'ed fifo pointer and load that into the fifo 
    address register for the SPI interface 
    ----------------------------------------------------------*/
    rx_fifo_ptr = read_register( LORA_RX_CURR_ADDR );
    write_register( LORA_FIFO_ADDR_PTR, rx_fifo_ptr );

    /*----------------------------------------------------------
    Verify message[] can fit message received
    ----------------------------------------------------------*/
    if( *size > size_of_message )
        {
        *error = RX_ARRAY_SIZE_ERR;
        *size = 0;
        }
    else
        {
        /*----------------------------------------------------------
        Tranfer message to array
        ----------------------------------------------------------*/
        for( i = 0; i < *size; i++ )
            {
            message[i] = read_register( LORA_REGISTER_FIFO );
            }
        }

    /*----------------------------------------------------------
    Return true for message received
    ----------------------------------------------------------*/
    return true;
    }
else
    {
    /*----------------------------------------------------------
    Return false for no message received
    ----------------------------------------------------------*/
    return false;
    }
} /* core::loraInterface::get_last_message() */



/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::get_last_message
*
*   DESCRIPTION:
*       retrive the entire contents of the lora fifo
*
*********************************************************************/
bool core::loraInterface::get_message
    (
    uint8_t *message,                  /* pointer to return message */
    uint8_t size_of_message,           /* array size of message[]   */
    uint8_t *size,                     /* size of return message    */
    lora_errors *error                 /* pointer to error variable */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t volatile flag_register_data; /* data of flag 
                                        register          */
uint8_t volatile rx_fifo_ptr;        /* rx fifo pointer   */
uint8_t volatile rx_fifo_base_addr;  /* rx fifo base addr */
uint8_t fifo_read_idx;               /* rx fifo index to 
                                        begin reading     */
int i;                               /* interator         */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
flag_register_data  = 0x00;
rx_fifo_ptr         = 0x00;
i                   = 0x00;
fifo_read_idx       = 0x00;

/*----------------------------------------------------------
Initilize variables
----------------------------------------------------------*/
*error    = RX_NO_ERROR;
*size           = 0;

/*----------------------------------------------------------
Determine status of Rx
----------------------------------------------------------*/
flag_register_data = read_register( LORA_REGISTER_FLAGS );

/*----------------------------------------------------------
Determine if error is present
----------------------------------------------------------*/

if ( ( flag_register_data & LORA_CRC_ERROR_MASK ) == LORA_CRC_ERROR_MASK )
    {
    *error = RX_CRC_ERROR;
    }
else if ( ( flag_register_data & LORA_RX_TIMEOUT_MASK ) == LORA_RX_TIMEOUT_MASK )
    {
    *error = RX_TIMEOUT;
    }
/*----------------------------------------------------------
If errors are present, clear
----------------------------------------------------------*/
if ( *error != RX_NO_ERROR )
    {
    write_register( LORA_REGISTER_FLAGS, LORA_CLR_RX_ERR_FLAGS );
    }

/*----------------------------------------------------------
Determine if message has been received
----------------------------------------------------------*/
if ( ( flag_register_data & LORA_RX_DONE_MASK ) == LORA_RX_DONE_MASK )
    {
    /*----------------------------------------------------------
    Verify header
    ----------------------------------------------------------*/
    if ( ( flag_register_data & LORA_VALID_HEADER_MASK ) != LORA_VALID_HEADER_MASK )
        {
        *error = RX_INVALID_HEADER;
        }
    /*----------------------------------------------------------
    get size of last packet received
    ----------------------------------------------------------*/
    *size = read_register( LORA_RX_COUNT );

    /*----------------------------------------------------------
    Clear header and rx flag
    ----------------------------------------------------------*/
    write_register( LORA_REGISTER_FLAGS, LORA_CLR_RX_FLAG );

    /*----------------------------------------------------------
    Get current fifo pointer address and the base address to
    determine how many messages were previously received
    ----------------------------------------------------------*/
    rx_fifo_ptr       = read_register( LORA_RX_CURR_ADDR );
    rx_fifo_base_addr = read_register( LORA_RX_FIFO_ADDR ); //should always be 0?   //LORA_FIFO_ADDR_PTR //this is WRONG!!!


    /*----------------------------------------------------------
    Initilize read index to the most recent message base index
    ----------------------------------------------------------*/
    fifo_read_idx = rx_fifo_ptr;

    /*----------------------------------------------------------
    Aquire total size of buffer since last read using:

    1) Last msg rx'ed starting index - rx_fifo_ptr
    2) Last msg rx'ed size           - *size
    3) fifo address since last read  - p_last_fifo_ptr

    A) If the rx_fifo_ptr and p_last_fifo_ptr are not the same
       then that means more than 1 message has been received

    B) If rx_fifo_ptr < p_last_fifo_ptr than not only has more
       than one message been received, but we rolled over the
       fifo
    ----------------------------------------------------------*/
    if( p_last_fifo_ptr != rx_fifo_ptr )
        {
        /*----------------------------------------------------------
        Check for rollover and adjust size calculation if there
        has been a rollover of the fifo address. 
        ----------------------------------------------------------*/
        if( rx_fifo_ptr < p_last_fifo_ptr )
            {
            *size = ( *size ) + ( LORA_FIFO_SIZE - p_last_fifo_ptr ) + ( rx_fifo_ptr - rx_fifo_base_addr );
            }
        else
            {
            *size = ( *size ) + ( rx_fifo_ptr - p_last_fifo_ptr );
            }
        
        /*----------------------------------------------------------
        Update the fifo pointer to point to the start of the last
        message we processed
        ----------------------------------------------------------*/
        fifo_read_idx = p_last_fifo_ptr;
        }

    /*----------------------------------------------------------
    Update last fifo pointer variable and auto adjust for fifo
    rollover
    ----------------------------------------------------------*/
    p_last_fifo_ptr = (rx_fifo_ptr + *size ) % LORA_FIFO_SIZE;

    /*----------------------------------------------------------
    Update fifo address pointer to point to the base address
    so that we can empty the buffer
    ----------------------------------------------------------*/
    write_register( LORA_FIFO_ADDR_PTR, fifo_read_idx );

    /*----------------------------------------------------------
    Verify message[] can fit message received
    ----------------------------------------------------------*/
    if( *size > size_of_message )
        {
        *error = RX_ARRAY_SIZE_ERR;
        *size = 0;
        }
    else
        {
        /*----------------------------------------------------------
        Tranfer message to array
        ----------------------------------------------------------*/
        for( i = 0; i < *size; i++ )
            {
            message[i] = read_register( LORA_REGISTER_FIFO );
            }
        }

    /*----------------------------------------------------------
    Return true for message received
    ----------------------------------------------------------*/
    return true;
    }
else
    {
    /*----------------------------------------------------------
    Return false for no message received
    ----------------------------------------------------------*/
    return false;
    }
} /* core::loraInterface::get_message() */



/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::read_register
*
*   DESCRIPTION:
*       Reads from selected register and returns value
*
*********************************************************************/
uint8_t core::loraInterface::read_register
    (
    lora_registers register_address             /* register address */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t rx_message[2];   /* receive message               */
uint8_t tx_message[2];   /* transmit message              */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
tx_message[ 0 ] = register_address;
tx_message[ 1 ] = 0x00;
rx_message[ 0 ] = 0xFF;
rx_message[ 1 ] = 0xFF;

/*----------------------------------------------------------
Transmit and Recive Request
----------------------------------------------------------*/
spi_write_read_blocking( p_spi_port, tx_message, rx_message, 2 );

/*----------------------------------------------------------
Allow time for CS to toggle
----------------------------------------------------------*/
sleep_us( 10 );

return rx_message[1];

} /* core::loraInterface::read_register() */


/*********************************************************************
*
*   PROCEDURE NAME:
*       core::loraInterface::write_register
*
*   DESCRIPTION:
*       writes to a selected register
*
*********************************************************************/
void core::loraInterface::write_register
    (
    lora_registers  register_address,           /* register address */
    uint8_t         register_data               /* register data    */
    )
{
/*----------------------------------------------------------
Local variables
----------------------------------------------------------*/
uint8_t message[2];      /* message to be passed to SPI   */

/*----------------------------------------------------------
Initilize local variables
----------------------------------------------------------*/
message[ 0 ] = ( SPI_WRITE_DATA_FLAG | register_address );
message[ 1 ] = register_data;

/*----------------------------------------------------------
Put request on SPI
----------------------------------------------------------*/
spi_write_blocking( p_spi_port, message, 2 );

/*----------------------------------------------------------
Allow time for CS to toggle
----------------------------------------------------------*/
sleep_us( 10 );

/*----------------------------------------------------------
Add delay if changing modes since this takes longer
----------------------------------------------------------*/
if ( register_address == LORA_REGISTER_OP_MODE )
	{
	sleep_ms(300);
	}

	
} /* core::loraInterface::write_register() */