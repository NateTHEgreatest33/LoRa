#ifndef LORA_API_HPP
#define LORA_API_HPP

/*********************************************************************
*
*   HEADER:
*       header file for loraAPI
*
*   Copyright 2024 Nate Lenze
*
*********************************************************************/

/*--------------------------------------------------------------------
                           GENERAL INCLUDES
--------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "hardware/spi.h"
#include "console.hpp"

/*--------------------------------------------------------------------
                          LITERAL CONSTANTS
--------------------------------------------------------------------*/
#define MAX_LORA_MSG_SIZE ( 16 )   /* Buffer size on lora tranciver */

/*--------------------------------------------------------------------
                                TYPES
--------------------------------------------------------------------*/
typedef uint8_t lora_errors;       /* Error Codes                */
enum 
    {
    RX_NO_ERROR,                      /* NO RX error                */
    RX_TIMEOUT,                       /* RX timeout error           */
    RX_CRC_ERROR,                     /* RX CRC error               */
    RX_INVALID_HEADER,                /* RX Invalid header          */
    RX_ARRAY_SIZE_ERR,                /* message is too big for passed
                                         in array                   */
    }; 


typedef uint8_t lora_modes;     /* Operating Modes                  */
enum 
    {
    MODE_SLEEP,                 /* sleep mode                       */
    MODE_STBY,                  /* standby mode                     */
    MODE_FSTX,                  /* frequency sythesis transmit mode */
    MODE_TX,                    /* transmit mode                    */
    MODE_FSRX,                  /* frequency sythesis receive mode  */
    MODE_RXCONTINUOUS,          /* continious receive mode          */
    MODE_RXSINGLE,              /* single receive mode              */
    MODE_CAD                    /* preamble detect mode             */
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
    LORA_RX_COUNT         = 0x13,  /* rx byte count register        */
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

/*--------------------------------------------------------------------
                               CLASSES
--------------------------------------------------------------------*/
namespace core 
{

class loraInterface
    {
    public:
        loraInterface( spi_inst_t* spi, core::console& c_ref );
        ~loraInterface();
        bool init_tx();
        bool init_continious_rx();
        bool send_message( uint8_t Message[], uint8_t number_of_bytes );
        bool get_message( uint8_t *message, uint8_t size_of_message, uint8_t *size, lora_errors *error );
    private:
        uint8_t read_register( lora_registers register_address );
        void write_register(lora_registers  register_address, uint8_t register_data );
        spi_inst_t* p_spi_port;
        core::console& p_console;
    };

} /* core namespace */

#endif