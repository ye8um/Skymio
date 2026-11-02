/*
  Skymio — 3-Day Weather Display
  Board: Seeed Studio XIAO ESP32C3
  Display: Waveshare 2.4" SPI LCD (ILI9341, 240x320)
  Libraries: Adafruit GFX, Adafruit ILI9341, ArduinoJson (v7)

*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ArduinoJson.h>

// ----- SETTINGS -----
const char* WIFI_SSID     = "Verizon_KD4Q6C";
const char* WIFI_PASSWORD = "sat6_gules_kith";

constexpr float LATITUDE  = 38.89f;
constexpr float LONGITUDE = -77.09f;

// ----- PCB CONNECTIONS: GPIO NUMBERS -----
// LCD DIN = D10 (GPIO10), CLK = D8 (GPIO8), CS = D3 (GPIO5)
// LCD DC = D4 (GPIO6), RST = D5 (GPIO7), BL = 3V3 (always on)
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 8;
constexpr int TFT_CS   = 5;
constexpr int TFT_DC   = 6;
constexpr int TFT_RST  = 7;
constexpr int TFT_MISO = -1;

// Forecast update every 15 min. Retry every 60 sec after a failure.
constexpr unsigned long WEATHER_INTERVAL_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long RETRY_INTERVAL_MS   = 60UL * 1000UL;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

const uint16_t NAVY = 0x1083;
const uint16_t PANEL = 0x2186;
const uint16_t WHITE = 0xFFFF;
const uint16_t SUBTLE = 0xB5B6;
const uint16_t BLUE = 0x651F;
const uint16_t YELLOW = 0xFEA0;
const uint16_t GREEN = 0x7E6F;

struct DayForecast {
  String date;
  int code = -1;
  float high = NAN;
  float low = NAN;
  int rainChance = -1;
};

struct WeatherData {
  float currentTemp = NAN;
  float rain = 0;
  float showers = 0;
  int currentCode = -1;
  DayForecast day[3];
  bool valid = false;
};

WeatherData weather;
unsigned long lastAttemptMs = 0;
unsigned long nextDelayMs = 0;

String dayName(const String& isoDate) {
  if (isoDate.length() < 10) return "---";
  int y = isoDate.substring(0, 4).toInt();
  int m = isoDate.substring(5, 7).toInt();
  int d = isoDate.substring(8, 10).toInt();
  // Sakamoto algorithm: 0 = Sunday
  const int offsets[] = {0,3,2,5,0,3,5,1,4,6,2,4};
  if (m < 1 || m > 12) return "---";
  y -= m < 3;
  int day = (y + y/4 - y/100 + y/400 + offsets[m-1] + d) % 7;
  const char* names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  return names[day];
}

bool rainyCode(int code) {
  return (code >= 51 && code <= 67) || (code >= 80 && code <= 82) ||
         (code >= 95 && code <= 99);
}

String weatherDescription(int code) {
  if (code == 0) return "Clear";
  if (code == 1 || code == 2) return "Partly cloudy";
  if (code == 3) return "Cloudy";
  if (code == 45 || code == 48) return "Foggy";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 85 && code <= 86) return "Snow showers";
  if (code >= 95 && code <= 99) return "Thunderstorms";
  return "Weather";
}

void centeredText(const String& s, int x, int y, uint8_t size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  int w = (int)s.length() * 6 * size;
  tft.setCursor(x - w/2, y);
  tft.print(s);
}


void cloud(int cx, int cy, uint16_t color) {
  tft.fillRoundRect(cx - 19, cy + 2, 39, 14, 7, color);
  tft.fillCircle(cx - 9, cy + 3, 11, color);
  tft.fillCircle(cx + 7, cy - 1, 13, color);
}
void sun(int cx, int cy) {
  tft.fillCircle(cx, cy, 10, YELLOW);
  for (int i=0; i<8; ++i) {
    float a = i * 3.14159265f / 4.0f;
    int x1 = cx + (int)(14 * cos(a));
    int y1 = cy + (int)(14 * sin(a));
    int x2 = cx + (int)(19 * cos(a));
    int y2 = cy + (int)(19 * sin(a));
    tft.drawLine(x1,y1,x2,y2,YELLOW);
  }
}
void iconForCode(int code, int x, int y) {
  if (code == 0) { sun(x,y); return; }
  if (code == 1 || code == 2) { sun(x + 12,y - 9); cloud(x - 2,y + 2,WHITE); return; }
  cloud(x,y,WHITE);
  if (rainyCode(code)) {
    for (int i=-1;i<=1;i++) tft.drawLine(x+i*11,y+20,x+i*11-4,y+29,BLUE);
  } else if (code >= 71 && code <= 86) {
    for (int i=-1;i<=1;i++) tft.fillCircle(x+i*11,y+24,2,WHITE);
  }
}

void drawScreen() {
  tft.fillScreen(NAVY);
  tft.fillRect(0,0,320,28,PANEL);
  tft.setTextColor(WHITE); tft.setTextSize(2); tft.setCursor(10,7); tft.print("SKYMIO");
  tft.setTextSize(1); tft.setTextColor(SUBTLE); tft.setCursor(273,10); tft.print("WiFi");

  if (!weather.valid) {
    centeredText("Waiting for weather",160,95,2,WHITE);
    centeredText("Check Wi-Fi and settings",160,128,1,SUBTLE);
    return;
  }

  tft.setTextSize(5); tft.setTextColor(WHITE);
  tft.setCursor(14,39); tft.printf("%.0f", weather.currentTemp);
  tft.setTextSize(2); tft.print("F");
  iconForCode(weather.currentCode,265,68);
  tft.setTextSize(2); tft.setTextColor(SUBTLE);
  tft.setCursor(15,87); tft.print(weatherDescription(weather.currentCode));
  bool rainNow = (weather.rain > 0.0f || weather.showers > 0.0f || rainyCode(weather.currentCode));
  tft.setTextSize(2); tft.setTextColor(rainNow ? BLUE : GREEN);
  tft.setCursor(15,110); tft.print(rainNow ? "Rain reported" : "No rain reported");

  tft.drawFastHLine(10,136,300,SUBTLE);
  tft.setTextColor(SUBTLE); tft.setTextSize(1); tft.setCursor(12,144); tft.print("3-DAY FORECAST");

  for (int i = 0; i < 3; ++i) {
    int x = 8 + i*105;
    tft.fillRoundRect(x,157,98,82,5,PANEL);
    centeredText(i == 0 ? "TODAY" : dayName(weather.day[i].date),x+49,162,1,WHITE);
    iconForCode(weather.day[i].code,x+49,178);
    String temps = String((int)round(weather.day[i].high)) + "/" +
                   String((int)round(weather.day[i].low));
    centeredText(temps,x+49,202,2,WHITE);
    String rainTxt = weather.day[i].rainChance >= 0 ?
      String(weather.day[i].rainChance) + "% rain" : "Rain: --";
    centeredText(rainTxt,x+49,230,1,SUBTLE);
  }
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(LATITUDE,5) +
    "&longitude=" + String(LONGITUDE,5) +
    "&current=temperature_2m,rain,showers,weather_code" +
    "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max" +
    "&temperature_unit=fahrenheit&timezone=auto&forecast_days=3";

  WiFiClientSecure client;

  client.setInsecure();
  HTTPClient http;
  http.setTimeout(12000);
  if (!http.begin(client,url)) return false;
  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error: %d\n",status);
    http.end(); return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc,http.getStream());
  http.end();
  if (err) { Serial.printf("JSON error: %s\n",err.c_str()); return false; }
  if (doc["current"]["temperature_2m"].isNull() || doc["daily"]["time"].size() < 3) {
    Serial.println("Incomplete weather response"); return false;
  }

  WeatherData w;
  w.currentTemp = doc["current"]["temperature_2m"].as<float>();
  w.rain = doc["current"]["rain"] | 0.0f;
  w.showers = doc["current"]["showers"] | 0.0f;
  w.currentCode = doc["current"]["weather_code"] | -1;
  for (int i=0;i<3;i++) {
    w.day[i].date = doc["daily"]["time"][i].as<String>();
    w.day[i].code = doc["daily"]["weather_code"][i] | -1;
    w.day[i].high = doc["daily"]["temperature_2m_max"][i].as<float>();
    w.day[i].low = doc["daily"]["temperature_2m_min"][i].as<float>();
    w.day[i].rainChance = doc["daily"]["precipitation_probability_max"][i] | -1;
  }
  w.valid = true;
  weather = w;
  return true;
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis()-start < 15000UL) {
    delay(250); Serial.print('.');
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " failed");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  SPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS);
  tft.begin();
  tft.setRotation(1);
  connectWiFi();
  if (fetchWeather()) nextDelayMs = WEATHER_INTERVAL_MS;
  else nextDelayMs = RETRY_INTERVAL_MS;
  lastAttemptMs = millis();
  drawScreen();
}

void loop() {
  if (millis()-lastAttemptMs >= nextDelayMs) {
    connectWiFi();
    bool ok = fetchWeather();
    nextDelayMs = ok ? WEATHER_INTERVAL_MS : RETRY_INTERVAL_MS;
    lastAttemptMs = millis();
    if (ok || !weather.valid) drawScreen();
  }
  delay(100);
}
