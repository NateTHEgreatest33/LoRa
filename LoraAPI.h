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

/*--------------------------------------------------------------------
                          LITERAL CONSTANTS
--------------------------------------------------------------------*/

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
    SPI_ERROR                         /* SPI comm error             */
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
loraAPI.c
--------------------------------------------------------------------*/
bool lora_init_tx
    (
    void
    );

bool lora_send_message
    (
    uint8_t Message[],                    /* array of bytes to send */
    uint8_t number_of_bytes               /* size of array          */
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
    uint8_t *message[],                /* pointer to return message */
    uint8_t size_of_message,           /* array size of message[]   */
    uint8_t *size,                     /* size of return message    */
    lora_errors *error                 /* pointer to error variable */
    );

/* LoraAPI.h */
