
#include "module_stm32ota.h"
#include "module_SoftwareSerial.h"

extern SoftwareSerial mySerial;

void stm32SendCommand(unsigned char commd)
{ // Tested
  Serial.write(commd);
  Serial.write(~commd);
}

unsigned char stm32Erase()
{ // Tested
  int temp = 0;
  mySerial.println("stm32Erase");
  stm32SendCommand(STM32ERASE);
  //stm32SendCommand(0xBC);
  while (!Serial.available())
  {

  } 
  temp = Serial.read();
  mySerial.println(temp, HEX);
  if (temp == STM32ACK)
  {
    mySerial.println("stm32 erase STM32ACK");
    Serial.write(0xFF);
    Serial.write(0x00);
  }
  else
  {
    mySerial.println("stm32 erase error");
    return STM32ERR;
  }
  while (!Serial.available())
  {
  }
  return Serial.read();
}

unsigned char stm32Erasen()
{
  int temp = 0; // Tested
  mySerial.println("stm32Erasen");
  stm32SendCommand(STM32ERASEN);
  while (!Serial.available())
  {

  } 
  temp = Serial.read();
  mySerial.println(temp, HEX);
  if (temp == STM32ACK)
  {
    mySerial.println("STM32ACK");
    
    /*Serial.write(0xFF);
    Serial.write(0xFF);
    Serial.write(0x00);
    */
    special_earse(NUMBER_OF_PAGE_STM32L152ZE);
  }
  else
  {
    mySerial.println("STM32ERR");
    return STM32ERR;
  }
  while (!Serial.available())
  {

  } 
  temp = Serial.read();
  mySerial.println(temp, HEX);
  return temp;
}

void special_earse(uint16_t ui16_number_of_pages)
{
  mySerial.println("special_earse");
  uint16_t i = 0;
  uint8_t ui8_msb = 0;
  uint8_t ui8_lsb = 0;
  uint8_t ui8_addcheck = 0;  
  uint16_t ui16_temp = 0;
  ui16_temp = ui16_number_of_pages - 1;
  ui8_msb = ui16_temp>>8;
  ui8_lsb = ui16_temp; 
  Serial.write(ui8_msb);
  Serial.write(ui8_lsb);
  ui8_addcheck ^= ui8_msb;
  ui8_addcheck ^= ui8_lsb;
  mySerial.println(ui8_msb,HEX);
  mySerial.println(ui8_lsb,HEX);
  for(i = 0;i<ui16_number_of_pages;i++)
  {
    ui8_msb = i>>8;
    ui8_lsb = i; 
    Serial.write(ui8_msb);
    Serial.write(ui8_lsb);
    ui8_addcheck ^= ui8_msb;
    ui8_addcheck ^= ui8_lsb;
    mySerial.println(ui8_msb,HEX);
    mySerial.println(ui8_lsb,HEX);
  }
  Serial.write(ui8_addcheck);   
  mySerial.println(ui8_addcheck,HEX); 
}
// No test yet
unsigned char stm32Run()
{
  stm32SendCommand(STM32RUN);
  while (!Serial.available())
  {

  } 
  if (Serial.read() == STM32ACK)
  {
    stm32Address(STM32STADDR);
    return STM32ACK;
  }
  else
  {
    return STM32ERR;
  }
}

// No test yet
unsigned char stm32Read(unsigned char *rdbuf, unsigned long rdaddress, unsigned char rdlen)
{
  stm32SendCommand(STM32RD);
  while (!Serial.available())
   {

  } 
  if (Serial.read() == STM32ACK)
  {
    stm32Address(rdaddress);
  }
  else
  {
    return STM32ERR;
  }
  while (!Serial.available())
  {

  } 
  if (Serial.read() == STM32ACK)
  {
    stm32SendCommand(rdlen);
  }
  while (!Serial.available())
   {

  } 
  size_t getlen = Serial.available();
  Serial.readBytes(rdbuf, getlen);
  return STM32ACK;
}

unsigned char stm32Address(unsigned long addr)
{ // Tested
  unsigned char sendaddr[4];
  unsigned char addcheck = 0;
  sendaddr[0] = addr >> 24;
  sendaddr[1] = (addr >> 16) & 0xFF;
  sendaddr[2] = (addr >> 8) & 0xFF;
  sendaddr[3] = addr & 0xFF;
  for (int i = 0; i <= 3; i++)
  {
    Serial.write(sendaddr[i]);
    addcheck ^= sendaddr[i];
  }
  Serial.write(addcheck);
  while (!Serial.available())
  {

  } 
  return Serial.read();
}

unsigned char stm32SendData(unsigned char *data, unsigned char wrlen)
{ // Tested
  Serial.write(wrlen);
  for (int i = 0; i <= wrlen; i++)
  {
    Serial.write(data[i]);
  }
  Serial.write(getChecksum(data, wrlen));
  while (!Serial.available())
   {

  } 
  return Serial.read();
}

char stm32Version()
{ // Tested
  unsigned char vsbuf[14];
  stm32SendCommand(STM32GET);
  while (!Serial.available())
  {

  }  
  vsbuf[0] = Serial.read();
  if (vsbuf[0] != STM32ACK)
  {
    return STM32ERR;
  }
  else
  {
    Serial.readBytesUntil(STM32ACK, vsbuf, 14);
    return vsbuf[1];
  }
}

unsigned char stm32GetId()
{ // Tested
  int getid = 0;
  unsigned char sbuf[5];
  stm32SendCommand(STM32ID);
  mySerial.println("Get id");
  while (!Serial.available())
    ;
  sbuf[0] = Serial.read();
  mySerial.println(sbuf[0], HEX);
  //Serial.println(sbuf[0],HEX);
  if (sbuf[0] == STM32ACK)
  {
    mySerial.println("Get id STM32ACK");
    Serial.readBytesUntil(STM32ACK, sbuf, 4);
    mySerial.println(sbuf[1], HEX);
    mySerial.println(sbuf[2], HEX);
    //Serial.println(sbuf[1],HEX);
    //Serial.println(sbuf[2],HEX);
    getid = sbuf[1];
    getid = (getid << 8) + sbuf[2];
    mySerial.println(getid, HEX);
    if (getid == 0x444)
    {
      return 1;
    }
    if (getid == 0x440)
    {
      return 2;
    }
    if (getid == 0x442)
    {
      return 3;
    }
    if (getid == 0x412)
    {
      return 4;
    }
    if (getid == 0x410)
    {
      return 5;
    }
    if (getid == 0x414)
    {
      return 6;
    }
    if (getid == 0x418)
    {
      return 7;
    }
    if (getid == 0x427)
    {
      mySerial.println("get  id = 8");
      return 8;
    }
  }
  else
  {
    mySerial.println("return 0");
    return 0;
  }
}

unsigned char getChecksum(unsigned char *data, unsigned char datalen)
{ // Tested
  unsigned char lendata = datalen;
  for (int i = 0; i <= datalen; i++)
  {
    lendata ^= data[i];
  }
  return lendata;
}
