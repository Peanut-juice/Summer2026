int pins[] = {9,6,5,3};
const int center = 512;
const int deadZone = 20;
void setup() {
  pinMode(9, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(3, OUTPUT);
}

void loop() {
  int x = analogRead(A0);
  int y = analogRead(A1);
  int xDistance = abs(x - center);
  int yDistance = abs(y - center);
  analogWrite(3, 0);
  analogWrite(5, 0);
  analogWrite(6, 0);
  analogWrite(9, 0);
  if (x > center + deadZone) {
    analogWrite(3, map(x, center + deadZone, 1023, 0, 255));
  }
  else if (x < center - deadZone) {
    analogWrite(6, map(x, center - deadZone, 0, 0, 255));
  }
  if (y > center + deadZone) {
    analogWrite(9, map(y, center + deadZone, 1023, 0, 255));
  }
  else if (y < center - deadZone) {
    analogWrite(5, map(y, center - deadZone, 0, 0, 255));
  }
}
