#define USER_SETUP_INFO "ST7796 Basic Setup"

#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Konfiguracja pinów
#define TFT_CS   5   // Chip Select
#define TFT_DC   22  // Data/Command
#define TFT_RST  21  // Reset
#define TFT_BL   4   // (PWM)

// Konfiguracja SPI
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1  // Nie używany

#define SPI_FREQUENCY  74000000
#define SUPPORT_TRANSACTIONS 1