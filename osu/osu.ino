// #include "merrychristmas.ino"
//------------------------------------

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0


// change this to whichever pin you want to use
int buzzer = A5;
// change this to make the song slower or faster
int tempo = 160;


// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {

  // We Wish You a Merry Christmas
  // Score available at https://musescore.com/user/6208766/scores/1497501
  
  NOTE_C5,4, //1
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,

  NOTE_F5,2, NOTE_C5,4, //8 
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4,

  NOTE_F5,4, NOTE_F5,4, NOTE_F5,4,//17
  NOTE_E5,2, NOTE_E5,4,
  NOTE_F5,4, NOTE_E5,4, NOTE_D5,4,
  NOTE_C5,2, NOTE_A5,4,
  NOTE_AS5,4, NOTE_A5,4, NOTE_G5,4,
  NOTE_C6,4, NOTE_C5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4, 
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8, //27
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4,
  NOTE_F5,4, NOTE_F5,4, NOTE_F5,4,
  NOTE_E5,2, NOTE_E5,4,
  NOTE_F5,4, NOTE_E5,4, NOTE_D5,4,
  
  NOTE_C5,2, NOTE_A5,4,//36
  NOTE_AS5,4, NOTE_A5,4, NOTE_G5,4,
  NOTE_C6,4, NOTE_C5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4, 
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8, 
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,//45
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4,
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, //53
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, REST,4
};

const int rows = 3;
const int columns = 3;

// Top row to bottom row, left to right in each row.
const int ledGrid[rows][columns] = {
  {7, 10, 13},
  {6, 9, 12},
  {5, 8, 11}
};

const int buttonPins[columns] = {A0, A1, A2};

// 0 = left, 1 = center, 2 = right, -1 = no playable note.
const int trackLength = 32;
const int noteSequence[trackLength] = {
  0, 1, 2, 1,
  0, 1, 2, 1,
  0, 1, 2, 1,
  0, 1, 2, 1,
  0, 1, 2, 1,
  0, 1, 2, 1,
  0, 1, 2, 1,
  0, 1, 2, 1
};

const int melodyNoteCount = sizeof(melody) / sizeof(melody[0]) / 2;
const int wholenote = (60000 * 4) / tempo;

const unsigned long startDelay = 2000;
const unsigned long rowSpeed = 650;
const unsigned long travelTime = rowSpeed * (rows - 1);
const unsigned long hitWindow = 300;
const unsigned long bottomHoldTime = 900;

bool noteDone[melodyNoteCount];
unsigned long startTime;
int score = 0;
int currentSongNote = -1;

int noteDurationFor(int divider) {
  if (divider > 0) {
    return wholenote / divider;
  }

  int dottedDuration = wholenote / abs(divider);
  return dottedDuration * 3 / 2;
}

unsigned long melodyStartTime(int noteIndex) {
  unsigned long total = 0;

  for (int i = 0; i < noteIndex; i++) {
    total += noteDurationFor(melody[i * 2 + 1]);
  }

  return total;
}

void setup() {
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < columns; col++) {
      pinMode(ledGrid[row][col], OUTPUT);
      digitalWrite(ledGrid[row][col], LOW);
    }
  }

  for (int lane = 0; lane < columns; lane++) {
    pinMode(buttonPins[lane], INPUT_PULLUP);
  }

  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
  startTime = millis();
}

void loop() {
  unsigned long gameTime = millis() - startTime;

  updateSong(gameTime);
  clearDisplay();
  updateGame(gameTime);
}

void updateSong(unsigned long gameTime) {
  if (gameTime < startDelay) {
    noTone(buzzer);
    return;
  }

  unsigned long songTime = gameTime - startDelay;
  unsigned long elapsed = 0;

  for (int noteIndex = 0; noteIndex < melodyNoteCount; noteIndex++) {
    int pitch = melody[noteIndex * 2];
    int duration = noteDurationFor(melody[noteIndex * 2 + 1]);

    if (songTime >= elapsed && songTime < elapsed + duration) {
      if (currentSongNote != noteIndex) {
        currentSongNote = noteIndex;

        if (pitch == REST) {
          noTone(buzzer);
        } else {
          tone(buzzer, pitch);
        }
      }

      if (songTime - elapsed > duration * 9 / 10) {
        noTone(buzzer);
      }

      return;
    }

    elapsed += duration;
  }

  noTone(buzzer);
}

void updateGame(unsigned long gameTime) {
  for (int noteIndex = 0; noteIndex < melodyNoteCount; noteIndex++) {
    int lane = noteSequence[noteIndex % trackLength];

    if (lane < 0 || lane >= columns || noteDone[noteIndex]) {
      continue;
    }

    unsigned long hitTime = startDelay + melodyStartTime(noteIndex);
    unsigned long spawnTime = hitTime - travelTime;

    if (gameTime >= spawnTime && gameTime <= hitTime + bottomHoldTime) {
      int row = (gameTime - spawnTime) / rowSpeed;

      if (row >= rows) {
        row = rows - 1;
      }

      digitalWrite(ledGrid[row][lane], HIGH);
    }

    if (abs((long)gameTime - (long)hitTime) <= (long)hitWindow) {
      if (digitalRead(buttonPins[lane]) == LOW) {
        noteDone[noteIndex] = true;
        score += 100;

        Serial.print("Hit! Score: ");
        Serial.println(score);
      }
    }

    if (gameTime > hitTime + bottomHoldTime) {
      noteDone[noteIndex] = true;
      Serial.println("Miss!");
    }
  }
}

void clearDisplay() {
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < columns; col++) {
      digitalWrite(ledGrid[row][col], LOW);
    }
  }
}
