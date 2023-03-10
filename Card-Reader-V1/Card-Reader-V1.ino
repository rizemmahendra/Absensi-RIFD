// =================== Library WiFi ===================
#include <WiFi.h>
#include <HTTPClient.h>
const char *ssid2 = "JTKWIFI";
const char *passwd2 = "RuangJTK-2021";
const char *ssid = "Warga ResLab";
const char *passwd = "wargareslab";
//String url = "http://10.44.9.79/api.php";
String url = "http://192.168.1.112/api.php";

// =================== Library RFID ===================
#include <SPI.h>
#include <MFRC522.h>
#define SDA_PIN 5
#define RST_PIN 27
MFRC522 rfid(SDA_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// =================== Library JSON ===================
#include <ArduinoJson.h>

// ==================== Library LCD ===================
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =================== Fungsi Setup ===================
void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Booting...");

//  WiFi.mode(WIFI_OFF);
//  delay(1000);
//  WiFi.mode(WIFI_STA);
//
//  WiFi.begin(ssid, passwd);
  connectingWifi();
}

// ================ Fungsi Baca RFID =================
String readCard() {
  // put your main code here, to run repeatedly:
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return "";

  // Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Serial.println(rfid.PICC_GetTypeName(piccType));

  //id kartu dan yang akan dikirim ke database
  String strID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    strID +=
      (rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
      String(rfid.uid.uidByte[i], HEX) +
      (i != rfid.uid.size - 1 ? ":" : "");
  }

  strID.toUpperCase();
  Serial.print("Kartu ID Anda : ");
  Serial.println(strID);
  delay(1000);
  return strID;
}

// =============== Fungsi Connect WiFi ===============
void connectingWifi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, passwd);
  Serial.print("Connecting");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  lcd.setCursor(0,1);
  lcd.print("to " + (String)ssid);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    timeout++;
    if(timeout >=100 && timeout < 200){
      WiFi.begin(ssid2, passwd2);
      lcd.setCursor(0, 0);
      lcd.print("Connecting...");
      lcd.setCursor(0,1);
      lcd.print("to " + (String)ssid2);
    }
    else if(timeout >= 200){
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.print("Connected to : ");
  Serial.println(ssid);
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect to Wifi");
  delay(1000);
}

// =================== Fungsi Loop ===================
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    String tag_id = "";
    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<200> doc;
      deserializeJson(doc, payload);
      String jam = doc["jam"];
      String hari = doc["hari"];
      int hour = doc["hour"]; //Perkondisian Jam
      lcd.setCursor(0, 0);
      lcd.print(jam);
      lcd.setCursor(0, 1);
      lcd.print(hari + "                ");
      Serial.print(hari+", ");Serial.println(jam);
      String readMode = doc["status"];
      if (hour >= 8 && hour <= 23) {
        lcd.noBacklight();
      }
      if (readMode == "read_card" || readMode == "add_card") {
        Serial.print("mode : "); Serial.println(readMode);
        tag_id = readCard();
        if (tag_id != "") {
          lcd.backlight();
          String postData = (String)"tag_id=" + tag_id;
          http.addHeader("Content-Type", "application/x-www-form-urlencoded");
          int responseCode = http.POST(postData);
          Serial.print("response Code : ");
          Serial.println(responseCode);
          String statusResponse = http.getString();
          Serial.println(statusResponse);
          StaticJsonDocument<200> resp;
          deserializeJson(resp, statusResponse);
          String info = resp["info"];
          String info2 = resp["info2"];
          Serial.print(info+" ");Serial.println(info2);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(info);
          lcd.setCursor(0,1);
          lcd.print(info2);
          delay(1000);
        }
      }
    }
    http.end();
  }
  else if (WiFi.status() != WL_CONNECTED) {
    lcd.backlight();
    Serial.println("NodeMCU tidak terhubung ke Access Point");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Koneksi Terputus");
    lcd.setCursor(0, 1);
    lcd.print("Reconnect...");
    connectingWifi();
  }
  delay(100);
}
