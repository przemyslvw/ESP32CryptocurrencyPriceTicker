#include <TFT_eSPI.h>         // Biblioteka wyświetlacza TFT wymaga odpowiedniej konfiguracji 
#include <XPT2046_Touchscreen.h> // Ekran dotykowy
#include <WiFi.h>             // Do połączenia z Wi-Fi
#include <HTTPClient.h>       // Do obsługi API HTTP
#include <ArduinoJson.h>      // Do przetwarzania danych JSON

// Wi-Fi credentials
#include "WiFiConfig.h"

// API settings
const char* api_url_main = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,dogecoin,pepe,cardano,avalanche,mew&vs_currencies=usd&include_24hr_change=true";
const char* api_url_details_template = "https://api.coingecko.com/api/v3/coins/"; // ID kryptowaluty dopełnione dynamicznie

// TFT display
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(33); // CS pin dla dotyku

// Aktualny widok (0 = tabela, 1 = szczegóły)
int currentView = 0;
String selectedCrypto = "";

// Funkcja inicjalizacji Wi-Fi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Połączono!");
}

// Pobieranie danych z API
String fetchCryptoData(const char* url) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      return http.getString();
    } else {
      Serial.println("Błąd podczas pobierania danych z API");
      return "";
    }
    http.end();
  }
  return "";
}

// Funkcja rysująca wykres liniowy
void drawLineGraph(float data[], int dataCount, int x, int y, int width, int height) {
  float minValue = data[0], maxValue = data[0];

  // Znalezienie minimalnej i maksymalnej wartości
  for (int i = 1; i < dataCount; i++) {
    if (data[i] < minValue) minValue = data[i];
    if (data[i] > maxValue) maxValue = data[i];
  }

  // Rysowanie osi
  tft.drawRect(x, y, width, height, TFT_WHITE);

  // Przeliczanie wartości na piksele
  for (int i = 0; i < dataCount - 1; i++) {
    int x0 = x + (i * (width / (dataCount - 1)));
    int y0 = y + height - ((data[i] - minValue) / (maxValue - minValue) * height);
    int x1 = x + ((i + 1) * (width / (dataCount - 1)));
    int y1 = y + height - ((data[i + 1] - minValue) / (maxValue - minValue) * height);
    
    tft.drawLine(x0, y0, x1, y1, TFT_GREEN);
  }
}

// Wyświetlanie szczegółów kryptowaluty
void displayCryptoDetails(String cryptoId) {
  String api_url = String(api_url_details_template) + cryptoId + "/market_chart?vs_currency=usd&days=7";
  String jsonData = fetchCryptoData(api_url.c_str());

  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED);
    tft.print("Blad JSON");
    return;
  }

  // Parsowanie danych historycznych
  const JsonArray& prices = doc["prices"];
  float data[7];
  for (int i = 0; i < 7; i++) {
    data[i] = prices[i][1]; // Druga kolumna to cena
  }

  // Wyświetlenie wykresu
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Kryptowaluta: ");
  tft.println(cryptoId);

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("Wykres 7 dni");

  drawLineGraph(data, 7, 10, 60, 220, 150);

  // Dodanie przycisku powrotu
  tft.fillRect(200, 280, 40, 30, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(210, 290);
  tft.print("Back");
}

// Obsługa dotyku
void handleTouch() {
  if (!ts.touched()) return;

  TS_Point p = ts.getPoint();
  int x = p.x, y = p.y;

  x = map(x, 0, 4095, 0, SCREEN_WIDTH);
  y = map(y, 0, 4095, 0, SCREEN_HEIGHT);

  if (currentView == 0) {
    // Kliknięcie w tabeli kryptowalut
    if (y > 10 && y < 40) selectedCrypto = "bitcoin";
    else if (y > 40 && y < 70) selectedCrypto = "dogecoin";
    else if (y > 70 && y < 100) selectedCrypto = "pepe";
    else if (y > 100 && y < 130) selectedCrypto = "cardano";
    else if (y > 130 && y < 160) selectedCrypto = "avalanche";
    else if (y > 160 && y < 190) selectedCrypto = "mew";

    if (selectedCrypto != "") {
      currentView = 1;
      displayCryptoDetails(selectedCrypto);
    }
  } else if (currentView == 1) {
    if (x > 200 && y > 280) {
      currentView = 0;
      String data = fetchCryptoData(api_url_main);
      displayCryptoTable(data);
    }
  }
}

void displayCryptoTable(String jsonData) {
  // Funkcja tabeli pozostaje jak wcześniej
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(1);

  connectToWiFi();

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Ladowanie danych...");
}

void loop() {
  if (currentView == 0) {
    String data = fetchCryptoData(api_url_main);
    if (data != "") {
      displayCryptoTable(data);
    }
  }
  handleTouch();
  delay(100);
}
