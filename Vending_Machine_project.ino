#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// =====================================================
// WIFI
// =====================================================

const char* WIFI_SSID = "ESP 32";
const char* WIFI_PASSWORD = "12345678";

// =====================================================
// SUPABASE
// =====================================================

const char* CREATE_PAYMENT_URL =
  "https://bnjwunbpkaqaapsoyjmf.supabase.co/functions/v1/create-payment-link";

const char* TRANSACTION_STATUS_URL =
  "https://bnjwunbpkaqaapsoyjmf.supabase.co/functions/v1/transaction-status";

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// BUTTONS
// =====================================================

#define BUTTON_1 25
#define BUTTON_2 26
#define BUTTON_3 27

// =====================================================
// SERVOS
// =====================================================

#define SERVO_1_PIN 13
#define SERVO_2_PIN 14
#define SERVO_3_PIN 32

Servo servo1;
Servo servo2;
Servo servo3;

// =====================================================
// SERVO ANGLES
// CHANGE THESE LATER AFTER MECHANICAL CALIBRATION
// =====================================================

const int SERVO1_CLOSED = 0;
const int SERVO1_OPEN   = 90;

const int SERVO2_CLOSED = 0;
const int SERVO2_OPEN   = 90;

const int SERVO3_CLOSED = 0;
const int SERVO3_OPEN   = 90;

// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);

// =====================================================
// CURRENT TRANSACTION
// =====================================================

String transactionID = "";
String paymentURL = "";

int selectedCandy = 0;
int selectedPrice = 0;

bool paymentWaiting = false;
bool dispensing = false;

unsigned long lastStatusCheck = 0;

const unsigned long STATUS_INTERVAL = 2000;

// =====================================================
// HELPER: OLED
// =====================================================

void showOLED(String line1, String line2 = "", String line3 = "", String line4 = "") {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println(line1);

  display.setCursor(0, 16);
  display.println(line2);

  display.setCursor(0, 32);
  display.println(line3);

  display.setCursor(0, 48);
  display.println(line4);

  display.display();
}

// =====================================================
// OLED: STANDBY SCREEN FOR NEXT CUSTOMER
// Always shows the IP address.
// =====================================================

void showStandbyOLED() {

  showOLED(
    "VENDING MACHINE",
    "Open in phone:",
    WiFi.localIP().toString(),
    "Select candy"
  );
}

// =====================================================
// WIFI
// =====================================================

void connectWiFi() {

  showOLED("Connecting WiFi...", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi CONNECTED");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  showOLED(
    "WiFi Connected",
    "Open in phone:",
    WiFi.localIP().toString(),
    "Starting machine..."
  );

  delay(3000);
}

// =====================================================
// EXTRACT JSON STRING VALUE
// Simple helper so ArduinoJson is not required
// =====================================================

String getJSONValue(String json, String key) {

  String search = "\"" + key + "\":\"";

  int start = json.indexOf(search);

  if (start == -1) {
    return "";
  }

  start += search.length();

  int end = json.indexOf("\"", start);

  if (end == -1) {
    return "";
  }

  return json.substring(start, end);
}

// =====================================================
// CREATE PAYMENT LINK
// =====================================================

bool createPayment(int candyID) {

  Serial.println();
  Serial.println("Creating payment...");

  HTTPClient http;

  http.begin(CREATE_PAYMENT_URL);

  http.addHeader("Content-Type", "application/json");

  String body =
    "{\"candy_id\":" + String(candyID) + "}";

  int httpCode = http.POST(body);

  String response = http.getString();

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  Serial.println("Response:");
  Serial.println(response);

  http.end();

  if (httpCode != 200) {

    Serial.println("Payment link creation FAILED");

    showOLED(
      "Payment Error",
      "Try again"
    );

    return false;
  }

  transactionID =
    getJSONValue(response, "transaction_id");

  paymentURL =
    getJSONValue(response, "payment_url");

  if (transactionID == "" || paymentURL == "") {

    Serial.println("Invalid payment response");

    showOLED(
      "Payment Error",
      "Invalid response"
    );

    return false;
  }

  paymentWaiting = true;

  Serial.println("Transaction ID:");
  Serial.println(transactionID);

  Serial.println("Payment URL:");
  Serial.println(paymentURL);

  showOLED(
    "PAYMENT READY",
    "Open in phone:",
    WiFi.localIP().toString(),
    "Tap PAY NOW"
  );

  return true;
}

// =====================================================
// WEB PAGE
// =====================================================

