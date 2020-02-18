/* -----------------------------------------------------------------
 *			   Includes
 * -------------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"

/* -----------------------------------------------------------------
 *			   Defintions
 * -------------------------------------------------------------- */
#define CONFIG_REG_ADDR 0x01
#define LORA_FIFO_ADDR 0x00

/* -----------------------------------------------------------------
 *			   Variables
 * -------------------------------------------------------------- */
enum modes{SLEEP, STBY, FSTX, TX, FSRX, RXCONTINUOUS, RXSINGLE, CAD}; 

/* -----------------------------------------------------------------
 *			   Functions
 * -------------------------------------------------------------- */
/* -----------------------------------------------------------------
LoRaRead(): Reads from selected register and returns value
Inputs:
- (uint8_t) RegisterAddress_8bit: value of register you wish to read from
Outputs:
- (uint8_t) Value of register being read from
Notes:
- uses PA3 as Chip select manually, PA3 must be configured as GPIO output pin
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
uint32_t loRaRead(uint8_t RegisterAddress_8bit){
	uint32_t MessageReturn = 0;
	uint8_t clearFIFO = 0xFF;
	uint32_t JUNKDATA = 0;
    //clear FIFO
	while(clearFIFO != 0x00){
		clearFIFO = SSIDataGetNonBlocking(SSI0_BASE,&JUNKDATA);
	}
	GPIO_PORTA_DATA_R &= ~(0x8);//~(1<<3);//toggle A3 low
	SSIDataPut(SSI0_BASE, RegisterAddress_8bit);//put register value
	while(SSIBusy(SSI0_BASE)){}	//wait for data put
	SSIDataPut(SSI0_BASE, 0x00);//put blank data for timing
	SSIDataGet(SSI0_BASE, &MessageReturn);//get junk message
	while(SSIBusy(SSI0_BASE)){}//wait for data put
	SSIDataGet(SSI0_BASE, &MessageReturn);//get actual message
	//for(i = 0; i<300000;i++){}
	GPIO_PORTA_DATA_R |= 0x08;//toggle A3 high
	return MessageReturn;
}

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
void loRaWrite(uint8_t RegisterAddress_8bit, volatile uint8_t data8Bit){
	uint8_t clearFIFO = 0xFF;
	uint32_t JUNKDATA = 0;
    //clear FIFO of junk data
	while(clearFIFO != 0x00){                               
		clearFIFO = SSIDataGetNonBlocking(SSI0_BASE,&JUNKDATA);
	}
    
	GPIO_PORTA_DATA_R &= ~(0x8); //~(1<<3);//toggle A3 low
	SSIDataPut(SSI0_BASE, (0x80 | RegisterAddress_8bit));//put register value (OR w/ x80 for write)
	while(SSIBusy(SSI0_BASE)){} //wait for data put
	SSIDataPut(SSI0_BASE, data8Bit);//put blank data for timing
	while(SSIBusy(SSI0_BASE)){}//wait for data put
	GPIO_PORTA_DATA_R |= 0x08;//toggle A3 high
}

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
bool LoraInitTx(void){
	uint8_t configRegisterData = 0x80;
	uint8_t TxFifoInfo = 0x00;
	uint8_t highPowerMode = 0x00;
	loRaWrite(CONFIG_REG_ADDR, 0x80); //write 0x80 to config register
	//0x80 = sleep mode, HighFreq mode, LoraMode
	configRegisterData = loRaRead(CONFIG_REG_ADDR); 
	if(configRegisterData != 0x80){
		return false;
	}
	//highPower TX mode
	highPowerMode = loRaRead(0x09);
	loRaWrite(0x09, 0xFF);
	highPowerMode = loRaRead(0x09);
	if(highPowerMode != 0xFF){
		return false;
	}
	//make sure Tx Fifo is setup correctly
	TxFifoInfo = loRaRead(0x0D); //Base
	TxFifoInfo = loRaRead(0x0E); //0x80 (TX fifo Base)
	loRaWrite(0x0D,TxFifoInfo);//point 0x00 FIFO to TX fifo Base
	TxFifoInfo = loRaRead(0x0D); //verify changes
	if(TxFifoInfo != 0x80){
		return false;
	}
	return true;
}

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
bool LoraInitRx(void){
	uint8_t configRegisterData = 0x80;
	uint8_t RxFifoInfo = 0x00;
	uint8_t highPowerMode = 0x00;
	loRaWrite(CONFIG_REG_ADDR, configRegisterData); //write 0x80 to config register
	//0x80 = sleep mode, HighFreq mode, LoraMode
	configRegisterData = loRaRead(CONFIG_REG_ADDR); 
	if(configRegisterData != 0x80){
		return false;
	}
	//highPower TX mode
	highPowerMode = loRaRead(0x09);
	loRaWrite(0x09, 0xFF);
	highPowerMode = loRaRead(0x09);
	if(highPowerMode != 0xFF){
		return false;
	}
  	//setup Rx fifoPtr
	RxFifoInfo = loRaRead(0x0F); //Rx FIFO base addrss (should be 0x00)
	loRaWrite(0x0D,RxFifoInfo);//0x00 is the start of RX buffer, 0x0D is the fifoPtr by default
	RxFifoInfo = loRaRead(0x0D); //verify changes
	if(RxFifoInfo != 0x00){
		return false;
	}
	return true;
}

/* -----------------------------------------------------------------
 * LoRaMode(): sets configuration register to specificed mode
 * Inputs:
 *   - (uint8_t) mode: what mode to set, (See enum 'modes' at top of file)
 * Outputs:
 *   - (bool) successfull or not
 * Notes:
 *   - uses LoRaRead() and LoRaWrite() so their requirements must be met
 * -------------------------------------------------------------- */
bool LoRaMode(uint8_t mode){
	uint8_t configReg = 0x00;
	if(mode == SLEEP){
		configReg = loRaRead(0x01);
		configReg &= ~(0x7);
		loRaWrite(0x01, configReg);
		if(loRaRead(0x01) != configReg){
			return false;
		}
		return true;
	}else if(mode == STBY){
		configReg = loRaRead(0x01);
		configReg &= ~(0x7);
		configReg |= 0x01;
		loRaWrite(0x01, configReg);
		if(loRaRead(0x01) != configReg){
			return false;
		}
		return true;
	}else if(mode == TX){
		configReg = loRaRead(0x01);
		configReg &= ~(0x7);
		configReg |= 0x03;
		loRaWrite(0x01, configReg);
		if(loRaRead(0x01) != configReg){
			return false;
		}
		return true;
	}else if(mode == RXCONTINUOUS){
		configReg = loRaRead(0x01);
		configReg &= ~(0x7);
		configReg |= 0x05;
		loRaWrite(0x01, configReg);
		if(loRaRead(0x01) != configReg){
			return false;
		}
		return true;
	}else if(mode == RXSINGLE){
		configReg = loRaRead(0x01);
		configReg &= ~(0x7);
		configReg |= 0x06;
		loRaWrite(0x01, configReg);
		if(loRaRead(0x01) != configReg){
			return false;
		}
		return true;
	}else{
		return false;
	}
}

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
bool LoraSendMessage(uint8_t Message[], uint8_t number_of_bytes){
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



