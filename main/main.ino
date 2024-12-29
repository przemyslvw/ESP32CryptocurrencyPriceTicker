#include <TFT_eSPI.h>         // Biblioteka wyświetlacza TFT, wymaga konfiguracji w User_Setup.h
#include <XPT2046_Touchscreen.h> // Obsługa dotykowego ekranu
#include <WiFi.h>             // Do połączenia z Wi-Fi
#include <HTTPClient.h>       // Do obsługi zapytań HTTP
#include <ArduinoJson.h>      // Do przetwarzania danych JSON

// Import Wi-Fi credentials z pliku "WiFiConfig.h"
// Plik "WiFiConfig.h" musi zawierać:
// const char* ssid = "Twoja_Siec";
// const char* password = "Twoje_Haslo";
#include "WiFiConfig.h"

// API CoinGecko do pobierania danych o kryptowalutach
const char* api_url_main = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,dogecoin,pepe,cardano,avalanche,mew&vs_currencies=usd&include_24hr_change=true";
const char* api_url_details_template = "https://api.coingecko.com/api/v3/coins/"; // Podstawa URL do szczegółowych danych

// Inicjalizacja wyświetlacza TFT i ekranu dotykowego
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(33); // CS pin dla dotyku ustawiony na GPIO 33

// Aktualny widok (0 = tabela, 1 = szczegóły)
int currentView = 0;
String selectedCrypto = ""; // ID wybranej kryptowaluty

// Konfiguracja wykresu
const int graphWidth = 220;   // Szerokość wykresu
const int graphHeight = 150;  // Wysokość wykresu
const int graphX = 10;        // Położenie X wykresu
const int graphY = 60;        // Położenie Y wykresu

// Funkcja inicjalizująca połączenie Wi-Fi
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
    http.begin(url); // Rozpoczynamy zapytanie HTTP do podanego URL
    int httpResponseCode = http.GET(); // Pobieramy odpowiedź

    if (httpResponseCode > 0) { // Jeśli odpowiedź jest poprawna
      String response = http.getString();
      http.end(); // Zwalniamy zasoby HTTP
      return response;
    } else {
      Serial.println("Błąd podczas pobierania danych z API");
      http.end(); // Zwalniamy zasoby HTTP w przypadku błędu
      return ""; // W przypadku błędu zwracamy pusty string
    }
  }
  return ""; // Jeśli Wi-Fi jest rozłączone, zwracamy pusty string
}

// Funkcja rysująca wykres liniowy
void drawLineGraph(float data[], int dataCount, int x, int y, int width, int height) {
  float minValue = data[0], maxValue = data[0];

  // Znalezienie minimalnej i maksymalnej wartości w danych
  for (int i = 1; i < dataCount; i++) {
    if (data[i] < minValue) minValue = data[i];
    if (data[i] > maxValue) maxValue = data[i];
  }

  // Rysowanie ramki wykresu
  tft.drawRect(x, y, width, height, TFT_WHITE);

  // Rysowanie linii wykresu
  for (int i = 0; i < dataCount - 1; i++) {
    int x0 = x + (i * (width / (dataCount - 1)));
    int y0 = y + height - ((data[i] - minValue) / (maxValue - minValue) * height);
    int x1 = x + ((i + 1) * (width / (dataCount - 1)));
    int y1 = y + height - ((data[i + 1] - minValue) / (maxValue - minValue) * height);
    
    tft.drawLine(x0, y0, x1, y1, TFT_GREEN); // Zielona linia dla danych
  }
}

// Wyświetlanie szczegółów kryptowaluty
void displayCryptoDetails(String cryptoId) {
  String api_url = String(api_url_details_template) + cryptoId + "/market_chart?vs_currency=usd&days=7";
  String jsonData = fetchCryptoData(api_url.c_str());

  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) { // Jeśli wystąpił błąd parsowania JSON
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED);
    tft.print("Błąd JSON");
    return;
  }

  // Parsowanie danych historycznych
  const JsonArray& prices = doc["prices"];
  float data[7];
  for (int i = 0; i < 7; i++) {
    data[i] = prices[i][1]; // Druga kolumna zawiera cenę
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

  drawLineGraph(data, 7, graphX, graphY, graphWidth, graphHeight);

  // Dodanie przycisku powrotu
  tft.fillRect(200, 280, 40, 30, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(210, 290);
  tft.print("Back");
}

// Obsługa dotyku
void handleTouch() {
  if (!ts.touched()) return; // Jeśli ekran nie został dotknięty, nic nie robimy

  TS_Point p = ts.getPoint();
  int x = map(p.x, 0, 4095, 0, 240);
  int y = map(p.y, 0, 4095, 0, 320);

  if (currentView == 0) {
    // Kliknięcie w tabeli kryptowalut
    if (y > 10 && y < 40) selectedCrypto = "bitcoin";
    else if (y > 40 && y < 70) selectedCrypto = "dogecoin";
    else if (y > 70 && y < 100) selectedCrypto = "pepe";
    else if (y > 100 && y < 130) selectedCrypto = "cardano";
    else if (y > 130 && y < 160) selectedCrypto = "avalanche";
    else if (y > 160 && y < 190) selectedCrypto = "mew";

    if (selectedCrypto != "") {
      currentView = 1; // Przełączamy widok na szczegóły
      displayCryptoDetails(selectedCrypto);
    }
  } else if (currentView == 1 && x > 200 && y > 280) {
    currentView = 0; // Powrót do tabeli kryptowalut
  }
}

// Funkcja wyświetlająca tabelę kryptowalut
void displayCryptoTable(String jsonData) {
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.print("Blad danych");
    return;
  }

  // Parsowanie i wyświetlanie danych z JSON
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.print("Kryptowaluty:");

  int y = 40;
  for (String crypto : {"bitcoin", "dogecoin", "pepe", "cardano", "avalanche", "mew"}) {
    float price = doc[crypto]["usd"];
    float change = doc[crypto]["usd_24h_change"];

    tft.setCursor(10, y);
    tft.printf("%s: %.2f USD (%.2f%%)", crypto.c_str(), price, change);
    y += 30;
  }
}

// Funkcja inicjalizacyjna
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
