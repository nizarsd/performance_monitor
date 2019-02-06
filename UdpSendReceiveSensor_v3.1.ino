/*
This version works with python script "<PROJECT_DIRECTORY>/Arduino/Python_Scripts/socketRealTimePlotV3.py"

Version History:
2.0: transmits 1 channel and the built_in ADC of the Arduino nano
3.0: transmits 4 channels also uses the ADS1115 16-bit ADC to real sesnor data 
3.1: configures the ESP as access point to connect directly with the data sink (laptop or mobile) without the need for wifi router 

*/


#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// ADC inistantiation 

Adafruit_ADS1115 ads1115;


// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif
static int OK_LED = LED_BUILTIN;
static int ESP_EN_PIN = LED_BUILTIN;

#define PACKET_SIZE  50 // 16-bit words
bool sending = false;
static int delayVal = 0;

char ssid[] = "TwimEsp";         // your network SSID (name)
char pass[] = "12345678";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

int i = 0;
unsigned int localPort = 10002;  // local port to listen on
unsigned int remoteUdpPort = 5005;   // remote port to send to

char packetBuffer[10];          // buffer to hold incoming packet

// buffers to hold outgoing packet of sensor data
uint16_t *ReplyBuffer0 = malloc(PACKET_SIZE*2+5);   
uint16_t *ReplyBuffer1 = malloc(PACKET_SIZE*2+5);   
uint16_t *ReplyBuffer2 = malloc(PACKET_SIZE*2+5);   
uint16_t *ReplyBuffer3 = malloc(PACKET_SIZE*2+5);   

WiFiEspUDP Udp;

// sensor 
int sensorPin = A0;           // select the input pin for the sensor input
int sensorValue = 0;          // variable to store the value coming from the sensor

// timer variables
long previousMillis = 0; 
long interval = 1000;  
bool check_input = false;

void printWifiStatus();


void setup() {
  pinMode(ESP_EN_PIN, OUTPUT );
  digitalWrite(ESP_EN_PIN, HIGH);

  
  // initialize serial for debugging
  Serial.begin(115200);
  // initialize serial for ESP module
  Serial1.begin(38400);
  
  // This need to be excuted by the ESP once to set the baud-rate
  // Serial1.println("AT+UART_DEF=9600,8,1,0,0");
  // delay(500);
  
  // initialize ESP module
  WiFi.init(&Serial1);

  // initialize ADS1115 ADC
  int16_t adc0, adc1, adc2, adc3;
  // ads1115.setGain(ADS1015_REG_CONFIG_PGA_2_048V);
  ads1115.begin();
   
  Serial.println("Finished initialization.");
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }


  // start access point
  status = WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK);

  Serial.println("Access point started");
  printWifiStatus();
  
 
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  Udp.begin(localPort);
  
  Serial.print("Listening on port ");
  Serial.println(localPort);

  pinMode(sensorPin, INPUT);

  
  // led is ON, wifi is connected and setup is finished successfully 
  digitalWrite(OK_LED, HIGH);
  
}



void loop()
{

 
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis > interval) 
  {
     // save the last time you blinked the LED 
     previousMillis = currentMillis;   
     check_input = true;
  }
  
  if (check_input) //  execute this every second
  {
    check_input = false;
    // if there's data available, read a packet
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      //Serial.print("From ");
      //IPAddress remoteIp = Udp.remoteIP();
      //Serial.print(remoteIp);
      //Serial.print(", port ");
      //Serial.println(Udp.remoteUdpPort());
  
      // read the packet into packetBufffer
      int len = Udp.read(packetBuffer, 10);
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      Serial.println("Contents:");
      //Serial.println(packetBuffer);
      String cmd = String (packetBuffer);
      Serial.println(cmd);
      if (cmd.equals("start"))
       {
          sending = true;
          Serial.println("Sending enabled"); 
          
       } 
       else if (cmd.equals("stop")) 
       {
          sending = false;
          i=0;
          Serial.println("Sending disabled"); 
       }
    
    }
  }
    if (sending)
    {
      if (i > PACKET_SIZE)
      { 
        Udp.beginPacket(Udp.remoteIP(), remoteUdpPort); 
        Udp.write((uint8_t *) ReplyBuffer0, (size_t) i*2);
        Udp.endPacket();
        
        Udp.beginPacket(Udp.remoteIP(), remoteUdpPort); 
        Udp.write((uint8_t *) ReplyBuffer1, (size_t) i*2);
        Udp.endPacket();
        
        Udp.beginPacket(Udp.remoteIP(), remoteUdpPort); 
        Udp.write((uint8_t *) ReplyBuffer2, (size_t) i*2);
        Udp.endPacket();
        
        Udp.beginPacket(Udp.remoteIP(), remoteUdpPort); 
        Udp.write((uint8_t *) ReplyBuffer3, (size_t) i*2);
        Udp.endPacket(); 

        i=0;
       
      }
      // Reading sensor value into buffer
      delay(delayVal);
      uint16_t  adc0 = ads1115.readADC_SingleEnded(0);
      delay(delayVal);
      uint16_t  adc1 = ads1115.readADC_SingleEnded(1);
      delay(delayVal);
      uint16_t  adc2 = ads1115.readADC_SingleEnded(2);
      delay(delayVal);
      uint16_t  adc3 = ads1115.readADC_SingleEnded(3);

      ReplyBuffer0[i] = adc0;
      ReplyBuffer1[i] = adc1;
      ReplyBuffer2[i] = adc2;
      ReplyBuffer3[i] = adc3;
   
      i++;
      //delay(delayVal);
    }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
