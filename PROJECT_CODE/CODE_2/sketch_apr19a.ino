#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// GSM Module Configuration
SoftwareSerial gsm(8, 7); // RX=8, TX=7

// GPS Module Configuration
SoftwareSerial gpsSerial(5, 4); // RX=5, TX=4

// I2C LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); // 0x27 address, 16x2 display

// DHT22 Sensor Configuration
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Pin Definitions
const int ldrPin = A0;
const int buttonPin = 6;

// Thresholds
const int LDR_DARK_THRESHOLD = 300;
String PHONE = "+1234567890"; // Replace with your number

// State Variables
bool alertSent = false;
unsigned long lastSmsTime = 0;
int lastButtonState = HIGH;
unsigned long lastDisplaySwitch = 0;
int displayPage = 0;

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  gpsSerial.begin(9600);
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");

  pinMode(buttonPin, INPUT_PULLUP);

  delay(2000);
  gsm.println("AT");
  delay(1000);
  gsm.println("AT+CMGF=1");
  delay(1000);

  lcd.clear();
}

void loop() {
  // Read Sensors
  int ldrValue = analogRead(ldrPin);
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();
  int buttonState = digitalRead(buttonPin);
  String gpsData = readGPS();

  // Button Alert
  if (buttonState != lastButtonState) {
    delay(50);
    if (buttonState == LOW) {
      sendAlert("MANUAL DISTRESS SIGNAL!", gpsData);
    }
  }
  lastButtonState = buttonState;

  // LDR Alert
  if (ldrValue > LDR_DARK_THRESHOLD && !alertSent) {
    sendAlert("DARKNESS DETECTED!", gpsData);
    alertSent = true;
  } else if (ldrValue <= LDR_DARK_THRESHOLD) {
    alertSent = false;
  }

  // LCD Display Switch
  if (millis() - lastDisplaySwitch >= 4000) {
    lcd.clear();
    displayPage = (displayPage + 1) % 2;
    lastDisplaySwitch = millis();
  }

  if (displayPage == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(humidity, 0);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Light:");
    lcd.print(ldrValue);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("GPS:");

    if (gpsData != "No Fix") {
      int commaIndex = gpsData.indexOf(',');
      String lat = gpsData.substring(0, commaIndex);
      String lon = gpsData.substring(commaIndex + 1);
      lcd.setCursor(0, 1);
      lcd.print(lat + "," + lon);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Waiting for fix");
    }
  }

  // Periodic SMS
  if (millis() - lastSmsTime >= 30000) {
    String message = "System Status:\n";
    message += "Temp: " + String(temp, 1) + "C\n";
    message += "Humidity: " + String(humidity, 0) + "%\n";
    message += "Light: " + String(ldrValue) + "\n";
    message += "Location: " + gpsData;

    sendSMS(PHONE, message);
    lastSmsTime = millis();
  }

  delay(1000);
}

String readGPS() {
  while (gpsSerial.available()) {
    String data = gpsSerial.readStringUntil('\n');
    if (data.startsWith("$GPGGA")) {
      int index = 0;
      for (int i = 0; i < 6; i++) index = data.indexOf(',', index + 1);
      String lat = data.substring(index + 1, index + 10);
      index = data.indexOf(',', index + 1);
      String lon = data.substring(index + 1, index + 11);
      return lat + "," + lon;
    }
  }
  return "No Fix";
}

void sendAlert(String type, String location) {
  String message = type + "\n";
  message += "Location: " + location;
  sendSMS(PHONE, message);
}

void sendSMS(String number, String text) {
  gsm.print("AT+CMGS=\"" + number + "\"\r");
  delay(1000);
  gsm.print(text);
  delay(1000);
  gsm.write(26); // CTRL+Z
  delay(1000);
  Serial.println("SMS Sent");
}
