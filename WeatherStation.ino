#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h> 
#include <FS.h>
#include <LittleFS.h>
#include <WiFiManager.h> 

// ================= KONFIGURASI HARDWARE =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Tombol
#define FLASH_BUTTON 0 

// ================= GLOBAL VARIABLES =================
char apiKey[40] = ""; // DEFAULT KOSONG (User wajib isi via Config Portal)
char city_str[50] = ""; // DEFAULT KOSONG (User wajib isi via Config Portal)

bool shouldSaveConfig = false;

// Waktu (WIB = GMT+7)
const long  gmtOffset_sec = 7 * 3600; 
const int   daylightOffset_sec = 0;
const char* dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Timer Variables
unsigned long lastHeaderSwitch = 0;
bool showCityName = true; 
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 600000; // Update tiap 10 menit

struct WeatherData {
  String desc; float temp; int humidity; float windSpeed;
  int pressure; String cityName; int iconId; bool valid;
};
WeatherData currentData;

// ================= CUSTOM WEB UI =================
const char* customWebPage = R"====(
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  /* Reset & Base Config */
  * { box-sizing: border-box; }
  body {
    background-color: #121212; /* Dark Matte Background */
    color: #e0e0e0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    display: flex; flex-direction: column; align-items: center; justify-content: center;
    min-height: 100vh; margin: 0; padding: 20px;
  }

  /* Card Container */
  body > div {
    background-color: #1e1e1e; /* Surface Color */
    border: 1px solid #333;
    border-radius: 12px;
    padding: 40px 30px;
    width: 100%; max-width: 400px;
    box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    text-align: center;
  }

  /* Typography */
  h1 { font-size: 24px; font-weight: 600; color: #ffffff; margin-bottom: 30px; letter-spacing: 0.5px; }
  
  /* Form Labels */
  div { text-align: left; font-size: 14px; color: #a0a0a0; margin-bottom: 8px; font-weight: 500; }
  
  /* Input Fields */
  input[type="text"], input[type="password"] {
    width: 100%; background-color: #2c2c2c; border: 1px solid #3e3e3e;
    color: #fff; padding: 14px; border-radius: 8px; margin-bottom: 20px;
    font-size: 16px; transition: all 0.3s ease; outline: none;
  }
  
  /* Input Focus Effect */
  input[type="text"]:focus, input[type="password"]:focus {
    border-color: #26a69a; background-color: #333; box-shadow: 0 0 0 3px rgba(38, 166, 154, 0.2);
  }

  /* Buttons */
  button, input[type="submit"] {
    width: 100%; background-color: #26a69a; color: #121212;
    border: none; padding: 14px; border-radius: 8px;
    font-size: 16px; font-weight: 600; cursor: pointer;
    margin-top: 10px; transition: background-color 0.2s, transform 0.1s;
  }
  button:hover, input[type="submit"]:hover { background-color: #4db6ac; }
  button:active, input[type="submit"]:active { transform: scale(0.98); }

  /* Links & Footer */
  a { color: #26a69a; text-decoration: none; }
  .c { margin-top: 30px; font-size: 12px; color: #555; text-align: center !important; }
</style>
)====";

// ================= PROTOTYPE FUNCTIONS =================
void drawModernStatus(String title, String subText, int percent);
void drawModernInfo(String title, String line1, String line2, bool isWarning);
void saveConfigCallback();
void setupWiFiAndConfig();
void fetchWeather();
void drawUI();
void drawSignal(int x, int y, int rssi);
void drawLargeIcon(int x, int y, int id);
String truncateString(String text, int width, int textSize);
String getLocalTime();
String getDayName();
void saveConfigFile();
void loadConfigFile();
void checkResetButton();

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  pinMode(FLASH_BUTTON, INPUT_PULLUP);

  // Inisialisasi OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED Error!")); for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextWrap(false); 

  // Mount File System
  if (LittleFS.begin()) {
    Serial.println("FS Mounted");
    loadConfigFile();
  } else {
    Serial.println("FS Failed");
  }

  // Mulai WiFi & Config
  setupWiFiAndConfig();

  // Sync Waktu
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  
  // Ambil data pertama kali
  fetchWeather();
}

// ================= LOOP =================
void loop() {
  checkResetButton();

  // Update Cuaca
  if (millis() - lastUpdate >= updateInterval || lastUpdate == 0) {
    fetchWeather();
    lastUpdate = millis();
  }

  // Ganti Header (Nama Kota <-> Hari)
  if (millis() - lastHeaderSwitch >= 3000) {
    showCityName = !showCityName;
    lastHeaderSwitch = millis();
  }

  drawUI();
}

// ================= MODERN UI HELPERS =================
void drawModernStatus(String title, String subText, int percent) {
  display.clearDisplay(); display.setTextColor(WHITE);
  int16_t x1, y1; uint16_t w, h;
  
  display.setTextSize(1);
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 10); display.print(title);

  display.drawRoundRect(14, 28, 100, 8, 3, WHITE); 
  if (percent > 100) percent = 100; if (percent < 0) percent = 0;
  int barW = map(percent, 0, 100, 0, 94);
  if (barW > 0) display.fillRoundRect(17, 30, barW, 4, 2, WHITE);

  display.getTextBounds(subText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 45); display.print(subText);
  display.display();
}

void drawModernInfo(String title, String line1, String line2, bool isWarning) {
  display.clearDisplay();
  if (isWarning) {
    display.fillRoundRect(0, 0, 128, 16, 0, WHITE); display.setTextColor(BLACK, WHITE); 
  } else {
    display.setTextColor(WHITE); display.drawLine(0, 15, 128, 15, WHITE);
  }
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(1);
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h); display.setCursor((128 - w) / 2, 4); display.print(title);
  
  display.setTextColor(WHITE); 
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h); display.setCursor((128 - w) / 2, 25); display.print(line1);
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h); display.setCursor((128 - w) / 2, 40); display.print(line2);
  display.display();
}

