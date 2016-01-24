/*
 * Homebrewed Arcade Panel
 * Uno
 * 
 * MIT License
 * chrdev 2016
 */
const int PIN_A = 12;
const int PIN_B = 11;
const int PIN_1Y = 10;
const int PIN_2Y = 9;
const int PIN_1YD = 8;

void setup()
{
  Serial.begin(115200);
  setupPins();
  digitalWrite(PIN_A, LOW); // Prepare for first time read
  digitalWrite(LED_BUILTIN, LOW);
}

void setupPins()
{
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_1Y, INPUT);
  pinMode(PIN_2Y, INPUT);
  pinMode(PIN_1YD, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  static word prev_keys = 0;
  word keys = readKeys();
  if (keys != prev_keys) {
    prev_keys = keys;
    Serial.write((byte*)&keys, sizeof(keys)); // Report HID status through serial
  }
  delay(1); // Just sleep a little bit
}

// Returns stick and button status in a certain bit order
// 8 7 6 5 4 3 2 1 * * * * U D L R
word readKeys()
{
  word rlt = 0;

  // Read from all C0s
  digitalWrite(PIN_B, LOW);
  // PIN_A is presumably LOW at this point
  bitWrite(rlt, 3, !digitalRead(PIN_1YD)); // Up
  bitWrite(rlt, 11, !digitalRead(PIN_2Y)); // 4
  bitWrite(rlt, 12, !digitalRead(PIN_1Y)); // 5

  // Read from all C1s
  // PIN_B is LOW at this point.
  digitalWrite(PIN_A, HIGH);
  bitWrite(rlt, 2, !digitalRead(PIN_1YD)); // Down
  bitWrite(rlt, 10, !digitalRead(PIN_2Y)); // 3
  bitWrite(rlt, 13, !digitalRead(PIN_1Y)); // 6

  // Read from all C3s
  digitalWrite(PIN_B, HIGH);
  // PIN_A is HGIH at this point
  bitWrite(rlt, 0, !digitalRead(PIN_1YD)); // Right
  bitWrite(rlt, 8, !digitalRead(PIN_2Y)); // 1
  bitWrite(rlt, 15, !digitalRead(PIN_1Y)); // 8
  
  // Read from all C2s
  // PIN_B is HIGH at this point.
  digitalWrite(PIN_A, LOW);
  bitWrite(rlt, 1, !digitalRead(PIN_1YD)); // Left
  bitWrite(rlt, 9, !digitalRead(PIN_2Y)); // 2
  bitWrite(rlt, 14, !digitalRead(PIN_1Y)); // 7

  return rlt;
}
