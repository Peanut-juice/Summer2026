int laserPin = 2;
int photoPin = A0;

void setup() {
  pinMode(laserPin, OUTPUT);
  pinMode(photoPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  int value = analogRead(photoPin);
  value > 450 ? digitalWrite(laserPin, HIGH) : digitalWrite(laserPin, LOW);   
  Serial.println(value);
  delay(250);    
}