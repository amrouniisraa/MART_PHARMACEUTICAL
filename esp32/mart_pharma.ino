#define ENABLE_DATABASE

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <FirebaseClient.h>

// ===== WIFI CONFIGURATION =====
#define WIFI_SSID "VOTRE_WIFI"
#define WIFI_PASSWORD "VOTRE_MOT_DE_PASSE"

// ===== FIREBASE CONFIGURATION =====
#define DATABASE_URL "https://VOTRE-PROJET-default-rtdb.europe-west1.firebasedatabase.app/"

// ===== ORACLE APEX CONFIGURATION =====
#define ORACLE_APEX_URL "https://oracleapex.com/ords/VOTRE_WORKSPACE/VOTRE_MODULE/esp32/data/"

// ===== DEVICE CONFIGURATION =====
#define DEVICE_ID "ESP32_RFID_01"

// ===== TIME CONFIGURATION (Tunisia = UTC+1) =====
#define GMT_OFFSET_SEC 3600
#define DAYLIGHT_OFFSET_SEC 0

// ===== RFID PINS =====
#define SS_PIN 5
#define RST_PIN 22

// ===== SCAN COOLDOWN =====
#define SAME_CARD_COOLDOWN_MS 3000

// ===== RFID OBJECT =====
MFRC522 rfid(SS_PIN, RST_PIN);

// ===== FIREBASE OBJECTS =====
NoAuth no_auth;
FirebaseApp app;
RealtimeDatabase Database;

WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

// ===== ORACLE APEX HTTPS CLIENT =====
WiFiClientSecure apexSSL;

// ===== STATUS VARIABLES =====
bool previousWiFiState = false;
bool firebaseReadyMessageShown = false;

String lastUID = "";
unsigned long lastScanMs = 0;

// ===== FUNCTION DECLARATIONS =====
void connectWiFi();
void syncTime();
void processData(AsyncResult &result);
String getUID();
void sendToOracle(String jsonString);

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println(" ESP32 RFID + FIREBASE + ORACLE APEX");
  Serial.println("====================================");

  // ----- WiFi -----
  connectWiFi();

  // ----- NTP TIME -----
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
  }

  // ----- SSL CONFIGURATION -----
  ssl_client.setInsecure();
  apexSSL.setInsecure();

  // ----- FIREBASE AUTHENTICATION -----
  initializeApp(
    aClient,
    app,
    getAuth(no_auth),
    processData,
    "authTask"
  );

  // ----- FIREBASE DATABASE -----
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  // ----- SPI AND RFID -----
  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();
  delay(100);

  Serial.println();
  Serial.println("[RFID] Reader initialized.");
  rfid.PCD_DumpVersionToSerial();

  Serial.println();
  Serial.println("[System] Firebase initialized.");
  Serial.println("[System] Oracle APEX configured.");
  Serial.println("[System] Scan an RFID card...");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // Maintain Firebase tasks
  app.loop();

  // ----- WIFI STATUS -----
  bool currentWiFiState = (WiFi.status() == WL_CONNECTED);

  if (currentWiFiState != previousWiFiState) {
    previousWiFiState = currentWiFiState;
    if (currentWiFiState) {
      Serial.print("[WiFi] Connected. IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[WiFi] Disconnected!");
    }
  }

  // ----- FIREBASE STATUS -----
  if (app.ready() && !firebaseReadyMessageShown) {
    firebaseReadyMessageShown = true;
    Serial.println("[Firebase] Authentication successful.");
    Serial.println("[Firebase] Ready to send RFID scans.");
  }

  // ----- CHECK SERVICES -----
  if (!currentWiFiState || !app.ready()) {
    delay(10);
    return;
  }

  // ----- CHECK RFID CARD -----
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(5);
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    delay(5);
    return;
  }

  String uidString = getUID();

  // ----- DUPLICATE SCAN PROTECTION -----
  if (uidString == lastUID &&
      (millis() - lastScanMs) < SAME_CARD_COOLDOWN_MS) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(50);
    return;
  }

  lastUID = uidString;
  lastScanMs = millis();

  Serial.println();
  Serial.println("========== RFID CARD DETECTED ==========");

  // ----- CARD TYPE -----
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  String typeString = String(rfid.PICC_GetTypeName(piccType));

  Serial.print("UID: ");
  Serial.println(uidString);
  Serial.print("Card type: ");
  Serial.println(typeString);

  // ----- FIREBASE PATH -----
  String databaseUID = uidString;
  databaseUID.replace(":", "");
  String path = String("/BadgesDetectes/") + databaseUID;

  // ----- TIMESTAMP -----
  char timeText[25] = "2026-11-18 18:00:00";
  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 100)) {
    strftime(
      timeText,
      sizeof(timeText),
      "%Y-%m-%d %H:%M:%S",
      &timeinfo
    );
  }

  // ----- JSON PACKET -----
  String jsonString = "{";
  jsonString += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonString += "\"uid\":\"" + uidString + "\",";
  jsonString += "\"type\":\"" + typeString + "\",";
  jsonString += "\"timeScanned\":\"" + String(timeText) + "\"";
  jsonString += "}";

  Serial.println();
  Serial.print("[JSON] ");
  Serial.println(jsonString);

  // =================================================
  // 1. SEND TO FIREBASE
  // =================================================
  Serial.println();
  Serial.println("[Firebase] Sending scan...");

  object_t json(jsonString.c_str());

  Database.set<object_t>(
    aClient,
    path,
    json,
    processData,
    "rfidWrite"
  );

  Serial.println("[Firebase] Write queued.");

  // =================================================
  // 2. SEND TO ORACLE APEX
  // =================================================
  sendToOracle(jsonString);

  // ----- FINISH RFID COMMUNICATION -----
  Serial.println();
  Serial.println("========== SCAN COMPLETED ==========");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(800);
}

