#include <TFT_eSPI.h>         // Biblioteka wyświetlacza TFT, wymaga konfiguracji w User_Setup.h
#include <XPT2046_Touchscreen.h> // Obsługa dotykowego ekranu
#include <WiFi.h>             // Do połączenia z Wi-Fi
#include <HTTPClient.h>       // Do obsługi zapytań HTTP
#include <ArduinoJson.h>      // Do przetwarzania danych JSON

#include "WiFiConfig.h"       // Plik z Wi-Fi credentials (ssid, password)

// Konfiguracja API
const char* api_url_main = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,dogecoin,pepe,cardano,avalanche,mew&vs_currencies=usd&include_24hr_change=true";
const char* api_url_details_template = "https://api.coingecko.com/api/v3/coins/";

// Inicjalizacja TFT i dotyku
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(33); // CS dla dotyku ustawiony na GPIO 33

// Aktualny widok
int currentView = 0; // 0 = tabela, 1 = szczegóły
String selectedCrypto = ""; // ID wybranej kryptowaluty

// Konfiguracja wykresu
const int graphWidth = 220;
const int graphHeight = 150;
const int graphX = 10;
const int graphY = 60;

// Zmienna przechowująca status Wi-Fi
bool isWiFiConnected = false;

// Funkcja inicjalizująca Wi-Fi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z Wi-Fi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nPołączono z Wi-Fi!");
    isWiFiConnected = true;
  } else {
    Serial.println("\nNie udało się połączyć z Wi-Fi.");
    isWiFiConnected = false;
  }
}

// Pobieranie danych z API
String fetchCryptoData(const char* url) {
  if (!isWiFiConnected) {
    Serial.println("Wi-Fi rozłączone. Brak możliwości pobrania danych.");
    return "";
  }

  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    http.end(); // Zwalniamy zasoby
    return response;
  } else {
    Serial.printf("Błąd HTTP: %d\n", httpResponseCode);
    http.end(); // Zwalniamy zasoby
    return "";
  }
}

// Wyświetlanie tabeli kryptowalut
void displayCryptoTable(String jsonData) {
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.print("Błąd danych JSON");
    return;
  }

  // Wyświetlenie nagłówka
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.print("Tabela kryptowalut:");

  // Wyświetlenie listy kryptowalut
  int y = 40;
  for (String crypto : {"bitcoin", "dogecoin", "pepe", "cardano", "avalanche", "mew"}) {
    float price = doc[crypto]["usd"];
    float change = doc[crypto]["usd_24h_change"];

    tft.setCursor(10, y);
    tft.printf("%s: %.2f USD", crypto.c_str(), price);

    // Zmiana koloru dla wzrostu/spadku
    if (change >= 0) tft.setTextColor(TFT_GREEN);
    else tft.setTextColor(TFT_RED);

    tft.printf(" (%.2f%%)", change);
    tft.setTextColor(TFT_WHITE);
    y += 30; // Odstęp między liniami
  }
}

// Wyświetlenie szczegółów kryptowaluty
void displayCryptoDetails(String cryptoId) {
  String api_url = String(api_url_details_template) + cryptoId + "/market_chart?vs_currency=usd&days=7";
  String jsonData = fetchCryptoData(api_url.c_str());

  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED);
    tft.print("Błąd JSON");
    return;
  }

  // Wyświetlenie wykresu
  const JsonArray& prices = doc["prices"];
  float data[7];
  for (int i = 0; i < 7; i++) {
    data[i] = prices[i][1];
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Kryptowaluta: ");
  tft.println(cryptoId);

  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("Wykres 7 dni");

  drawLineGraph(data, 7, graphX, graphY, graphWidth, graphHeight);

  // Przycisk powrotu
  tft.fillRect(200, 280, 40, 30, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(210, 290);
  tft.print("Back");
}

// Funkcja rysująca wykres
void drawLineGraph(float data[], int dataCount, int x, int y, int width, int height) {
  float minValue = data[0], maxValue = data[0];

  for (int i = 1; i < dataCount; i++) {
    if (data[i] < minValue) minValue = data[i];
    if (data[i] > maxValue) maxValue = data[i];
  }

  tft.drawRect(x, y, width, height, TFT_WHITE);

  for (int i = 0; i < dataCount - 1; i++) {
    int x0 = x + (i * (width / (dataCount - 1)));
    int y0 = y + height - ((data[i] - minValue) / (maxValue - minValue) * height);
    int x1 = x + ((i + 1) * (width / (dataCount - 1)));
    int y1 = y + height - ((data[i + 1] - minValue) / (maxValue - minValue) * height);
    
    tft.drawLine(x0, y0, x1, y1, TFT_GREEN);
  }
}

// Obsługa dotyku
void handleTouch() {
  if (!ts.touched()) return;

  TS_Point p = ts.getPoint();
  int x = map(p.x, 0, 4095, 0, 240);
  int y = map(p.y, 0, 4095, 0, 320);

  if (currentView == 0) {
    if (y > 40 && y < 190) {
      selectedCrypto = y < 70 ? "bitcoin" : y < 100 ? "dogecoin" : y < 130 ? "pepe" :
                       y < 160 ? "cardano" : y < 190 ? "avalanche" : "mew";
      currentView = 1;
      displayCryptoDetails(selectedCrypto);
    }
  } else if (currentView == 1 && x > 200 && y > 280) {
    currentView = 0;
  }
}

// Setup
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
  tft.print("Ładowanie danych...");
}

// Główna pętla
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