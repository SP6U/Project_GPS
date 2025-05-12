#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);

// Konfiguracja GPS
#define GPS_RX_PIN 3
#define GPS_TX_PIN 1
#define GPS_BAUD 9600

// Konfiguracja Joysticka
#define JOY_UP_PIN 12
#define JOY_DOWN_PIN 13
#define JOY_LEFT_PIN 14
#define JOY_RIGHT_PIN 25
#define JOY_MID_PIN 26
#define JOY_SET_PIN 27
#define JOY_RST_PIN 16

// Dane WiFi
const char* ssid = "xxxxxxxxxxxxxxxxxxx";
const char* password = "xxxxxxxxxxxxxxxxxx";

// API pogodowe
const String weatherAPIKey = "xxxxxxxxxxxxxxxxxxxx";
String weatherData = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 300000; // 5 minut

// Kolory
#define BACKGROUND_COLOR 0x0000   // Czarny
#define TEXT_COLOR 0xFFFF        // Biały
#define ACCENT_COLOR 0x07FF      // Cyjan
#define WARNING_COLOR 0xF800     // Czerwony
#define HIGHLIGHT_COLOR 0xF81F   // Różowy
#define MENU_BG_COLOR 0x2104     // Ciemnoszary
#define SATELLITE_COLOR 0x5EBC   // Niebiesko-zielony
#define POSITION_COLOR 0xFBE0    // Żółty
#define GRID_COLOR 0x39E7        // Szaro-niebieski

// Kolory ekranu startowego i pogody
#define DARK_GREEN 0x03E0
#define MEDIUM_GREEN 0x07E0
#define LIGHT_GREEN 0xAFE5
#define TEXT_GREEN 0xBFE0
#define BACKGROUND_GREEN 0x02A0
#define WEATHER_BG_COLOR 0x0320
#define WEATHER_TEXT_COLOR 0xBFE0
#define WEATHER_ACCENT_COLOR 0x07E0

// Menu
enum AppState { 
  SPLASH_SCREEN, 
  MAIN_MENU, 
  GPS_RADAR, 
  COMPASS, 
  OFFLINE_MAPS, 
  WEATHER 
};

AppState currentState = SPLASH_SCREEN;
AppState previousState = SPLASH_SCREEN;
int menuSelection = 0;
const char* menuItems[] = {"GPS RADAR", "KOMPAS", "MAPY", "POGODA"};
const int menuItemsCount = 4;

// Rozmiary i pozycje
#define DATA_PANEL_WIDTH 200
#define RADAR_POS_X 260
#define RADAR_POS_Y 70
#define RADAR_RADIUS 88
#define ICON_SIZE 24

// Zmienne globalne
unsigned long lastWiFiUpdate = 0;
const unsigned long wifiUpdateInterval = 5000;
int prevSatCount = -1;
String prevValues[5] = {"", "", "", "", ""};
bool needsRedraw = true;

// Deklaracje funkcji
void clearScreen();
void drawSplashScreen();
void drawMainMenu();
void handleMenuNavigation();
void setupGpsRadar();
void drawGpsRadarScreen();
void setupCompass();
void drawCompassScreen();
void setupOfflineMaps();
void drawOfflineMapsScreen();
void setupWeather();
void drawWeatherScreen();
void drawWifiIcon(int strength);
void drawGPSIcon(int satCount);
void drawRadar(int satCount);
void drawDataPanel();
void updateDataPanel();
void connectToWiFi();
String getLocalTimeString();
String formatTimeDigits(int num);
bool isDST(int year, int month, int day, int hour);
void updateWeatherData();
void changeState(AppState newState);

void clearScreen() {
  tft.fillScreen(BACKGROUND_COLOR);
  needsRedraw = false;
}

void changeState(AppState newState) {
  previousState = currentState;
  currentState = newState;
  needsRedraw = true;
}

void drawSplashScreen() {
  tft.fillScreen(BACKGROUND_GREEN);
  
  // Logo górskie
  tft.fillTriangle(60, 180, 100, 120, 140, 180, DARK_GREEN);
  tft.fillTriangle(120, 180, 160, 100, 200, 180, MEDIUM_GREEN);
  tft.fillTriangle(180, 180, 220, 140, 260, 180, LIGHT_GREEN);
  
  tft.setTextColor(TEXT_GREEN);
  tft.setTextSize(4);
  tft.setCursor(60, 50);
  tft.print("PROJEKT GPS");
  
  tft.setTextSize(2);
  tft.setCursor(90, 90);
  tft.print("Rafal Jakimow");
  
  // Animowane kropki ładowania w kolorach zielonych
  int dotColors[] = {DARK_GREEN, MEDIUM_GREEN, LIGHT_GREEN, TEXT_GREEN};
  for (int i = 0; i < 8; i++) {
    tft.fillCircle(140 + i*20, 220, 6, dotColors[i % 4]);
    delay(200);
  }
  
  delay(1000);
  changeState(MAIN_MENU);
}

