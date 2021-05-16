/*********************************************************************
*
*   HEADER:
*       header file for loraAPI
*
*   Copyright 2020 Nate Lenze
*
*********************************************************************/

/*--------------------------------------------------------------------
                           GENERAL INCLUDES
--------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

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
    RX_DOUBLE,                        /* more than one message was 
                                         received at once           */
    RX_SIZING,                        /* less than one message was 
                                         received at once           */
    RX_KEY_ERR,                       /* Invalid key                */
    RX_INIT_ERR,                      /* Error initing rx mode      */
    SPI_ERROR                         /* SPI comm error             */
    }; 

typedef struct 
    {
    uint8_t SSI_BASE;                     /* SPI interface selected */
    uint8_t SSI_PORT;                     /* SPI pin selected       */
    uint8_t SSI_PIN;                      /* PI port selected       */             
    } lora_config;                        /* SPI interface info     */

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
loraAPI.c
--------------------------------------------------------------------*/
void lora_port_init
    (
    lora_config config_data                  /* SPI Interface info  */
    );

bool lora_init_tx
    (
    void
    );

bool lora_init_continious_rx
    (
    void
    );

bool lora_send_message
    (
    uint8_t Message[],                    /* array of bytes to send */
    uint8_t number_of_bytes               /* size of array          */
    );

bool lora_get_message
    (
    uint8_t *message,                  /* pointer to return message */
    uint8_t size_of_message,           /* array size of message[]   */
    uint8_t *size,                     /* size of return message    */
    lora_errors *error                 /* pointer to error variable */
    );

/* LoraAPI.h */
