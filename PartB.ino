#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include "SSD1306.h"

SSD1306  display(0x3c, 4, 15);

#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6

const char* ssid     = "raspberry";      //your network name
const char* password = "123456789";      //your network pass

//Définir l'adresse de cet équipement
byte localAddress = 0xAA;

//Message reçu par LoRa
String incoming = "";

//Fonction qui affiche un texte dans l'ecran OLED
void draw(String txt1, String txt2, String txt3) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, txt1);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(64, 22, txt2);
    display.drawString(128, 33, txt3);
    display.display();
}

void setup() {
    pinMode(16,OUTPUT);
    digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
    delay(50); 
    digitalWrite(16, HIGH);   // while OLED is running, must set GPIO16 in high
  
    Serial.begin(115200);
    while (!Serial); //if just the the basic function, must connect to a computer

    display.init();
    display.flipScreenVertically();
    SPI.begin(5,19,27,18);
    LoRa.setPins(SS,RST,DI0);
    
    if (!LoRa.begin(BAND)) {
      draw("Starting LoRa Receiver failed","...",":(");
      while (1);
    }
    
    draw("LoRa Receiver","Starting LoRa Success ","...");
    
    delay(1000);
    // Connection au Wifi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    // Tant que la connection n'est pas établie on reste dans la boucle
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        draw("Connecting to the Raspberry AP","..","..");
    }
  Serial.print("Connected to the WiFi network with IP: ");
  Serial.println(WiFi.localIP());
  draw("Connected to Raspberry AP","","");
  delay(2000);
}

void loop()
{
    if(WiFi.status() == WL_CONNECTED){ // Vérifier la connection wifi
      onReceive(LoRa.parsePacket());
      Serial.println("----->>"+incoming);
      
       HTTPClient http;   

       if (incoming!=""){
           http.begin("http://192.168.4.1:8090/");  //Specify destination for HTTP request
           http.addHeader("Content-Type", "text/plain");             //Specify content-type header
         
           int httpResponseCode = http.POST(incoming);   //Send the actual POST request
           Serial.println(incoming);
           http.end();  //Free resources
       }
    }
    else {
      display.clear(); 
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 10, "NOT CONNECTED :(");
      delay(3000);
      display.display();
    }
    delay(10000);
}


void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    //Serial.println("error: message length does not match length");
    draw("ERROR ::","Message length does not match length","");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress ) { //&& recipient != 0xFF) {
    draw("ERROR ::","This message is not for me.","");
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  draw("Nouveau message ! ",String(incoming),"");

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}

