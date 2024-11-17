#include <TFT_eSPI.h>         // Biblioteka wyświetlacza TFT
#include <WiFi.h>             // Do połączenia z Wi-Fi
#include <HTTPClient.h>       // Do obsługi API HTTP
#include <ArduinoJson.h>      // Do przetwarzania danych JSON

// Wi-Fi credentials (uzupełnij)
const char* ssid = "Twoja_Nazwa_Sieci";
const char* password = "Twoje_Haslo";

// API settings
const char* api_url = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,dogecoin,pepe,cardano,avalanche,mew&vs_currencies=usd&include_24hr_change=true";

// TFT display
TFT_eSPI tft = TFT_eSPI();

// Kolory dla wzrostów i spadków
#define GREEN tft.color565(0, 255, 0)
#define RED   tft.color565(255, 0, 0)
#define WHITE tft.color565(255, 255, 255)

// Wymiary ekranu
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

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
String fetchCryptoData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(api_url);
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

// Wyświetlanie tabeli kryptowalut
void displayCryptoTable(String jsonData) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    Serial.println("Błąd JSON");
    return;
  }

  // Wyświetlenie tabeli
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);

  const char* cryptos[] = {"bitcoin", "dogecoin", "pepe", "cardano", "avalanche", "mew"};
  int y = 10;

  for (int i = 0; i < 6; i++) {
    String name = cryptos[i];
    float price = doc[name]["usd"];
    float change = doc[name]["usd_24h_change"];

    // Wyświetlanie nazwy
    tft.setCursor(10, y);
    tft.print(name);

    // Wyświetlanie ceny
    tft.setCursor(100, y);
    tft.print("$");
    tft.print(price);

    // Wyświetlanie zmiany procentowej
    tft.setCursor(180, y);
    if (change >= 0) {
      tft.setTextColor(GREEN);
    } else {
      tft.setTextColor(RED);
    }
    tft.printf("%.2f%%", change);
    tft.setTextColor(WHITE);

    y += 30;
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);

  connectToWiFi();

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Ladowanie danych...");
}

void loop() {
  String data = fetchCryptoData();
  if (data != "") {
    displayCryptoTable(data);
  }
  delay(60000); // Aktualizacja co minutę
}
