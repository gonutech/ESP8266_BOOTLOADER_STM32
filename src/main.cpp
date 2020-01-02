/*
*   ESP8266/8285 Pin      STM32 MCU      NodeMCU Pin(ESP8266 based)
*   RXD                  	PA9             RXD
*   TXD                  	PA10            TXD
*   Pin4                 	BOOT0           D2
*   Pin5                 	RST             D1
*   Vcc                  	3.3V            3.3V
*   GND                  	GND             GND
*   En -> 10K -> 3.3V
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "main.h"
#include "module_FS.h"
#include "module_stm32ota.h"
#include "module_SoftwareSerial.h"
#include "module_debug.h"


const String STM32_CHIPNAME[MAX_DEVICE_SUPPORT] =
{
  "Unknown Chip",
  "STM32F03xx4/6",
  "STM32F030x8/05x",
  "STM32F030xC",
  "STM32F103x4/6",
  "STM32F103x8/B",
  "STM32F103xC/D/E",
  "STM32F105/107",
  "STM32L152RCT6"
};

#define PIN_NRST    5
#define PIN_BOOT0   4
#define PIN_LED     2


IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 9);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
const char *serverIndex = "<h1>Upload STM32 BinFile</h1><h2><br><br><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Upload'></form></h2>";
File fsUploadFile;
uint8_t binread[256];
int bini = 0;
String stringtmp;
int rdtmp;
int stm32ver;
bool initflag = 0;
bool Runflag = 0;

String makePage(String title, String contents);
void RunMode();
void FlashMode();

SoftwareSerial mySerial(PIN_UART_RX_DEBUG, PIN_UART_TX_DEBUG, false);

void handleFlash()
{
  String FileName, flashwr;
  int lastbuf = 0;
  uint8_t cflag, fnum = 256;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    FileName = dir.fileName();
  }
  fsUploadFile = SPIFFS.open(FileName, "r");
  if (fsUploadFile)
  {
    bini = fsUploadFile.size() / 256;
    lastbuf = fsUploadFile.size() % 256;
    flashwr = String(bini) + "-" + String(lastbuf) + "<br>";
    for (int i = 0; i < bini; i++)
    {
      fsUploadFile.read(binread, 256);
      stm32SendCommand(STM32WR);
      while (!Serial.available())
        ;
      cflag = Serial.read();
      if (cflag == STM32ACK)
      {
        if (stm32Address(STM32STADDR + (256 * i)) == STM32ACK)
        {
          if (stm32SendData(binread, 255) == STM32ACK)
          {
            flashwr += ".";
          }
          else
          {
            flashwr = "Error";
          }
        }
      }
    }
    fsUploadFile.read(binread, lastbuf);
    stm32SendCommand(STM32WR);
    while (!Serial.available())
      ;
    cflag = Serial.read();
    if (cflag == STM32ACK)
    {
      if (stm32Address(STM32STADDR + (256 * bini)) == STM32ACK)
      {
        if (stm32SendData(binread, lastbuf) == STM32ACK)
        {
          flashwr += "<br>Finished<br>";
        }
        else
        {
          flashwr = "Error";
        }
      }
    }   
    fsUploadFile.close();
    String flashhtml = "<h1>Programming</h1><h2>" + flashwr + "<br><br><a style=\"color:white\" href=\"/up\">Upload STM32 BinFile</a><br><br><a style=\"color:white\" href=\"/list\">List STM32 BinFile</a></h2>";
    server.send(200, "text/html", makePage("Flash Page", flashhtml));
  }
}

void handleFileUpload()
{
  if (server.uri() != "/upload")
  {
    return;
  }    
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
    {
      filename = "/" + filename;
    }

    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
    {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {
      fsUploadFile.close();
    }
  }
}

void handleFileDelete()
{  
  String FileList = "File: ";
  String FName;
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    FName = dir.fileName();
  }
  FileList += FName;
  if (SPIFFS.exists(FName))
  {
    server.send(200, "text/html", makePage("Deleted", "<h2>" + FileList + " be deleted!<br><br><a style=\"color:white\" href=\"/list\">Return </a></h2>"));
    SPIFFS.remove(FName);
  }
  else
    return server.send(404, "text/html", makePage("File Not found", "404"));
}

void handleListFiles()
{
  String FileList = "Bootloader Ver: ";
  String Listcode;
  char blversion = 0;
  Dir dir = SPIFFS.openDir("/");
  blversion = stm32Version();
  FileList += String((blversion >> 4) & 0x0F) + "." + String(blversion & 0x0F) + "<br> MCU: ";
  FileList += STM32_CHIPNAME[stm32GetId()];
  FileList += "<br><br> File: ";
  while (dir.next())
  {
    String FileName = dir.fileName();
    File f = dir.openFile("r");
    String FileSize = String(f.size());
    int whsp = 6 - FileSize.length();
    while (whsp-- > 0)
    {
      FileList += " ";
    }
    FileList += FileName + "   Size:" + FileSize;
  }
  Listcode = "<h1>List STM32 BinFile</h1><h2>" + FileList + "<br><br><a style=\"color:white\" href=\"/flash\">Flash Menu</a><br><br><a style=\"color:white\" href=\"/delete\">Delete BinFile </a><br><br><a style=\"color:white\" href=\"/up\">Upload BinFile</a></h2>";
  server.send(200, "text/html", makePage("FileList", Listcode));
}

void setup(void)
{
  SPIFFS.begin();
  Serial.begin(UART0_BAUD_RATE, SERIAL_8E1);
  mySerial.begin(UART_DEBUG_BAUD_RATE);
  delay(1000);
  pinMode(PIN_BOOT0, OUTPUT);
  pinMode(PIN_NRST, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  WiFi.begin(SSID_WIFI_STA, PASS_WIFI_STA);
  delay(100);
  digitalWrite(PIN_BOOT0, HIGH);
  delay(100);
  digitalWrite(PIN_NRST, LOW);
  digitalWrite(PIN_LED, LOW);
  delay(50);
  digitalWrite(PIN_NRST, HIGH);
  delay(500);
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    delay(100);
  }
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { 
    // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {
    Serial.println("WL_CONNECTED");
    server.on("/up", HTTP_GET, []() {
      server.send(200, "text/html", makePage("Select file", serverIndex));
    });
    server.on("/list", HTTP_GET, handleListFiles);
    server.on("/programm", HTTP_GET, handleFlash);
    server.on("/run", HTTP_GET, []() 
    {
      String Runstate = "STM32 Restart and runing!<br><br> you can reflash MCU (click 1.FlashMode before return Home) <br><br> Or close Browser";     
      if (Runflag == 0)
      {
        RunMode();
        Runflag = 1;
      }
      else
      {
        FlashMode();       
        Runflag = 0;
      }
      server.send(200, "text/html", makePage("Run", "<h2>" + Runstate + "<br><br><a style=\"color:white\" href=\"/run\">1.FlashMode </a><br><br><a style=\"color:white\" href=\"/\">2.Home </a></h2>"));
    });
    Serial.println("ABC");
    server.on("/erase", HTTP_GET, []() {
      if (stm32Erase() == STM32ACK)
      {
        stringtmp = "<h1>Erase OK</h1><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>";
      }
      else if (stm32Erasen() == STM32ACK)
      {
        stringtmp = "<h1>Erase OK</h1><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>";
      }
      else
      {
        stringtmp = "<h1>Erase failure</h1><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>";
      }

      server.send(200, "text/html", makePage("Erase page", stringtmp));
    });
    server.on("/flash", HTTP_GET, []() {
      stringtmp = "<h1>FLASH MENU</h1><h2><a style=\"color:white\" href=\"/programm\">Flash STM32</a><br><br><a style=\"color:white\" href=\"/erase\">Erase STM32</a><br><br><a style=\"color:white\" href=\"/run\">Run STM32</a><br><br><a style=\"color:white\" href=\"/list\">Return </a></h2>";
      server.send(200, "text/html", makePage("Flash page", stringtmp));
    });
    server.on("/delete", HTTP_GET, handleFileDelete);
    server.onFileUpload(handleFileUpload);
    server.on("/upload", HTTP_POST, []() {
      server.send(200, "text/html", makePage("FileList", "<h1> Uploaded OK </h1><br><br><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>"));
    });
    server.on("/", HTTP_GET, []() 
    {
      if (Runflag == 1)
      {       
        FlashMode();
        Runflag = 0;
      }
   
      Serial.write(STM32INIT);
      delay(10);    
      while (!Serial.available())
      {

      }       
      rdtmp = Serial.read();
      mySerial.println(rdtmp, HEX);     
      if (rdtmp == STM32ACK)
      {      
        mySerial.println("STM32 ACK");
        stringtmp = STM32_CHIPNAME[stm32GetId()];
        mySerial.println(stringtmp);
      }
      else if (rdtmp == STM32NACK)
      {
        mySerial.println("STM32 NACK");       
        Serial.write(STM32INIT);    
        while (!Serial.available())
        {

        }        
        rdtmp = Serial.read();         
        if (rdtmp == STM32ACK)
        {          
          stringtmp = STM32_CHIPNAME[stm32GetId()];
        }
      }
      else
      {        
        stringtmp = "ERROR";
      }     
      String starthtml = "<h1>STM32-OTA</h1><h2>Version 1.0 by <a style=\"color:white\" href=\"https://github.com/hongthiet1994\">Embedded system channel<br><br><a style=\"color:white\" href=\"/up\">Upload STM32 BinFile </a><br><br><a style=\"color:white\" href=\"/list\">List STM32 BinFile</a></h2>";
      server.send(200, "text/html", makePage("Start Page", starthtml + "- Init MCU -<br> " + stringtmp));
    });

    server.begin();
  }
}

void loop(void)
{
  server.handleClient();
}

String makePage(String title, String contents)
{
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title >";
  s += title;
  s += "</title></head><body text=#ffffff bgcolor=##4da5b9 align=\"center\">";
  s += contents;
  s += "</body></html>";
  return s;
}

void FlashMode()
{ 
  //Tested  Change to flashmode
  digitalWrite(PIN_BOOT0, HIGH);
  delay(100);
  digitalWrite(PIN_NRST, LOW);
  digitalWrite(PIN_LED, LOW);
  delay(50);
  digitalWrite(PIN_NRST, HIGH);
  delay(200);
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    delay(100);
  }
}

void RunMode()
{
  //Tested  Change to runmode
  digitalWrite(PIN_BOOT0, LOW);
  delay(100);
  digitalWrite(PIN_NRST, LOW);
  digitalWrite(PIN_LED, LOW);
  delay(50);
  digitalWrite(PIN_NRST, HIGH);
  delay(200);
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    delay(100);
  }
}
