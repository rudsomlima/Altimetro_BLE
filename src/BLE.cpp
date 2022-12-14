/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "MS5611.h"
#define I2C_SDA 21
#define I2C_SCL 22

MS5611 MS5611(0x77);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

uint32_t pressao = 0;
uint16_t temperatura = 0;
uint32_t altitude = 0;
uint16_t bateria = 10100;
uint32_t get_time = millis();
#define FREQUENCY 10 // freq output in Hz
// #define BLUETOOTH_SPEED 9600 //bluetooth speed (9600 by default)

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  delay(500);
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.begin(115200);
  esp_err_t esp_wifi_init ();  //corrige o problema do BLE não inicializar
 
  if (MS5611.begin() == true)
  {
    Serial.println("MS5611 found.");
  }
  else
  {
    Serial.println("MS5611 not found. halt.");
    while (1);
  }
  Serial.println();
  MS5611.setOversampling(OSR_ULTRA_HIGH);
  MS5611.setTemperatureOffset(-1);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::READ   |
                      NIMBLE_PROPERTY::WRITE  |
                      NIMBLE_PROPERTY::NOTIFY |
                      NIMBLE_PROPERTY::INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  // pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (millis() >= (get_time + (1000/FREQUENCY))) {
    // notify changed value
      if (deviceConnected) {
        // Serial.println(millis()-get_time);
        get_time = millis();
        // LK8EX1,pressure,altitude,vario,temperature,battery,*checksum
        // bool dataready = MSXX.doBaro(true); //Calculate pressure and temperature, boolean for altitude estimation from sea level
        float temp=0;
        float pressure = 0;
        MS5611.read();   
        temp = MS5611.getTemperature();
        pressure = MS5611.getPressure()*100; //mBar para Pascal;
        Serial.println(temp);
        Serial.println(MS5611.getPressure());
        temperatura = uint16_t(temp)*100; 
        pressao = uint32_t(pressure);
        altitude = 99999;  //nao utilizado por isso 99999
        String str_out = String("LK8EX1,")+pressao+","
        +altitude+ "," + "9999," + temperatura + ","+bateria;
        
        uint16_t checksum = 0, bi;
        for (uint8_t ai = 0; ai < str_out.length(); ai++)
        {
          bi = (uint8_t)str_out[ai];
          checksum ^= bi;
        }
        str_out = "$"+str_out+"*"+String(checksum, HEX)+String("\r\n");

        char buf[100];
        str_out.toCharArray(buf,100);
        pCharacteristic->setValue(buf);
        pCharacteristic->notify();
        bateria--;
        Serial.println(str_out);       
        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
      }
      // disconnecting
      if (!deviceConnected && oldDeviceConnected) {
          delay(500); // give the bluetooth stack the chance to get things ready
          pServer->startAdvertising(); // restart advertising
          Serial.println("start advertising");
          oldDeviceConnected = deviceConnected;
      }
      // connecting
      if (deviceConnected && !oldDeviceConnected) {
          // do stuff here on connecting
          oldDeviceConnected = deviceConnected;
      }
  }
  
}