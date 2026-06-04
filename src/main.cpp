#include <Arduino.h>

#define SENSOR_PIN 34

void setup() {
  Serial.begin(115200);
}

void loop() {

  int sensorValue = analogRead(SENSOR_PIN);

  Serial.print("Nilai Sensor: ");
  Serial.println(sensorValue);

  if(sensorValue > 1900){
    Serial.println("Status: BERSIH");
  }
  else if(sensorValue > 1850){
    Serial.println("Status: KERUH");
  }
  else{
    Serial.println("Status: KOTOR");
  }

  Serial.println("----------------");

  delay(1000);
}