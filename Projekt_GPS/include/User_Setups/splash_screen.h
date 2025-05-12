#pragma once
#include <TFT_eSPI.h>

// Kolory w odcieniach zieleni
#define DARK_GREEN 0x03E0
#define MEDIUM_GREEN 0x07E0
#define LIGHT_GREEN 0xAFE5
#define TEXT_GREEN 0xBFE0
#define BACKGROUND_GREEN 0x02A0

void drawSplashScreen(TFT_eSPI &tft) {
    // Wypełnij tło
    tft.fillScreen(BACKGROUND_GREEN);
    
    // Narysuj stylizowany tytuł
    tft.setTextColor(TEXT_GREEN);
    tft.setTextSize(3);
    tft.setCursor(40, 50);
    tft.print("Projekt GPS");
    
    tft.setTextSize(2);
    tft.setCursor(80, 90);
    tft.print("Rafal Jakimow");
    
    // Narysuj prostą grafikę gór
    int mountainY = 200;
    int mountainHeight = 120;
    
    // Góra 1
    tft.fillTriangle(50, mountainY + mountainHeight, 
                    150, mountainY + mountainHeight, 
                    100, mountainY, MEDIUM_GREEN);
    
    // Góra 2
    tft.fillTriangle(150, mountainY + mountainHeight, 
                    250, mountainY + mountainHeight, 
                    200, mountainY - 20, DARK_GREEN);
    
    // Góra 3
    tft.fillTriangle(250, mountainY + mountainHeight, 
                    350, mountainY + mountainHeight, 
                    300, mountainY, LIGHT_GREEN);
    
    // Dodaj prostą mapę w tle
    tft.drawRect(50, 350, 220, 100, LIGHT_GREEN);
    tft.drawLine(50, 400, 270, 400, LIGHT_GREEN);
    tft.drawLine(160, 350, 160, 450, LIGHT_GREEN);
    
    // Dodaj kompas
    tft.fillCircle(280, 380, 30, MEDIUM_GREEN);
    tft.drawCircle(280, 380, 35, TEXT_GREEN);
    tft.setTextSize(1);
    tft.setCursor(275, 365);
    tft.print("N");
    tft.setCursor(275, 395);
    tft.print("S");
    tft.setCursor(255, 380);
    tft.print("W");
    tft.setCursor(295, 380);
    tft.print("E");
    
    // Dodaj animację ładowania
    for (int i = 0; i < 10; i++) {
        tft.fillRect(100 + i * 12, 300, 8, 8, i % 2 == 0 ? LIGHT_GREEN : DARK_GREEN);
        delay(100);
    }
}