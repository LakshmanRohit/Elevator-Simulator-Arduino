//Elevator control system
//For LED display
#include <LiquidCrystal.h>
int rs=7, en=8, d4=9, d5=10, d6=11, d7=12;
LiquidCrystal lcd(rs,en,d4,d5,d6,d7);

const int buttonPins[4] = {A5, 3, 4, A4};
const int ledPins[4]    = {5, 6, 13, 2};
const int buzzPin = A2;

// States
enum State { IDLE, MOVE_UP, MOVE_DOWN, DOOR_OPENING, DOOR_CLOSING };
State currentState = IDLE;

// Floor variables
int currentFloor      = 1;
int mainTarget        = 1;   // The main destination for the current trip
int targetFloor       = 1;   // Immediate goal (may be intermediateStop or mainTarget)
int intermediateStop  = 0;   // One intermediate stop between current and mainTarget (0 = none)
int pendingFloor      = 0;   // Pending Floor (0 = none)

// Timers & door animation
unsigned long lastMoveTime = 0;
unsigned long lastDoorTime = 0;
unsigned long lastButtonTime[4] = {0,0,0,0};

int doorPos = 10;   // 0=open, 10=closed
const int moveInterval = 1800;  // Between floors
const int doorInterval = 250;   // Per door animation step
const int debounceDelay = 150;  // Debounce

void setup() {
  Serial.begin(9600);
  lcd.begin(16,2);
  pinMode(buzzPin, OUTPUT);
  pinMode(buttonPins[0], INPUT_PULLUP); // Buttons with internal pull-ups
  pinMode(ledPins[0], OUTPUT);
  pinMode(buttonPins[1], INPUT_PULLUP);
  pinMode(ledPins[1], OUTPUT); 
  pinMode(buttonPins[2], INPUT_PULLUP);
  pinMode(ledPins[2], OUTPUT); 
  pinMode(buttonPins[3], INPUT_PULLUP);
  pinMode(ledPins[3], OUTPUT);

  lcd.setCursor(0,0);
  lcd.print("System Initiated");
  updateLEDs();
  delay(800);
  lcd.clear();
  showCurrentFloor();
}

//Main loop
void loop() {
  readButtons(); //Run always

  switch (currentState) {
    case IDLE: //Wait for button
      break;

    case MOVE_UP:
      handleMovement(+1);
      break;

    case MOVE_DOWN:
      handleMovement(-1);
      break;

    case DOOR_OPENING:
      handleDoorOpening();
      break;

    case DOOR_CLOSING:
      handleDoorClosing();
      break;
  }
}

//For reading buttons
void readButtons() {
  for (int i = 0; i < 4; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      if (millis() - lastButtonTime[i] < debounceDelay) continue;
      lastButtonTime[i] = millis();

      int pressed = i + 1;
      Serial.print("Button pressed: ");
      Serial.println(pressed);

      // If door is moving and user presses current floor -> reopen
      if ((currentState == DOOR_OPENING || currentState == DOOR_CLOSING) && pressed == currentFloor) {
        currentState = DOOR_OPENING;
        doorPos = 10;
        lastDoorTime = millis();
        return;
      }

      if (currentState == IDLE) {
        mainTarget = pressed;
        targetFloor = pressed;
        intermediateStop = 0;
        pendingFloor = 0;
        if (mainTarget > currentFloor) currentState = MOVE_UP;
        else if (mainTarget < currentFloor) currentState = MOVE_DOWN;
        else {
          doorPos = 10;
          currentState = DOOR_OPENING;
          lastDoorTime = millis();
        }
        return;
      }

      // If moving UP:
      if (currentState == MOVE_UP) {
        // If pressed lies between currentFloor and mainTarget -> intermediate
        if (pressed > currentFloor && pressed < mainTarget) {
          if (intermediateStop == 0) intermediateStop = pressed;
          // immediate goal becomes the intermediate stop
          targetFloor = intermediateStop;
          Serial.print("Intermediate stop set to ");
          Serial.println(intermediateStop);
        }
        else {
          // pressed is outside current upward path -> store as pending (overwrite allowed)
          if (pressed != currentFloor && pressed != mainTarget) {
            pendingFloor = pressed;
            Serial.print("Pending floor stored: ");
            Serial.println(pendingFloor);
          }
        }
        return;
      }

      // If moving DOWN:
      if (currentState == MOVE_DOWN) {
        if (pressed < currentFloor && pressed > mainTarget) {
          if (intermediateStop == 0) intermediateStop = pressed;
          targetFloor = intermediateStop;
          Serial.print("Intermediate stop set to ");
          Serial.println(intermediateStop);
        }
        else {
          if (pressed != currentFloor && pressed != mainTarget) {
            pendingFloor = pressed;
            Serial.print("Pending floor stored: ");
            Serial.println(pendingFloor);
          }
        }
        return;
      }

      // If door is opening/closing and pressed is not current floor, treat accordingly
      if ((currentState == DOOR_OPENING || currentState == DOOR_CLOSING) && pressed != currentFloor) {
        pendingFloor = pressed;
        Serial.print("Pending floor stored during door: ");
        Serial.println(pendingFloor);
        return;
      }
    }
  }
}