void drawMainMenu() {
  if (!needsRedraw && previousState == MAIN_MENU) return;
  
  clearScreen();
  tft.fillScreen(MENU_BG_COLOR);
  
  // Elementy menu
  for (int i = 0; i < menuItemsCount; i++) {
    int y = 42 + i * 70;
    
    if (i == menuSelection) {
      tft.fillRoundRect(30, y, tft.width()-60, 60, 10, DARK_GREEN);
      tft.setTextColor(TEXT_COLOR, DARK_GREEN);
      tft.fillTriangle(tft.width()-50, y+25, tft.width()-70, y+15, tft.width()-70, y+35, TEXT_COLOR);
    } else {
      tft.fillRoundRect(30, y, tft.width()-60, 60, 10, BACKGROUND_COLOR);
      tft.drawRoundRect(30, y, tft.width()-60, 60, 10, ACCENT_COLOR);
      tft.setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
    }
    
    tft.setTextSize(3);
    tft.setCursor(50, y + 20);
    tft.print(menuItems[i]);
  }
  
  // Ikona WiFi
  int rssi = WiFi.RSSI();
  int strength = map(constrain(rssi, -100, -50), -100, -50, 0, 100);
  drawWifiIcon(strength);
  
  previousState = MAIN_MENU;
  needsRedraw = false;
}

void handleMenuNavigation() {
  static unsigned long lastButtonPress = 0;
  if (millis() - lastButtonPress < 200) return;
  
  if (digitalRead(JOY_UP_PIN) == LOW) {
    menuSelection = (menuSelection - 1 + menuItemsCount) % menuItemsCount;
    needsRedraw = true;
    lastButtonPress = millis();
  }
  if (digitalRead(JOY_DOWN_PIN) == LOW) {
    menuSelection = (menuSelection + 1) % menuItemsCount;
    needsRedraw = true;
    lastButtonPress = millis();
  }
  if (digitalRead(JOY_LEFT_PIN) == LOW) {
    changeState(MAIN_MENU);
    lastButtonPress = millis();
  }
  if (digitalRead(JOY_MID_PIN) == LOW) {
    switch (menuSelection) {
      case 0: changeState(GPS_RADAR); break;
      case 1: changeState(COMPASS); break;
      case 2: changeState(OFFLINE_MAPS); break;
      case 3: changeState(WEATHER); break;
    }
    lastButtonPress = millis();
  }
}

void setupGpsRadar() {
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  prevSatCount = -1;
  for (int i = 0; i < 5; i++) prevValues[i] = "";
}

void drawGpsRadarScreen() {
  if (!needsRedraw && previousState == GPS_RADAR) return;
  
  clearScreen();
  drawGPSIcon(0);
  drawDataPanel();
  drawRadar(0);
  
  previousState = GPS_RADAR;
  needsRedraw = false;
}

void setupCompass() {
  // Inicjalizacja kompasu
}

void drawCompassScreen() {
  if (!needsRedraw && previousState == COMPASS) return;
  
  clearScreen();
  tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(3);
  tft.setCursor(80, 100);
  tft.print("KOMPAS");
  tft.drawCircle(tft.width()/2, tft.height()/2, 80, ACCENT_COLOR);
  
  previousState = COMPASS;
  needsRedraw = false;
}

void setupOfflineMaps() {
  // Inicjalizacja map offline
}

void drawOfflineMapsScreen() {
  if (!needsRedraw && previousState == OFFLINE_MAPS) return;
  
  clearScreen();
  tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(3);
  tft.setCursor(60, 100);
  tft.print("MAPY OFFLINE");
  
  previousState = OFFLINE_MAPS;
  needsRedraw = false;
}

void setupWeather() {
  weatherData = "";
  updateWeatherData();
}