// ================= TOMBOL RESET =================
void checkResetButton() {
  if (digitalRead(FLASH_BUTTON) == LOW) {
    drawModernInfo("SYSTEM ALERT", "HOLD TO RESET", "FACTORY SETTINGS", true);
    delay(2000); 

    if (digitalRead(FLASH_BUTTON) == LOW) {
      for(int i=0; i<=100; i+=10) { drawModernStatus("SYSTEM RESET", "Clearing Config...", i); delay(50); }
      LittleFS.remove("/config.json");
      WiFiManager wm; wm.resetSettings();
      drawModernStatus("REBOOTING", "Please Wait...", 100);
      delay(1000); ESP.restart();
    }
  }
}

// ================= WIFI & CONFIG =================
void saveConfigCallback () { shouldSaveConfig = true; }

void setupWiFiAndConfig() {
  for(int i=0; i<=30; i+=5) { drawModernStatus("BOOTING", "Loading System...", i); delay(50); }

  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setCustomHeadElement(customWebPage);

  WiFiManagerParameter custom_city("city", "Nama Kota (cth: Surabaya)", city_str, 50);
  WiFiManagerParameter custom_api("apikey", "API Key OpenWeather", apiKey, 40);

  wm.addParameter(&custom_city);
  wm.addParameter(&custom_api);

  wm.setAPCallback([](WiFiManager *myWiFiManager) {
    drawModernInfo("SETUP MODE", "Connect to WiFi:", "Weather_AP", false);
  });

  drawModernStatus("CONNECTION", "Auto Connecting...", 50);

  if (!wm.autoConnect("Weather_AP")) {
    drawModernStatus("ERROR", "Failed. Rebooting.", 0);
    delay(3000); ESP.restart(); 
  }

  drawModernStatus("SUCCESS", "WiFi Connected!", 100);
  delay(1000);
  
  strcpy(city_str, custom_city.getValue());
  strcpy(apiKey, custom_api.getValue());

  if (shouldSaveConfig) saveConfigFile();
}

