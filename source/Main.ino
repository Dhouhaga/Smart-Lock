#include<WiFi.h>
#include<WebServer.h>
#include <Keypad_I2C.h>
#include <Keypad.h>        
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include<SPI.h>
#include<MFRC522.h>

// ===== PIN DEFINITIONS =====
#define greenLED 32
#define redLED 25
#define lock 14
#define BUTTON_PIN 33  // Physical button for emergency unlock
#define I2CADDR 0x20   // I2C address for keypad

// ===== RFID PIN DEFINITIONS =====
#define SS_PIN 5
#define RST_PIN 27

// ===== CONSTANTS =====
#define MAX_PASSWORD_LENGTH 10
#define WIFI_TIMEOUT_MS 10000
#define PASSWORD_ATTEMPT_LIMIT 3
#define ATTEMPT_TIMEOUT_MS 30000

// ===== OBJECT INSTANCES =====
MFRC522 rfid(SS_PIN, RST_PIN);  // RFID reader instance
LiquidCrystal_I2C lcd(0x27, 20, 4);  // LCD display instance

// ===== GLOBAL VARIABLES =====
String cardID = "";  // Store user's RFID card ID
String readpw = "";  // Store entered password

// LCD Messages
const char* MSG_ENTER_PW = "ENTER PW :";
const char* MSG_CHANGING_PW = "\\CHANGING PW\\";
const char* MSG_NEW_PW = "ENTER NEW PW :";

// Password storage (NOTE: In production, use encrypted EEPROM storage)
String PW1 = "0000";  // Default password
String PW2 = "1234";  // Secondary password

// Input handling
char key, prevkey = 'N';  // 'N' = NO_KEY

// LCD cursor variables
int i = 0, j = 0, k = 0;

// State management
enum SystemState {
  GETTING_PW = 0,
  CHANGING_PW = 1,
  TESTING_OLD_PW = 2,
  GETTING_NEW_PW = 3,
  VALIDATING_PW = 4
};

SystemState change = GETTING_PW;
bool pwchange = false;

// Security tracking
int passwordAttempts = 0;
unsigned long lastAttemptTime = 0;
bool isLocked = true;  // System starts in locked state

// ===== KEYPAD CONFIGURATION =====
const byte ROWS = 4;
const byte COLS = 3;