void drawWeatherScreen() {
  if (!needsRedraw && previousState == WEATHER) return;
  
  clearScreen();
  tft.fillScreen(WEATHER_BG_COLOR);
  
  tft.setTextColor(WEATHER_TEXT_COLOR);
  tft.setTextSize(3);
  tft.setCursor(100, 20);
  tft.print("POGODA");
  
  // Rysuj ikony pogodowe
  tft.fillCircle(50, 80, 20, WEATHER_ACCENT_COLOR); // Słońce
  for (int i = 0; i < 8; i++) {
    float angle = i * PI / 4;
    int x = 50 + cos(angle) * 30;
    int y = 80 + sin(angle) * 30;
    tft.drawLine(50, 80, x, y, WEATHER_ACCENT_COLOR);
  }
  
  if (weatherData == "") {
    tft.setTextColor(WEATHER_TEXT_COLOR, WEATHER_BG_COLOR);
    tft.setTextSize(2);
    tft.setCursor(30, 120);
    tft.print("Pobieram dane...");
  } else {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, weatherData);
    
    float temp = doc["main"]["temp"];
    int pressure = doc["main"]["pressure"];
    float windSpeed = doc["wind"]["speed"];
    
    // Temperatura
    tft.setTextColor(WEATHER_TEXT_COLOR, WEATHER_BG_COLOR);
    tft.setTextSize(2);
    tft.setCursor(30, 80);
    tft.print("Temperatura:");
    tft.setTextSize(3);
    tft.setCursor(30, 110);
    tft.print(String(temp, 1) + " C");
    
    // Ciśnienie
    tft.setTextSize(2);
    tft.setCursor(30, 150);
    tft.print("Cisnienie:");
    tft.setTextSize(3);
    tft.setCursor(30, 180);
    tft.print(String(pressure) + " hPa");
    
    // Wiatr
    tft.setTextSize(2);
    tft.setCursor(30, 220);
    tft.print("Predkosc wiatru:");
    tft.setTextSize(3);
    tft.setCursor(30, 250);
    tft.print(String(windSpeed, 1) + " m/s");
  }
  
  previousState = WEATHER;
  needsRedraw = false;
}

void updateWeatherData() {
  if (!gps.location.isValid()) return;
  if (millis() - lastWeatherUpdate < weatherUpdateInterval) return;
  
  String lat = String(gps.location.lat(), 6);
  String lon = String(gps.location.lng(), 6);
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + weatherAPIKey;
  
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    weatherData = http.getString();
    lastWeatherUpdate = millis();
    if (currentState == WEATHER) {
      needsRedraw = true;
    }
  }
  
  http.end();
}

void drawWifiIcon(int strength) {
  int x = tft.width() - 34;
  int y = 10;
  int bars = map(constrain(strength, 0, 100), 0, 100, 0, 4);
  
  tft.fillRect(x, y, 28, 20, (currentState == MAIN_MENU) ? MENU_BG_COLOR : BACKGROUND_COLOR);
  
  for (int i = 0; i < 4; i++) {
    if (i < bars) {
      tft.fillRect(x + i * 6, y + 16 - (i + 1) * 5, 4, (i + 1) * 5, 
                  (i >= 2) ? ACCENT_COLOR : TEXT_COLOR);
    }
  }
}

void drawGPSIcon(int satCount) {
  int x = 10, y = 10;
  
  tft.fillRect(x, y, ICON_SIZE, ICON_SIZE, BACKGROUND_COLOR);
  
  if (satCount > 0) {
    tft.fillCircle(x + ICON_SIZE/2, y + ICON_SIZE/2, 3, ACCENT_COLOR);
    tft.drawCircle(x + ICON_SIZE/2, y + ICON_SIZE/2, 6, ACCENT_COLOR);
    tft.drawCircle(x + ICON_SIZE/2, y + ICON_SIZE/2, 9, ACCENT_COLOR);
    
    tft.fillCircle(x + ICON_SIZE/2, y + 4, 2, (satCount >= 1) ? SATELLITE_COLOR : TEXT_COLOR);
    tft.fillCircle(x + ICON_SIZE-4, y + ICON_SIZE/2, 2, (satCount >= 2) ? SATELLITE_COLOR : TEXT_COLOR);
    tft.fillCircle(x + ICON_SIZE/2, y + ICON_SIZE-4, 2, (satCount >= 3) ? SATELLITE_COLOR : TEXT_COLOR);
    tft.fillCircle(x + 4, y + ICON_SIZE/2, 2, (satCount >= 4) ? SATELLITE_COLOR : TEXT_COLOR);
  } else {
    tft.setTextColor(WARNING_COLOR, BACKGROUND_COLOR);
    tft.drawString("!", x + ICON_SIZE/2 - 3, y + 3, 2);
  }
}