// ================= FILE SYSTEM =================
void saveConfigFile() {
  DynamicJsonDocument json(1024);
  json["city"] = city_str;
  json["apikey"] = apiKey;
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) return;
  serializeJson(json, configFile); configFile.close();
}

void loadConfigFile() {
  if (LittleFS.exists("/config.json")) {
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto error = deserializeJson(json, buf.get());
      if (!error) {
        if(json.containsKey("city")) strcpy(city_str, json["city"]);
        if(json.containsKey("apikey")) {
             const char* tempKey = json["apikey"];
             if(strlen(tempKey) > 5) strcpy(apiKey, tempKey);
        }
      }
    }
  }
}

// ================= API OPENWEATHER =================
void fetchWeather() {
  if(WiFi.status() != WL_CONNECTED) return;
  if(strlen(city_str) == 0) { currentData.valid = false; return; }

  drawModernStatus("SYNC DATA", "Downloading...", 100);

  WiFiClientSecure client; client.setInsecure(); client.setTimeout(10000);
  HTTPClient http;
  
  String url = "https://api.openweathermap.org/data/2.5/weather?q=" + String(city_str) + "&appid=" + String(apiKey) + "&units=metric";
  
  if (http.begin(client, url)) {
    int httpCode = http.GET();
    if (httpCode > 0) {
      DynamicJsonDocument doc(2048); 
      DeserializationError error = deserializeJson(doc, http.getString());
      if (!error) {
        currentData.temp = doc["main"]["temp"];
        currentData.humidity = doc["main"]["humidity"];
        currentData.pressure = doc["main"]["pressure"];
        currentData.windSpeed = doc["wind"]["speed"];
        currentData.desc = doc["weather"][0]["description"].as<String>();
        currentData.cityName = doc["name"].as<String>();
        currentData.iconId = doc["weather"][0]["id"];
        currentData.valid = true;
      } 
    }
    http.end();
  }
}

// ================= MAIN UI =================
void drawUI() {
  display.clearDisplay();

  // 1. CEK DATA
  if(strlen(city_str) == 0) {
    int anim = (millis() / 20) % 100;
    drawModernStatus("SETUP REQUIRED", "Set City in Config", anim); return;
  }
  if (!currentData.valid) { 
    int anim = (millis() / 15) % 100; 
    String info = "Waiting Data...";
    if(strlen(apiKey) < 5) info = "Check API Key!";
    drawModernStatus("SYNC WEATHER", info, anim); return; 
  }

  // 2. HEADER
  display.setTextSize(1); display.setCursor(0, 0);
  if (showCityName) display.print(truncateString(currentData.cityName, 72, 1));
  else display.print(getDayName());

  // 3. JAM
  display.setCursor(76, 0); 
  String timeStr = getLocalTime();
  if ((millis() % 1000) > 500) { timeStr.replace(":", " "); }
  display.print(timeStr);

  drawSignal(112, 0, WiFi.RSSI());
  display.drawLine(0, 10, 128, 10, WHITE); 

  // 4. ICON
  drawLargeIcon(0, 18, currentData.iconId); 

  // 5. SUHU
  display.setTextSize(3); 
  display.setCursor(30, 20);
  int tempInt = (int)round(currentData.temp);
  display.print(tempInt);
  
  display.setTextSize(1);
  int xPos = display.getCursorX() - 6; 
  display.setCursor(xPos, 20); 
  display.print((char)247); 
  
  display.setCursor(display.getCursorX() - 1, 20);
  display.print(F("C"));

  // 6. DETAIL KANAN
  int xRight = 76; int spacing = 8; 
  display.setCursor(xRight, 15); display.print(F("H")); 
  display.setCursor(xRight + spacing, 15); display.print(currentData.humidity); display.print(F("%"));

  display.setCursor(xRight, 25); display.print(F("W")); 
  display.setCursor(xRight + spacing, 25); display.print(currentData.windSpeed, 1); display.print(F("m/s"));

  display.setCursor(xRight, 35); display.print(F("P"));
  display.setCursor(xRight + spacing, 35); display.print(currentData.pressure); display.print(F("hPa"));

  // 7. FOOTER
  display.drawLine(0, 50, 128, 50, WHITE); 
  String desc = currentData.desc; desc.toUpperCase();
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(desc, 0, 0, &x1, &y1, &w, &h);
  int xText = (128 - w) / 2; if(xText < 0) xText = 0;
  display.setCursor(xText, 54); display.print(desc);

  display.display();
}