char kpad[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {0, 1, 2, 3};
byte colPins[COLS] = {4, 5, 6};

Keypad_I2C kypad(makeKeymap(kpad), rowPins, colPins, ROWS, COLS, I2CADDR);

// ===== WIFI CONFIGURATION =====
const char* ssid = "";  // TODO: Enter your WiFi SSID
const char* password = "";  // TODO: Enter your WiFi password

WebServer server(80);
char LEDState[2][10] = {"Unlocked", "Locked"};
int state = 1;  // 1 = Locked, 0 = Unlocked

// ===== WEB SERVER HANDLERS =====

void Home() {
  String site = "<!DOCTYPE html>";
  site += "<html>";
  site += "  <head>";
  site += "    <title>Smart Lock Control</title>";
  site += "    <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8' />";
  site += "  <style>";
  site += "    .button {";
  site += "      border: none;";
  site += "      color: white;";
  site += "      display: inline-block;";
  site += "      text-align: center;";
  site += "      font-size: 20px;";
  site += "      padding: 10px 200px;";
  site += "      cursor: pointer;";
  site += "    }";
  site += "    .button1 { background-color: #4CAF50; margin: 20px 2px; }";
  site += "    .button2 { background-color: #FF0000; margin: 20px 2px; }";
  site += "  </style>";
  site += "  </head>";
  site += "  <body>";
  site += "    <p style='background-color: powderblue; text-align: center; font-size: 160%'>";
  site += "      <br>Door: ";
  site += LEDState[state];
  site += "<br><br>";
  site += "      <a href='/lock' class='button button2'>Lock</a>";
  site += "      <a href='/unlock' class='button button1'>Unlock</a>";
  site += "    </p>";
  site += "  </body>";
  site += "</html>";

  server.setContentLength(site.length());
  server.send(200, "text/html", site);
}

void Lock() {
  state = 1;
  isLocked = true;
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void Unlock() {
  state = 0;
  isLocked = false;
  digitalWrite(greenLED, HIGH);
  digitalWrite(redLED, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void NotFound() {
  server.send(404, "text/plain", "404: Not found");
}

// ===== INTERRUPT HANDLER =====

void openDoor() {
  // Emergency unlock via physical button
  digitalWrite(lock, HIGH);
  delay(500);
  digitalWrite(lock, LOW);
  Serial.println("[SYSTEM] Emergency unlock triggered");
}

// ===== SETUP =====

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize pins
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(lock, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // Initial LED state (locked = red on)
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, HIGH);
  digitalWrite(lock, LOW);

  // Initialize I2C
  Wire.begin();

  // Initialize Keypad
  kypad.begin();

  // Initialize LCD
  lcd.begin();
  delay(500);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();

  // Setup physical button interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), openDoor, RISING);

  // WiFi connection
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");

  WiFi.begin(ssid, password);
  Serial.print("[WIFI] Connecting to: ");
  Serial.println(ssid);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_TIMEOUT_MS) {
      Serial.println("[WIFI] Connection timeout!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Failed");
      break;
    }
    Serial.print(".");
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected successfully!");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  }

  // Setup web server routes
  server.on("/", Home);
  server.on("/lock", Lock);
  server.on("/unlock", Unlock);
  server.onNotFound(NotFound);
  server.begin();

  Serial.println("[SYSTEM] Web server started on port 80");

  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  delay(1500);
  lcd.clear();
}

// ===== MAIN LOOP =====

void loop() {
  // Check for RFID card
  if (rfid.PICC_IsNewCardPresent()) {
    RFID();
  }

  // Handle web server requests
  server.handleClient();

  // State machine for password/lock control
  switch (change) {
    case GETTING_PW:
      gettingpw_0();
      break;

    case CHANGING_PW:
      changepw_1();
      break;

    case TESTING_OLD_PW:
      ChangePWTest_2();
      break;

    case GETTING_NEW_PW:
      getNewPW_3();
      break;

    case VALIDATING_PW:
      PWTest_4();
      break;

    default:
      change = GETTING_PW;
      break;
  }
}

// ===== STATE FUNCTIONS =====

void gettingpw_0() {
  i = 0;
  j = 0;
  lcd.setCursor(i, j++);
  lcd.print("ENTER PW :");

  if ((key = kypad.getKey()) != 0) {
    // Check for buffer overflow
    if (readpw.length() >= MAX_PASSWORD_LENGTH) {
      Serial.println("[ERROR] Password buffer full!");
      return;
    }

    // Visual feedback
    lcd.setCursor(k++, j);
    lcd.print("*");
    digitalWrite(redLED, HIGH);
    delay(50);
    digitalWrite(redLED, LOW);

    // Check for password change trigger (*#)
    if ((readpw == "*") && (key == '#')) {
      k = 0;
      i = 0;
      readpw = "";
      lcd.clear();
      change = CHANGING_PW;
      Serial.println("[SYSTEM] Password change mode activated");
    }
    // Check for clear command (**)
    else if ((prevkey == '*') && (key == '*')) {
      lcd.clear();
      // Visual feedback pattern
      for (int x = 0; x < 3; x++) {
        digitalWrite(greenLED, HIGH);
        delay(100);
        digitalWrite(greenLED, LOW);
        delay(100);
      }
      digitalWrite(redLED, HIGH);
      lcd.setCursor(i = 0, j = 0);
      readpw = "";
      k = 0;
      Serial.println("[SYSTEM] Password cleared");
    }
    // Confirm password with #
    else if (key == '#') {
      lcd.clear();
      lcd.setCursor(5, 2);
      change = VALIDATING_PW;
      Serial.println("[SYSTEM] Validating password...");
    }
    // Normal key input
    else {
      readpw = readpw + key;
      prevkey = key;
    }
  }
}

void changepw_1() {
  // Getting old password for verification
  lcd.setCursor(i = 4, j = 0);
  lcd.print(MSG_CHANGING_PW);
  lcd.setCursor(i = 0, j = 1);
  lcd.print(MSG_ENTER_PW);

  if ((key = kypad.getKey()) != 0) {
    if (key == '#') {
      change = TESTING_OLD_PW;
      Serial.println("[SYSTEM] Old password entered, verifying...");
    } else {
      // Buffer overflow check
      if (readpw.length() >= MAX_PASSWORD_LENGTH) {
        Serial.println("[ERROR] Password too long!");
        return;
      }
      lcd.setCursor(k++, j = 2);
      lcd.print("*");
      readpw = readpw + key;
    }
  }
}

void ChangePWTest_2() {
  // Verify old password before allowing new password entry
  if (readpw == PW2) {
    Serial.println("[SYSTEM] Old password verified, entering new password");
    readpw = "";
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print(MSG_CHANGING_PW);
    lcd.setCursor(i = 0, j = 1);
    lcd.print(MSG_NEW_PW);
    j++;
    k = 0;
    change = GETTING_NEW_PW;
  } else {
    // Wrong password - cancel operation
    Serial.println("[SYSTEM] Old password incorrect!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WRONG PW");
    delay(2000);
    lcd.clear();
    change = GETTING_PW;
    readpw = "";
    k = 0;
  }
}

void getNewPW_3() {
  // Getting and saving new password
  if ((key = kypad.getKey()) != 0) {
    lcd.setCursor(k++, j);

    if (key == '#') {
      if (readpw != "") {
        PW2 = readpw;
        pwchange = true;
        Serial.println("[SYSTEM] Password changed successfully!");
        lcd.clear();
        lcd.setCursor(5, 2);
        change = VALIDATING_PW;
      } else {
        // Empty password not allowed
        Serial.println("[ERROR] Empty password not allowed!");
        lcd.clear();
        change = GETTING_PW;
        readpw = "";
        k = 0;
      }
    } else {
      // Buffer overflow check
      if (readpw.length() >= MAX_PASSWORD_LENGTH) {
        Serial.println("[ERROR] Password too long!");
        return;
      }
      lcd.print(key);
      readpw = readpw + key;
    }
  }
}

void PWTest_4() {
  // Password validation and door control
  if (pwchange) {
    // Password was successfully changed
    digitalWrite(greenLED, HIGH);
    delay(1500);
    digitalWrite(greenLED, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New PW:");
    lcd.setCursor(0, 1);
    lcd.print(PW2);
    delay(2000);
    pwchange = false;
  } else {
    // Validate password for door unlock
    if ((readpw == PW1) || (readpw == PW2)) {
      Serial.println("[SYSTEM] Password ACCEPTED");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.println("ACCEPTED");
      digitalWrite(greenLED, HIGH);
      digitalWrite(redLED, LOW);
      isLocked = false;
      delay(2000);
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, HIGH);
      isLocked = true;
      passwordAttempts = 0;  // Reset attempts on success
    } else {
      // Wrong password
      passwordAttempts++;
      Serial.print("[SYSTEM] Password DENIED (Attempt ");
      Serial.print(passwordAttempts);
      Serial.println(")");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.println("DENIED");
      delay(500);
      lcd.clear();
      lcd.setCursor(0, 0);

      // Lock system after too many attempts
      if (passwordAttempts >= PASSWORD_ATTEMPT_LIMIT) {
        Serial.println("[SECURITY] Too many failed attempts - system locked!");
        lcd.print("LOCKED");
        digitalWrite(redLED, HIGH);
        digitalWrite(greenLED, LOW);
        delay(ATTEMPT_TIMEOUT_MS);
        passwordAttempts = 0;
        lcd.clear();
      }
    }
  }

  lcd.clear();
  k = 0;
  digitalWrite(redLED, HIGH);
  readpw = "";
  change = GETTING_PW;
}

// ===== RFID HANDLER =====

void RFID() {
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Read RFID card ID
  cardID = "";
  for (byte i = 0; i < 4; i++) {
    cardID.concat(String(rfid.uid.uidByte[i], HEX));
  }
  cardID.toUpperCase();

  Serial.println("[RFID] New card detected!");
  Serial.print("[RFID] UID: ");
  Serial.println(cardID);

  // Stop reading
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Check authorized cards (NOTE: In production, use encrypted database)
  if (cardID == "C373C415") {
    Serial.println("[RFID] Authorized card - UNLOCKING");
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, LOW);
    isLocked = false;
    state = 0;
    delay(3000);
  } else if (cardID == "9C75849") {
    Serial.println("[RFID] Card not authorized - LOCKING");
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, HIGH);
    isLocked = true;
    state = 1;
  } else {
    Serial.println("[RFID] Unknown card");
  }

  cardID = "";
}
