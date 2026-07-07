int redLED = 13, yellowLED = 12, greenLED = 11, blueLED = 10, whiteLED = 9;

void setup() {
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(whiteLED, OUTPUT);
}

void loop() {
  digitalWrite(greenLED, HIGH);
  digitalWrite(yellowLED, LOW);
  digitalWrite(redLED, LOW);
  delay(5000);

  digitalWrite(greenLED, LOW);
  digitalWrite(yellowLED, HIGH);
  digitalWrite(redLED, LOW);
  delay(2000);

  digitalWrite(greenLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(redLED, HIGH);
  delay(5000);
  flash();
}

void flash() {
  digitalWrite(whiteLED, LOW);
  digitalWrite(blueLED, HIGH);
  delay(100);
  digitalWrite(whiteLED, HIGH);
  digitalWrite(blueLED, LOW);
  delay(100);
}