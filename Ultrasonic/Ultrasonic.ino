int pins[] = {9,10,13,7,6,5,4};
//trig, echo, led
double distance;
void setup() {
  pinMode(pins[0], OUTPUT);
  pinMode(pins[1], INPUT);
  pinMode(pins[2], OUTPUT);
  pinMode(pins[3], OUTPUT);
  pinMode(pins[4], OUTPUT);
  pinMode(pins[5], OUTPUT);
  pinMode(pins[6], OUTPUT);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(pins[0], LOW);
  delayMicroseconds(2);
  digitalWrite(pins[0], HIGH);
  delayMicroseconds(10);
  digitalWrite(pins[0], LOW);
  distance = pulseIn(pins[1], HIGH) * 0.034/2;
  Serial.println(distance);
  distance <= 20 ? digitalWrite(pins[2], HIGH) : digitalWrite(pins[2], LOW);
  distance <= 30 ? digitalWrite(pins[3], HIGH) : digitalWrite(pins[3], LOW);
  distance <= 40 ? digitalWrite(pins[4], HIGH) : digitalWrite(pins[4], LOW);
  distance <= 50 ? digitalWrite(pins[5], HIGH) : digitalWrite(pins[5], LOW);
  distance <= 60 ? digitalWrite(pins[6], HIGH) : digitalWrite(pins[6], LOW);
  delay(100);
}


// const int trigPin = 9;
// const int echoPin = 10;
// const int ledPin = 13;
// long duration;
// float distance;
// void setup() {
//   pinMode(trigPin, OUTPUT);
//   pinMode(echoPin, INPUT);
//   pinMode(ledPin, OUTPUT);
//   Serial.begin(9600);
// }
// void loop() {
//   digitalWrite(trigPin, LOW);
//   delayMicroseconds(2);
//   digitalWrite(trigPin, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPin, LOW);
//   duration = pulseIn(echoPin, HIGH);
//   distance = duration * 0.034/2;
//   Serial.println(distance);
//   delay(100);
// }
