#include <Arduino.h>
#include <BluetoothSerial.h>

uint32_t altura = 0;
uint16_t bateria = 1099;
uint32_t get_time = millis();
#define FREQUENCY 10 // freq output in Hz
BluetoothSerial SerialBT;

void setup() {
  delay(500);
  Serial.begin(115200);
  SerialBT.begin("ESP32");
}

void loop() {
  if (millis() >= (get_time + (1000/FREQUENCY))) {
    // notify changed value
        // Serial.println(millis()-get_time);
        get_time = millis();
        String str_out = String("LK8EX1,999999,")
        +String(altura)+",9999,999,"+String(bateria);
        
        uint16_t checksum = 0, bi;
        for (uint8_t ai = 0; ai < str_out.length(); ai++)
        {
          bi = (uint8_t)str_out[ai];
          checksum ^= bi;
        }
        str_out = "$"+str_out+"*"+String(checksum, HEX)+String("\r\n");
        char buf[100];
        str_out.toCharArray(buf,100);
        SerialBT.print(buf);
        altura++;
        bateria--;
        //Serial.println(str_out);       
        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }      
}
  