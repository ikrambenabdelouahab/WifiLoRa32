#include <MQ2.h>
#include "DHT.h"
#include <SPI.h>
#include <LoRa.h>
#include <Arduino.h>
#include <Wire.h>
#include "SSD1306.h"

#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6

SSD1306  display(0x3c, 4, 15);

#define DHTPIN 23           // Pin Digital où le capteur DHT11 est branché
#define DHTTYPE DHT11       // DHT11 ou DHT22  
DHT dht(DHTPIN, DHTTYPE);
int t, h;                   //La température et l'humidité donnée par DHT11  

int pin = 12;               //Pin analogic où le capteur MQ2 est branché
int lpg, co, smoke;         //Les gas que le capteur va detecter

MQ2 mq2(pin);


byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination  = 0xAA;      // destination to send to
//Le message envoyé par LoRa
String msg = "";

//Fonction qui affiche un texte dans l'ecran OLED
void draw(String txt1, String txt2, String txt3) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    // The coordinates define the left starting point of the text
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, txt1);
    // The coordinates define the right end of the text
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(64, 22, txt2);
    display.drawString(128, 33, txt3);
    display.display();
}


void setup(){
  //Allumer OLED
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH);   // while OLED is running, must set GPIO16 in high
  
  Serial.begin(115200);
  while (!Serial); //If just the the basic function, must connect to a computer

  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    draw("Starting LoRa failed !",":(","");
    while (1);
  }
  draw("LoRa Sender","LoRa Initial OK!","");
  delay(1000);
  
  //Lancer les capteurs
  dht.begin();
  mq2.begin();
}

void loop(){
  //Vider le message
  msg = "";
  
  //Récuperer l'humidité et la temperature
  t = dht.readTemperature();
  h = dht.readHumidity();
  Serial.println("T="+String(t)+" ; H="+String(h));

  //Récuperer les valeurs des Gas capter
  lpg = mq2.readLPG();
  co = mq2.readCO();
  smoke = mq2.readSmoke();
  
  //Eviter les valers aléatoire
  if (lpg >10000) lpg=0;
  if (co > 10000) co=0;
  if (smoke > 10000) smoke=0;

   float* values= mq2.read(true);
  //Serial.println(String(lpg)+","+String(co)+","+String(smoke));
  Serial.println("LPG="+String(lpg)+" ; CO="+String(co)+" ; SMOKE="+String(smoke));

  //Construction du message à envoyer 
  msg = "" + String(t)+","+String(h)+","+String(lpg)+","+String(co)+","+String(smoke);
  Serial.println("MESSAGE:: "+msg);

  sendMessage(msg);
  draw("LoRa Send msg N° "+String(msgCount), "" , msg );
  
  //Capter et envoyer les données chaque 5 secondes   
  delay(5000);
}

//Envoie du packet LoRa
void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