void drawRadar(int satCount) {
  tft.fillCircle(RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y + RADAR_RADIUS, RADAR_RADIUS, BACKGROUND_COLOR);
  tft.drawCircle(RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y + RADAR_RADIUS, RADAR_RADIUS, GRID_COLOR);
  tft.drawCircle(RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y + RADAR_RADIUS, RADAR_RADIUS/2, GRID_COLOR);
  
  tft.drawLine(RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y, 
               RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y + 2*RADAR_RADIUS, GRID_COLOR);
  tft.drawLine(RADAR_POS_X, RADAR_POS_Y + RADAR_RADIUS, 
               RADAR_POS_X + 2*RADAR_RADIUS, RADAR_POS_Y + RADAR_RADIUS, GRID_COLOR);
  
  tft.fillCircle(RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y + RADAR_RADIUS, 4, POSITION_COLOR);
  
  if (satCount > 0) {
    for (int i = 0; i < min(satCount, 12); i++) {
      float angle = random(0, 360) * PI / 180.0;
      float distance = random(RADAR_RADIUS/3, RADAR_RADIUS);
      int x = RADAR_POS_X + RADAR_RADIUS + cos(angle) * distance;
      int y = RADAR_POS_Y + RADAR_RADIUS + sin(angle) * distance;
      
      tft.fillCircle(x, y, 3, SATELLITE_COLOR);
      tft.drawLine(RADAR_POS_X + RADAR_RADIUS, RADAR_POS_Y + RADAR_RADIUS, x, y, 0x4A69);
    }
  }
  
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(1);
  tft.setCursor(RADAR_POS_X + 10, RADAR_POS_Y + 2*RADAR_RADIUS + 10);
  tft.print("SAT: ");
  tft.print(satCount);
}

