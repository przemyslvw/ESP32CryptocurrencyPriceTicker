#include <TFT_eSPI.h>         // Biblioteka wyświetlacza TFT
#include <XPT2046_Touchscreen.h> // Ekran dotykowy
#include <WiFi.h>             // Do połączenia z Wi-Fi
#include <HTTPClient.h>       // Do obsługi API HTTP
#include <ArduinoJson.h>      // Do przetwarzania danych JSON

// Wi-Fi credentials
#include "WiFiConfig.h"

// API settings
const char* api_url_main = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,dogecoin,pepe,cardano,avalanche,mew&vs_currencies=usd&include_24hr_change=true";
const char* api_url_details_template = "https://api.coingecko.com/api/v3/coins/"; // Dodamy ID kryptowaluty

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

// Wyświetlanie szczegółów kryptowaluty
void displayCryptoDetails(String cryptoId) {
  String api_url = String(api_url_details_template) + cryptoId;
  String jsonData = fetchCryptoData(api_url.c_str());
  
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(RED);
    tft.print("Blad JSON");
    return;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Kryptowaluta: ");
  tft.println(cryptoId);

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("Cena: $");
  tft.println(doc["market_data"]["current_price"]["usd"].as<float>());

  tft.setCursor(10, 60);
  tft.print("Zmiana 24h: ");
  tft.println(doc["market_data"]["price_change_percentage_24h"].as<float>());

  tft.setCursor(10, 80);
  tft.print("Najwyzsza 7d: ");
  tft.println(doc["market_data"]["high_24h"]["usd"].as<float>());

  tft.setCursor(10, 100);
  tft.print("Najnizsza 7d: ");
  tft.println(doc["market_data"]["low_24h"]["usd"].as<float>());

  // Dodaj wykres liniowy (funkcję dodamy później)
  // drawLineGraph(data_array, 240, 150);
}

// Obsługa dotyku
void handleTouch() {
  if (!ts.touched()) return;

  TS_Point p = ts.getPoint();
  int x = p.x, y = p.y;

  // Skalowanie dotyku (może wymagać kalibracji)
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
      currentView = 1; // Przejście do szczegółów
      displayCryptoDetails(selectedCrypto);
    }
  } else if (currentView == 1) {
    // Przycisk powrotu
    if (x > 200 && y > 280) {
      currentView = 0;
      selectedCrypto = "";
      // Wyświetl tabelę główną
      String data = fetchCryptoData(api_url_main);
      displayCryptoTable(data);
    }
  }
}

void displayCryptoTable(String jsonData) {
  // Jak w poprzedniej wersji
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(1);

  connectToWiFi();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(WHITE);
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
  delay(100); // Krótka pauza dla dotyku
}
