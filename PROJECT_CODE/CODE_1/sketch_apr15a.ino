#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Wire.h>

// GSM Module (SIM900A) Configuration
SoftwareSerial gsm(8, 7); // RX=8, TX=7

// GPS Module Configuration
SoftwareSerial gpsSerial(4, 5); // RX=4 (GPS TX), TX=5 (GPS RX)

// I2C LCD Configuration (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int ldrPin = A0;
const int buttonPin = 6;

// Alert thresholds
const int LDR_DARK_THRESHOLD = 300; // Adjust based on your LDR
String PHONE = "+917058443880"; // Replace with your number

// State variables
bool alertSent = false;
unsigned long lastSmsTime = 0;
int lastButtonState = HIGH;

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  gpsSerial.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");
  
  // Configure pins
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Initialize GSM
  delay(2000);
  gsm.println("AT");
  delay(1000);
  gsm.println("AT+CMGF=1");
  delay(1000);
  
  lcd.clear();
}

void loop() {
  // Read sensors
  int ldrValue = analogRead(ldrPin);
  int buttonState = digitalRead(buttonPin);
  String gpsData = readGPS();
  
  // Detect button press with debounce
  if (buttonState != lastButtonState) {
    delay(50); // Debounce delay
    if (buttonState == LOW) {
      sendAlert("MANUAL DISTRESS ALERT!", gpsData);
    }
  }
  lastButtonState = buttonState;

  // Check LDR for darkness
  if (ldrValue > LDR_DARK_THRESHOLD && !alertSent) {
    sendAlert("DARKNESS DETECTED!", gpsData);
    alertSent = true;
  } else if (ldrValue <= LDR_DARK_THRESHOLD) {
    alertSent = false;
  }

  // Update display
  lcd.setCursor(0, 0);
  lcd.print("LDR:");
  lcd.print(ldrValue);
  lcd.print("  ");
  if (ldrValue > LDR_DARK_THRESHOLD) lcd.print("DARK ");
  else lcd.print("LIGHT");
  
  lcd.setCursor(0, 1);
  lcd.print("GPS:");
  lcd.print(gpsData.substring(0, 12)); // Show first 12 chars

  // Send periodic update every 30 seconds
  if (millis() - lastSmsTime >= 30000) {
    String status = "Status Update:\n";
    status += "Light: " + String(ldrValue) + "\n";
    status += "Location: " + gpsData;
    sendSMS(PHONE, status);
    lastSmsTime = millis();
  }
  
  delay(1000);
}

void sendAlert(String type, String location) {
  String message = type + "\n";
  message += "Location: " + location;
  sendSMS(PHONE, message);
}

String readGPS() {
  while (gpsSerial.available()) {
    String data = gpsSerial.readStringUntil('\n');
    if (data.startsWith("$GPGGA")) {
      int index = 0;
      for (int i = 0; i < 6; i++) {
        index = data.indexOf(',', index + 1);
      }
      String lat = data.substring(index+1, index+10);
      index = data.indexOf(',', index+1);
      String lon = data.substring(index+1, index+11);
      return lat + "," + lon;
    }
  }
  return "No GPS Fix";
}

void sendSMS(String number, String text) {
  gsm.print("AT+CMGS=\"" + number + "\"\r");
  delay(1000);
  gsm.print(text);
  delay(1000);
  gsm.write(26); // CTRL+Z to send
  delay(1000);
  Serial.println("SMS Sent!");
}