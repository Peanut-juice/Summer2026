#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print(F("Humidity: "));
  Serial.print(h);
  (h < 30 || h > 50) ? digitalWrite(12, HIGH) : digitalWrite(12, LOW);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C"));
  (t < 25 || t > 37.5) ? digitalWrite(13, HIGH) : digitalWrite(13, LOW);
  (digitalRead(13) == HIGH && digitalRead(12) == HIGH) ? tone(8, 2000) : noTone(8);
  delay(1000);
}
