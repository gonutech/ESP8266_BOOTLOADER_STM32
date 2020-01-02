
#ifndef _STM32OTA_H_
#define _STM32OTA_H_
#endif
#include <Arduino.h>

#define MAX_DEVICE_SUPPORT    13

#define STERR     		"ERROR"
#define STM32INIT  		0x7F          // Send Init command
#define STM32ACK  		0x79          // return ACK answer
#define STM32NACK  		0x1F          // return NACK answer
#define STM32GET  		0             // get version command
#define STM32GVR      0x01          // get read protection status           never used in here
#define STM32ID       0x02          // get chip ID command
#define STM32RUN  		0x21          // Restart and Run programm
#define STM32RD  		  0x11          // Read flash command                   never used in here
#define STM32WR  		  0x31          // Write flash command
#define STM32ERASE    0x43          // Erase flash command
#define STM32ERASEN   0x44          // Erase extended flash command
#define STM32WP       0x63          // Write protect command                never used in here
#define STM32WP_NS    0x64          // Write protect no-stretch command     never used in here
#define STM32UNPCTWR  0x73          // Unprotect WR command                 never used in here
#define STM32UW_NS    0x74          // Unprotect write no-stretch command   never used in here
#define STM32RP       0x82          // Read protect command                 never used in here
#define STM32RP_NS    0x83          // Read protect no-stretch command      never used in here
#define STM32UR       0x92          // Unprotect read command               never used in here
#define STM32UR_NS    0x93          // Unprotect read no-stretch command    never used in here


#define STM32STADDR  	0x8000000     // STM32 codes start address, you can change to other address if use custom bootloader like 0x8002000
#define STM32ERR  		0


#define NUMBER_OF_PAGE_STM32L152ZE    1024


void stm32SendCommand(unsigned char commd);
unsigned char stm32Erase();
unsigned char stm32Erasen();
void special_earse(uint16_t ui16_number_of_pages);
unsigned char stm32Read(unsigned char * rdbuf, unsigned long rdaddress, unsigned char rdlen);
unsigned char stm32Address(unsigned long addr);
unsigned char stm32SendData(unsigned char * data, unsigned char wrlen);
unsigned char getChecksum( unsigned char * data, unsigned char datalen);
unsigned char stm32GetId();
char stm32Version();