void drawDataPanel() {
  tft.fillRect(0, 50, DATA_PANEL_WIDTH, tft.height() - 50, BACKGROUND_COLOR);
  
  tft.setTextColor(ACCENT_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(2);
  
  String labels[5] = {"Szerokosc:", "Dlugosc:", "Wysokosc:", "Godzina: ", "Satelity:"};
  for (int i = 0; i < 5; i++) {
    tft.setCursor(10, 60 + i * 40);
    tft.print(labels[i]);
  }
  
  for (int i = 0; i < 4; i++) {
    tft.drawFastHLine(10, 85 + i * 40, DATA_PANEL_WIDTH - 20, GRID_COLOR);
  }
}

void updateDataPanel() {
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.setTextSize(2);
  
  String latStr = gps.location.isValid() ? String(gps.location.lat(), 6) : "---.------";
  if (latStr != prevValues[0]) {
    tft.fillRect(130, 60, DATA_PANEL_WIDTH - 140, 20, BACKGROUND_COLOR);
    tft.setCursor(130, 60);
    tft.print(latStr);
    prevValues[0] = latStr;
  }
  
  String lngStr = gps.location.isValid() ? String(gps.location.lng(), 6) : "---.------";
  if (lngStr != prevValues[1]) {
    tft.fillRect(130, 100, DATA_PANEL_WIDTH - 140, 20, BACKGROUND_COLOR);
    tft.setCursor(130, 100);
    tft.print(lngStr);
    prevValues[1] = lngStr;
  }
  
  String altStr = gps.altitude.isValid() ? String(gps.altitude.meters(), 1) + " m" : "---.- m";
  if (altStr != prevValues[2]) {
    tft.fillRect(130, 140, DATA_PANEL_WIDTH - 140, 20, BACKGROUND_COLOR);
    tft.setCursor(130, 140);
    tft.print(altStr);
    prevValues[2] = altStr;
  }
  
  String timeStr = getLocalTimeString();
  if (timeStr != prevValues[3]) {
    tft.fillRect(130, 180, DATA_PANEL_WIDTH - 140, 20, BACKGROUND_COLOR);
    tft.setCursor(130, 180);
    tft.print(timeStr);
    prevValues[3] = timeStr;
  }
  
  String satStr = String(gps.satellites.value());
  if (satStr != prevValues[4]) {
    tft.fillRect(130, 220, DATA_PANEL_WIDTH - 140, 20, BACKGROUND_COLOR);
    tft.setCursor(130, 220);
    tft.print(satStr);
    prevValues[4] = satStr;
  }
  
  if (!gps.location.isValid()) {
    tft.setTextColor(WARNING_COLOR, BACKGROUND_COLOR);
    tft.setCursor(10, 260);
    tft.print("Szukam satelit...");
  } else {
    tft.fillRect(10, 260, DATA_PANEL_WIDTH - 20, 20, BACKGROUND_COLOR);
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  drawWifiIcon(100);
}

String formatTimeDigits(int num) {
  if (num < 10) return "0" + String(num);
  return String(num);
}

bool isDST(int year, int month, int day, int hour) {
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;
  
  int lastSundayMarch = (31 - (5 * year / 4 + 4) % 7);
  int lastSundayOctober = (31 - (5 * year / 4 + 1) % 7);
  
  if (month == 3) {
    if (day > lastSundayMarch) return true;
    if (day < lastSundayMarch) return false;
    return hour >= 2;
  }
  
  if (month == 10) {
    if (day < lastSundayOctober) return true;
    if (day > lastSundayOctober) return false;
    return hour < 3;
  }
  
  return false;
}

String getLocalTimeString() {
  if (!gps.time.isValid() || !gps.date.isValid()) return "--:--:--";
  
  int hour = gps.time.hour();
  int minute = gps.time.minute();
  int second = gps.time.second();
  int day = gps.date.day();
  int month = gps.date.month();
  int year = gps.date.year();
  
  bool dst = isDST(year, month, day, hour);
  hour += dst ? 2 : 1;
  
  if (hour >= 24) {
    hour -= 24;
    day++;
  }
  
  return formatTimeDigits(hour) + ":" + 
         formatTimeDigits(minute) + ":" + 
         formatTimeDigits(second);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  
  pinMode(JOY_UP_PIN, INPUT_PULLUP);
  pinMode(JOY_DOWN_PIN, INPUT_PULLUP);
  pinMode(JOY_LEFT_PIN, INPUT_PULLUP);
  pinMode(JOY_RIGHT_PIN, INPUT_PULLUP);
  pinMode(JOY_MID_PIN, INPUT_PULLUP);
  pinMode(JOY_SET_PIN, INPUT_PULLUP);
  pinMode(JOY_RST_PIN, INPUT_PULLUP);
  
  drawSplashScreen();
  connectToWiFi();
}

void loop() {
  switch (currentState) {
    case SPLASH_SCREEN:
      break;
      
    case MAIN_MENU:
      if (needsRedraw) drawMainMenu();
      handleMenuNavigation();
      
      // Aktualizacja ikony WiFi w menu
      if (millis() - lastWiFiUpdate > wifiUpdateInterval) {
        int rssi = WiFi.RSSI();
        int strength = map(constrain(rssi, -100, -50), -100, -50, 0, 100);
        drawWifiIcon(strength);
        lastWiFiUpdate = millis();
      }
      break;
      
    case GPS_RADAR:
      if (needsRedraw) {
        setupGpsRadar();
        drawGpsRadarScreen();
      }
      
      while (SerialGPS.available() > 0) {
        if (gps.encode(SerialGPS.read())) {
          int satCount = gps.satellites.value();
          if (satCount != prevSatCount) {
            drawGPSIcon(satCount);
            drawRadar(satCount);
            prevSatCount = satCount;
          }
          updateDataPanel();
          updateWeatherData();
        }
      }
      
      if (digitalRead(JOY_LEFT_PIN) == LOW) {
        changeState(MAIN_MENU);
      }
      break;
      
    case COMPASS:
      if (needsRedraw) {
        setupCompass();
        drawCompassScreen();
      }
      
      if (digitalRead(JOY_LEFT_PIN) == LOW) {
        changeState(MAIN_MENU);
      }
      break;
      
    case OFFLINE_MAPS:
      if (needsRedraw) {
        setupOfflineMaps();
        drawOfflineMapsScreen();
      }
      
      if (digitalRead(JOY_LEFT_PIN) == LOW) {
        changeState(MAIN_MENU);
      }
      break;
      
    case WEATHER:
      if (needsRedraw) {
        setupWeather();
        drawWeatherScreen();
      }
      
      updateWeatherData();
      
      if (digitalRead(JOY_LEFT_PIN) == LOW) {
        changeState(MAIN_MENU);
      }
      break;
  }
  
  delay(50);
}