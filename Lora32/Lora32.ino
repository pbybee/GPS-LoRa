/*
 Based on https://github.com/LiveSparks/ESP32_BLE_Examples/blob/master/BLE_temperature_sensor/BLE_temperature_sensor.ino
*/

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define BAND 921.2E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define gpsServiceUUID "3076b0b4-6e45-4fec-98f6-b57848224a2e"
#define gpsCharacteristicUUID "032ce508-80dc-4bbc-a3ba-f554591161a0"
#define gpsDescriptorUUID "032ce509-80dc-4bbc-a3ba-f554591161a0"

//packet counter
int counter = 0;
bool deviceConnected = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

BLECharacteristic *pCharacteristic;

String LoRaData;

void setup() {
 //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA RECEIVER ");
  display.display();
  
  //initialize Serial Monitor
  Serial.begin(115200);

  Serial.println("LoRa Receiver Test");
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();  


  // Create the BLE Device
  BLEDevice::init("ESP32 Wheres Posey"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

   // Create the BLE Service
  BLEService *pGPS = pServer->createService(gpsServiceUUID);

  // Create a BLE Characteristic
  pCharacteristic = pGPS->createCharacteristic(
                                         gpsCharacteristicUUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->setValue("Where's Posey?");
  pGPS->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(gpsServiceUUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  // Start advertising
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
//try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    //received a packet
//    Serial.print(/"Received packet ");
    counter++;
    if (counter > 999) {
      counter = 0;
    }

    //read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      //Serial.print(LoRaData);
    }

    //print RSSI of packet
    int rssi = LoRa.packetRssi();
//    Serial.print(" with RSSI ");    
//    Serial.println(rssi);

   // Dsiplay information
   display.clearDisplay();
   display.setCursor(0,0);
   display.print("LORA RECEIVER");
   display.setCursor(0,30);
   display.print("Received packet:");
   display.print(counter);
   display.setCursor(0,40);
   display.print(LoRaData);
   display.setCursor(0,50);
   display.print("RSSI:");
   display.setCursor(30,50);
   display.print(rssi);
   display.display();  
   if (deviceConnected) {
     uint8_t toSend[LoRaData.length()];
     LoRaData.getBytes(toSend,sizeof(toSend));
     pCharacteristic->setValue(toSend,sizeof(toSend));
     pCharacteristic->notify();
     display.setCursor(0,10);
     display.setTextColor(0xFFFF,0);
     display.println("                             ");
     display.display();
     display.setTextColor(WHITE);
     display.setCursor(0,10);
     display.print("BLE CONNECTED");
     display.display();
   } else {
      BLEDevice::startAdvertising();
     display.setCursor(0,10);
     display.setTextColor(0xFFFF,0);
     display.println("                               ");
     display.display();
     display.setTextColor(WHITE);
     display.setCursor(0,10);
     display.print("BLE DISCONNECTED");
     display.display();
   }
  }
//  delay/(1000);
}
