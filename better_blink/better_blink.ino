#define interval 500
#define ledPin 13

unsigned long time = millis(), previous = 0;
int ledState = LOW;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  time = millis();
  if (time - previous >= interval) {
    previous = time;
    ledState == LOW ? ledState = HIGH : ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}