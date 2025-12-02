// Elevator Control System
#include <LiquidCrystal.h>
int rs=7;
int en=8;
int d4=9;
int d5=10;
int d6=11;
int d7=12;

// Pin configurations
LiquidCrystal lcd(rs,en,d4,d5,d6,d7);
const int buttonPins[4] = {A5, 3, 4, A4}; // Floor buttons (1 to 3)
const int ledPins[4] = {5, 6, 13, 2};   // LEDs for floors (1 to 3)
const int buzzPin=A2;

// State machine states
enum State { IDLE, NEXT_STATE, MOVE_UP, MOVE_DOWN };
State currentState = IDLE;

// Initial Floor as 1
int currentFloor = 1;   
int targetFloor = 1;
int j=0;

void setup() {
  Serial.begin(9600);
  lcd.begin(16,2);
  pinMode(buzzPin,OUTPUT);
  pinMode(buttonPins[0], INPUT_PULLUP); // Buttons with internal pull-ups
  pinMode(ledPins[0], OUTPUT);
  pinMode(buttonPins[1], INPUT_PULLUP);
  pinMode(ledPins[1], OUTPUT); 
  pinMode(buttonPins[2], INPUT_PULLUP);
  pinMode(ledPins[2], OUTPUT); 
  pinMode(buttonPins[3], INPUT_PULLUP);
  pinMode(ledPins[3], OUTPUT); 

  updateLEDs();
  Serial.println("Elevator System Initialized.");
  lcd.setCursor(0,0);//Message on LED
  lcd.print("System Initiated");
}

void loop() {
  switch (currentState) {
    case IDLE:
      checkButtons(); // Wait for user input
      break;

    case NEXT_STATE:
      if (targetFloor > currentFloor) {
        currentState = MOVE_UP;
      } else if (targetFloor < currentFloor) {
        currentState = MOVE_DOWN;
      } else {
        j=11;
        door();
        delay(200);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Current floor :");
        lcd.println(currentFloor);
        currentState = IDLE;
      }
      break;

    case MOVE_UP:
      moveElevatorUp();
      break;

    case MOVE_DOWN:
      moveElevatorDown();
      break;
  }
}


// Functions
void checkButtons() {
  for (int i = 0; i < 4; i++) {
    if (digitalRead(buttonPins[i]) == LOW) { // Button pressed (active low)
      targetFloor = i + 1;
      Serial.print("Button pressed for floor: ");
      Serial.println(targetFloor);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Button pressed:");
      lcd.println(targetFloor);
      delay(1000);
      currentState = NEXT_STATE;
      break;
    }
  }
}

void moveElevatorUp() {
  Serial.println("Moving Up...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Moving Up...");
  delay(1000); // Simulate elevator moving time
  currentFloor++;
  updateLEDs();
  if (currentFloor == targetFloor) {
    Serial.print("Arrived at floor ");
    Serial.println(currentFloor);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Arrived floor :");
    lcd.println(currentFloor);
    updateLEDs();
    delay(1500);
    j=11;
    door();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Current floor :");
    lcd.println(currentFloor);
    currentState = IDLE;
  }
}

void moveElevatorDown() {
  Serial.println("Moving Down...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Moving Down...");
  delay(1000);
  currentFloor--;
  updateLEDs();
  if (currentFloor == targetFloor) {
    Serial.print("Arrived at floor ");
    Serial.println(currentFloor);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Arrived floor :");
    lcd.println(currentFloor);
    updateLEDs();
    delay(1500);
    j=11;
    door();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Current floor :");
    lcd.println(currentFloor);
    currentState = IDLE;
  }
}

void updateLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], (i + 1 == currentFloor) ? HIGH : LOW);
  }
}
void door(){
  digitalWrite(buzzPin,LOW);
  Serial.print("Door opening");
  Serial.print("[===......]");
  if(j==11){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Door Opening");
  lcd.setCursor(0,1);
  lcd.print("  [==========]");
  lcd.setCursor(12,1);
  }
  for(int i=j; i>1; i--){
    lcd.print(".");
    lcd.setCursor(i,1);
    delay(300);
  } 
  Serial.print("Door closing");
  Serial.print("[==========]");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Door Closing");
  lcd.setCursor(0,1);
  lcd.print("  [..........]");
  lcd.setCursor(3,1);
  digitalWrite(buzzPin,HIGH);
  j=0;
  for(int i=0; i<10; i++){
  if (digitalRead(buttonPins[currentFloor - 1])==LOW){
    //lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Opening again!");
    delay(500);
    door();
    break;
  }
  j=i+4;
  lcd.print("=");
  lcd.setCursor(i+4,1);
  delay(50);
  digitalWrite(buzzPin,LOW);
  delay(260);
  digitalWrite(buzzPin,HIGH);
  }
  digitalWrite(buzzPin,LOW);
 // delay(1000);
}