void handleRoot() {

  String html = "";

  html += "<!DOCTYPE html>";
  html += "<html lang='en'>";
  html += "<meta charset='UTF-8'>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<title>ESP32 Vending Machine</title>";

  html += "<style>";
  html += "body{font-family:Arial,sans-serif;text-align:center;background:#f2f2f2;padding:15px;margin:0;}";
  html += ".box{background:white;padding:22px;border-radius:15px;max-width:420px;margin:15px auto;box-shadow:0 2px 8px rgba(0,0,0,.12);}";
  html += "h1{font-size:25px;margin:5px 0 18px;}";
  html += "h2{font-size:20px;margin:12px 0;}";
  html += ".ip{background:#eeeeee;padding:12px;border-radius:8px;font-size:18px;font-weight:bold;}";
  html += ".item{background:#f8f8f8;padding:10px;margin:8px 0;border-radius:8px;text-align:left;}";
  html += "button{font-size:18px;padding:14px;margin:7px 0;width:100%;border:0;border-radius:10px;background:#eeeeee;}";
  html += ".pay{background:#28a745;color:white;text-decoration:none;padding:15px;display:block;border-radius:10px;margin-top:18px;font-size:20px;font-weight:bold;}";
  html += ".status{margin-top:15px;padding:12px;border-radius:8px;background:#fff3cd;}";
  html += ".note{font-size:14px;color:#555;line-height:1.4;}";
  html += "</style>";

  html += "</head>";
  html += "<body>";
  html += "<div class='box'>";

  html += "<h1>ESP32 VENDING MACHINE</h1>";

  html += "<p><b>Machine IP Address</b></p>";
  html += "<div class='ip'>" + WiFi.localIP().toString() + "</div>";

  if (selectedCandy == 0) {

    html += "<h2>Select Candy</h2>";

    html += "<div class='item'><b>Candy 1</b> - Toffee - <b>Rs 5</b></div>";
    html += "<button onclick=\"location.href='/select?candy=1'\">Select Candy 1 - Rs 5</button>";

    html += "<div class='item'><b>Candy 2</b> - Boomer - <b>Rs 10</b></div>";
    html += "<button onclick=\"location.href='/select?candy=2'\">Select Candy 2 - Rs 10</button>";

    html += "<div class='item'><b>Candy 3</b> - Chocolate - <b>Rs 20</b></div>";
    html += "<button onclick=\"location.href='/select?candy=3'\">Select Candy 3 - Rs 20</button>";

    html += "<p class='note'>Select a candy first. Then complete the payment on the Razorpay payment page.</p>";

  } else {

    String candyName;

    if (selectedCandy == 1) {
      candyName = "Toffee";
    } else if (selectedCandy == 2) {
      candyName = "Boomer";
    } else {
      candyName = "Chocolate";
    }

    html += "<h2>Order Details</h2>";
    html += "<div class='item'><b>Selected Candy:</b> Candy " + String(selectedCandy) + " - " + candyName + "</div>";
    html += "<div class='item'><b>Price:</b> Rs " + String(selectedPrice) + "</div>";

    if (paymentWaiting) {

      html += "<div class='status'>";
      html += "<b>Payment Status: Waiting for payment</b><br>";
      html += "Tap PAY NOW to open Razorpay.";
      html += "</div>";

      html += "<a class='pay' href='" + paymentURL + "' target='_blank'>PAY NOW - Rs " + String(selectedPrice) + "</a>";

      html += "<p class='note'>After successful payment, the machine will verify the payment and automatically dispense your selected candy.</p>";

    } else {

      html += "<div class='status'>Payment Status: Preparing payment...</div>";
      html += "<p class='note'>Please wait.</p>";
    }

    html += "<p><a href='/' style='text-decoration:none;'>Back to candy selection</a></p>";
  }

  html += "</div>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

// =====================================================
// WEB SELECT
// =====================================================

void handleSelect() {

  if (!server.hasArg("candy")) {

    server.send(400, "text/plain", "Missing candy");

    return;
  }

  int candy = server.arg("candy").toInt();

  if (candy < 1 || candy > 3) {

    server.send(400, "text/plain", "Invalid candy");

    return;
  }

  selectedCandy = candy;

  if (candy == 1) {

    selectedPrice = 5;

  } else if (candy == 2) {

    selectedPrice = 10;

  } else {

    selectedPrice = 20;
  }

  createPayment(selectedCandy);

  server.sendHeader("Location", "/");

  server.send(302);
}

// =====================================================
// PAYMENT PAGE
// =====================================================

void handlePayment() {

  if (paymentURL == "") {

    server.send(
      404,
      "text/plain",
      "No payment available"
    );

    return;
  }

  server.sendHeader(
    "Location",
    paymentURL
  );

  server.send(302);
}

// =====================================================
// CHECK PAYMENT STATUS
// =====================================================

void checkPaymentStatus() {

  if (!paymentWaiting) {
    return;
  }

  if (transactionID == "") {
    return;
  }

  if (millis() - lastStatusCheck < STATUS_INTERVAL) {
    return;
  }

  lastStatusCheck = millis();

  HTTPClient http;

  String url =
    String(TRANSACTION_STATUS_URL) +
    "?id=" +
    transactionID;

  http.begin(url);

  int httpCode = http.GET();

  String response = http.getString();

  http.end();

  Serial.print("Payment status HTTP: ");
  Serial.println(httpCode);

  Serial.println(response);

  if (httpCode != 200) {
    return;
  }

  String status =
    getJSONValue(response, "status");

  Serial.print("Transaction status: ");
  Serial.println(status);

  if (status == "paid") {

    Serial.println();
    Serial.println("PAYMENT CONFIRMED!");

    showOLED(
      "PAYMENT SUCCESS",
      "Dispensing...",
      "Please wait"
    );

    paymentWaiting = false;

    dispenseCandy(selectedCandy);
  }
}

// =====================================================
// DISPENSE CANDY
// =====================================================

void dispenseCandy(int candyID) {

  if (dispensing) {
    return;
  }

  dispensing = true;

  Serial.println();
  Serial.println("====================");
  Serial.println("DISPENSING CANDY");
  Serial.println("====================");

  if (candyID == 1) {

    Serial.println("Rs 5 candy");
    Serial.println("Servo 1 opening");

    servo1.write(SERVO1_OPEN);

    delay(1200);

    servo1.write(SERVO1_CLOSED);
  }

  else if (candyID == 2) {

    Serial.println("Rs 10 candy");
    Serial.println("Servo 2 opening");

    servo2.write(SERVO2_OPEN);

    delay(1200);

    servo2.write(SERVO2_CLOSED);
  }

  else if (candyID == 3) {

    Serial.println("Rs 20 candy");
    Serial.println("Servo 3 opening");

    servo3.write(SERVO3_OPEN);

    delay(1200);

    servo3.write(SERVO3_CLOSED);
  }

  Serial.println("Servo returned to closed position");

  delay(500);

  markDispensed();

  dispensing = false;

  selectedCandy = 0;
  selectedPrice = 0;

  transactionID = "";
  paymentURL = "";

  showOLED(
    "DISPENSE COMPLETE",
    "Thank you!",
    "Next customer:",
    WiFi.localIP().toString()
  );

  delay(3000);

  // Return to standby screen for the next customer.
  showStandbyOLED();
}

// =====================================================
// MARK TRANSACTION AS DISPENSED
// =====================================================

void markDispensed() {

  if (transactionID == "") {
    return;
  }

  HTTPClient http;

  String url =
    String(TRANSACTION_STATUS_URL) +
    "?id=" +
    transactionID;

  http.begin(url);

  http.addHeader(
    "Content-Type",
    "application/json"
  );

  int httpCode = http.sendRequest("PATCH");

  String response = http.getString();

  http.end();

  Serial.print("Dispensed update HTTP: ");
  Serial.println(httpCode);

  Serial.println(response);

  if (httpCode == 200) {

    Serial.println(
      "Transaction marked DISPENSED"
    );

  } else {

    Serial.println(
      "WARNING: Could not mark transaction dispensed"
    );
  }
}

// =====================================================
// PHYSICAL BUTTON HANDLER
// =====================================================

void checkButtons() {

  static bool lastButton1 = HIGH;
  static bool lastButton2 = HIGH;
  static bool lastButton3 = HIGH;

  bool b1 = digitalRead(BUTTON_1);
  bool b2 = digitalRead(BUTTON_2);
  bool b3 = digitalRead(BUTTON_3);

  // Button 1
  if (lastButton1 == HIGH && b1 == LOW) {

    if (!paymentWaiting && !dispensing) {

      Serial.println("BUTTON 1 PRESSED");

      selectedCandy = 1;
      selectedPrice = 5;

      createPayment(1);
    }

    delay(250);
  }

  // Button 2
  if (lastButton2 == HIGH && b2 == LOW) {

    if (!paymentWaiting && !dispensing) {

      Serial.println("BUTTON 2 PRESSED");

      selectedCandy = 2;
      selectedPrice = 10;

      createPayment(2);
    }

    delay(250);
  }

  // Button 3
  if (lastButton3 == HIGH && b3 == LOW) {

    if (!paymentWaiting && !dispensing) {

      Serial.println("BUTTON 3 PRESSED");

      selectedCandy = 3;
      selectedPrice = 20;

      createPayment(3);
    }

    delay(250);
  }

  lastButton1 = b1;
  lastButton2 = b2;
  lastButton3 = b3;
}

// =====================================================
// SERVO SETUP
// =====================================================

void setupServos() {

  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo3.setPeriodHertz(50);

  servo1.attach(SERVO_1_PIN);
  servo2.attach(SERVO_2_PIN);
  servo3.attach(SERVO_3_PIN);

  servo1.write(SERVO1_CLOSED);
  servo2.write(SERVO2_CLOSED);
  servo3.write(SERVO3_CLOSED);

  Serial.println("All servos initialized");
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // OLED
  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    Serial.println(
      "OLED initialization failed"
    );
  }

  showOLED(
    "ESP32 VENDING",
    "Starting..."
  );

  // Buttons
  pinMode(
    BUTTON_1,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_2,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_3,
    INPUT_PULLUP
  );

  // Servos
  setupServos();

  // WiFi
  connectWiFi();

  // Web server
  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/select",
    handleSelect
  );

  server.on(
    "/payment",
    handlePayment
  );

  server.begin();

  Serial.println(
    "Web server started"
  );

  Serial.print(
    "Open: http://"
  );

  Serial.println(
    WiFi.localIP()
  );

  // Standby screen for the first customer.
  showStandbyOLED();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  server.handleClient();

  checkButtons();

  checkPaymentStatus();
}