//Movement handling
void handleMovement(int direction) {
  unsigned long now = millis();
  if (now - lastMoveTime < moveInterval) return;
  lastMoveTime = now;

  // Bound the floor between 1 and 4
  int nextFloor = currentFloor + direction;
  if (nextFloor < 1 || nextFloor > 4) {
    currentState = IDLE;
    return;
  }

  currentFloor = nextFloor;
  updateLEDs();

  // Update display
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(direction > 0 ? "Moving Up" : "Moving Down");
  lcd.setCursor(0,1);
  lcd.print("Floor ");
  lcd.print(currentFloor);

  // If we've reached the immediate targetFloor, open door
  if (currentFloor == targetFloor) {
    doorPos = 10;
    currentState = DOOR_OPENING;
    lastDoorTime = millis();
  }
}

//Door handling
void handleDoorOpening() {
  unsigned long now = millis();
  if (now - lastDoorTime < doorInterval) return;
  lastDoorTime = now;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Door Opening");
  lcd.setCursor(2,1);
  lcd.print("[");
  for (int i = 0; i < (10 - doorPos); i++) lcd.print(".");
  for (int i = 0; i < doorPos; i++) lcd.print("=");
  lcd.print("]");

  if (doorPos > 0) {
    doorPos--;
  } else {
    // fully open → move to closing phase after short pause handled by intervals
    currentState = DOOR_CLOSING;
    lastDoorTime = millis();
  }
}

void handleDoorClosing() {
  unsigned long now = millis();
  if (now - lastDoorTime < doorInterval) return;
  lastDoorTime = now;

  // If user presses current floor while closing, reopen immediately
  if (digitalRead(buttonPins[currentFloor - 1]) == LOW) {
    currentState = DOOR_OPENING;
    doorPos = 10;
    return;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Door Closing");
  lcd.setCursor(2,1);
  lcd.print("[");
  for (int i = 0; i < (10 - doorPos); i++) lcd.print(".");
  for (int i = 0; i < doorPos; i++) lcd.print("=");
  lcd.print("]");

  digitalWrite(buzzPin, HIGH);

  if (doorPos < 10) {
    doorPos++;
    return;
  }

  // Door fully closed — decide what to do next
  digitalWrite(buzzPin, LOW);

  // CASE A: If we were serving an intermediate stop (and we are now at it)
  if (intermediateStop != 0 && currentFloor == intermediateStop) {
    // After serving intermediate stop, continue to mainTarget (original destination)
    if (mainTarget != currentFloor) {
      targetFloor = mainTarget;
      intermediateStop = 0;
      // set direction
      if (targetFloor > currentFloor) currentState = MOVE_UP;
      else if (targetFloor < currentFloor) currentState = MOVE_DOWN;
      else currentState = IDLE;
      return;
    } else {
      // if mainTarget equals current , clear intermediate and check pending
      intermediateStop = 0;
    }
  }

  // CASE B: If we arrived at the mainTarget
  if (currentFloor == mainTarget) {
    // If there is a pendingFloor (outside previous path), then move to it next
    if (pendingFloor != 0 && pendingFloor != currentFloor) {
      // promote pending to mainTarget and clear leftover
      mainTarget = pendingFloor;
      targetFloor = pendingFloor;
      pendingFloor = 0;
      intermediateStop = 0; // reset intermediate 
      if (targetFloor > currentFloor) currentState = MOVE_UP;
      else if (targetFloor < currentFloor) currentState = MOVE_DOWN;
      else currentState = IDLE;
      return;
    } else {
      // nothing pending -> remain idle at this floor
      currentState = IDLE;
      return;
    }
  }

  // CASE C: If we arrived at a target that is neither mainTarget nor intermediate (e.g., direct pending promoted)
  // If targetFloor equals pendingFloor (promoted earlier), we have already set mainTarget accordingly and cleared pending
  if (currentFloor == targetFloor) {
    if (currentFloor == mainTarget) {
      currentState = IDLE;
      return;
    } else {
      currentState = IDLE;
      return;
    }
  }
  currentState = IDLE;
}

void updateLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], (i + 1 == currentFloor) ? HIGH : LOW);
  }
}

void showCurrentFloor() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Current floor:");
  lcd.setCursor(0,1);
  lcd.print(currentFloor);
}