// ================= HELPERS UTILS =================
String getLocalTime() {
  time_t now = time(nullptr); struct tm* timeInfo = localtime(&now); char buf[6]; 
  if (now < 10000) return "--:--"; 
  sprintf(buf, "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min); return String(buf);
}
String getDayName() {
  time_t now = time(nullptr); struct tm* timeInfo = localtime(&now);
  if (now < 10000) return "---"; return String(dayNames[timeInfo->tm_wday]);
}
String truncateString(String text, int width, int textSize) {
  int charWidth = 6 * textSize; int maxChars = width / charWidth;
  if (text.length() > maxChars) return text.substring(0, maxChars - 1) + "."; return text;
}
void drawSignal(int x, int y, int rssi) {
  for(int i=0; i<4; i++) {
    int h = (i+1) * 2;
    if (rssi >= -100 + (i*10)) display.fillRect(x + (i*4), y + (8-h), 3, h, WHITE);
    else display.drawRect(x + (i*4), y + (8-h), 3, h, WHITE);
  }
}
void drawLargeIcon(int x, int y, int id) {
  if (id == 800) { 
    display.drawCircle(x+12, y+12, 6, WHITE); display.drawCircle(x+12, y+12, 2, WHITE);
    display.drawPixel(x+12, y+2, WHITE); display.drawPixel(x+12, y+22, WHITE); display.drawPixel(x+2, y+12, WHITE); display.drawPixel(x+22, y+12, WHITE);
  } else if (id >= 200 && id < 300) { 
    display.fillCircle(x+12, y+10, 6, WHITE); display.fillCircle(x+8, y+12, 5, WHITE); display.fillCircle(x+16, y+12, 5, WHITE);
    display.drawLine(x+12, y+18, x+8, y+22, WHITE); display.drawLine(x+8, y+22, x+12, y+22, WHITE); display.drawLine(x+12, y+22, x+8, y+26, WHITE);
  } else if (id >= 300 && id < 600) { 
    display.fillCircle(x+10, y+10, 6, WHITE); display.fillCircle(x+18, y+10, 6, WHITE); display.fillCircle(x+14, y+6, 6, WHITE);
    display.drawLine(x+8, y+18, x+6, y+22, WHITE); display.drawLine(x+14, y+18, x+12, y+22, WHITE); display.drawLine(x+20, y+18, x+18, y+22, WHITE);
  } else if (id >= 700 && id < 800) { 
    display.drawLine(x+4, y+8, x+20, y+8, WHITE); display.drawLine(x+2, y+12, x+22, y+12, WHITE); display.drawLine(x+4, y+16, x+20, y+16, WHITE);
  } else if (id >= 801) { 
    display.fillCircle(x+8, y+14, 6, WHITE); display.fillCircle(x+20, y+12, 5, WHITE); display.fillCircle(x+14, y+8, 7, WHITE);
  } else { 
    display.drawRect(x+4, y+4, 16, 16, WHITE); display.setCursor(x+8, y+8); display.setTextSize(1); display.print("?");
  }
}