// =====================================================
// SEND DATA TO ORACLE APEX
// =====================================================
void sendToOracle(String jsonString) {
  Serial.println();
  Serial.println("[Oracle APEX] Preparing HTTP request...");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Oracle APEX] WiFi disconnected.");
    return;
  }

  HTTPClient http;
  http.setTimeout(15000); // Timeout étendu à 15s

  if (http.begin(apexSSL, ORACLE_APEX_URL)) {
    // Suivi automatique des redirections Oracle Cloud
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");
    http.setUserAgent("ESP32-HTTPClient");

    Serial.println("[Oracle APEX] Sending POST request...");
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) {
      Serial.print("[Oracle APEX] HTTP Response Code: ");
      Serial.println(httpResponseCode);

      String response = http.getString();
      Serial.print("[Oracle APEX] Server Response: ");
      Serial.println(response);

      if (httpResponseCode >= 200 && httpResponseCode < 300) {
        Serial.println("[Oracle APEX] SUCCESS: Data inserted into Oracle!");
      } else {
        Serial.println("[Oracle APEX] ERROR: Server rejected request.");
      }
    } else {
      Serial.print("[Oracle APEX] HTTP Request Failed: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("[Oracle APEX] Connection initialization failed.");
  }

  Serial.println("[Oracle APEX] Request completed.");
}

// =====================================================
// WIFI CONNECTION
// =====================================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[WiFi] Connecting");

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    previousWiFiState = true;
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    previousWiFiState = false;
    Serial.println("[WiFi] Connection failed.");
  }
}

// =====================================================
// NTP TIME SYNC
// =====================================================
void syncTime() {
  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    "pool.ntp.org",
    "time.nist.gov"
  );

  Serial.print("[Time] Syncing");

  unsigned long startTime = millis();
  time_t now = time(nullptr);

  while (now < 1700000000 && millis() - startTime < 15000) {
    delay(300);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println();

  if (now >= 1700000000) {
    Serial.println("[Time] Synced.");
  } else {
    Serial.println("[Time] Sync failed.");
  }
}

// =====================================================
// GET RFID UID
// =====================================================
String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) {
      uid += ":";
    }
  }
  uid.toUpperCase();
  return uid;
}

// =====================================================
// FIREBASE ASYNC RESULT CALLBACK
// =====================================================
void processData(AsyncResult &result) {
  if (!result.isResult()) {
    return;
  }
  if (result.isEvent()) {
    Serial.printf("[Firebase Event] %s: %s (code %d)\n", result.uid().c_str(), result.eventLog().message().c_str(), result.eventLog().code());
  }
  if (result.isError()) {
    Serial.printf("[Firebase Error] %s: %s (code %d)\n", result.uid().c_str(), result.error().message().c_str(), result.error().code());
  }
}
