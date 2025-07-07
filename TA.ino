#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// INA219 config
Adafruit_INA219 ina219;

// WiFi dan Server
const char* ssid = "LAPTOP IJAL";   
const char* password = "12345678";  
const char* serverName = "http://192.168.137.1/data_esp32/save.php";

// Penghitung data
int dataCount = 0;
const int targetData = 3600;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // OLED Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal");
    while (1);
  }
  display.clearDisplay();

  // INA219 Init
  if (!ina219.begin()) {
    Serial.println("INA219 tidak terdeteksi");
    while (1);
  }

  // WiFi Connect
  WiFi.begin(ssid, password);
  Serial.print("Menyambungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung");
  displayMessage("WiFi Connected!");
  delay(2000);
}

void loop() {
  // Baca data sensor
  float busVoltage = ina219.getBusVoltage_V();
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float loadVoltage = busVoltage + (shuntVoltage / 1000.0);
  float current_mA = ina219.getCurrent_mA();
  float power_mW = busVoltage * current_mA;

  // Kirim data jika WiFi aktif
  if (WiFi.status() == WL_CONNECTED) {
    sendData(busVoltage, shuntVoltage, loadVoltage, current_mA, power_mW);
  } else {
    Serial.println("WiFi putus, data tidak dikirim");
  }

  // Tampilkan OLED
  displayDataOnOLED(dataCount, targetData, busVoltage, shuntVoltage, loadVoltage, current_mA, power_mW);
  delay(6000);
}

void sendData(float busVoltage, float shuntVoltage, float loadVoltage, float current_mA, float power_mW) {
  HTTPClient http;
  http.setTimeout(3000); // Batas waktu tunggu respons
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "bus_voltage=" + String(busVoltage) +
                    "&shunt_voltage=" + String(shuntVoltage) +
                    "&load_voltage=" + String(loadVoltage) +
                    "&current_mA=" + String(current_mA) +
                    "&power_mW=" + String(power_mW);

  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.println("Data berhasil dikirim");
    dataCount++;
  } else {
    Serial.print("Gagal kirim data, code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void displayMessage(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(message);
  display.display();
}

void displayDataOnOLED(int count, int target, float busVoltage, float shuntVoltage, float loadVoltage, float current_mA, float power_mW) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Data sensor
  display.setCursor(0, 20);
  display.print("BV: "); display.print(busVoltage, 2); display.println("V");
  display.setCursor(0, 0);
  display.print("SV: "); display.print(shuntVoltage, 1); display.println("mV");
  display.setCursor(0, 30);
  display.print("LV: "); display.print(loadVoltage, 2); display.println("V");
  display.setCursor(0, 40);
  display.print("C: "); display.print(current_mA, 1); display.println("mA");
  display.setCursor(0, 50);
  display.print("P: "); display.print(power_mW, 1); display.println("mW");

  // Status
  display.setCursor(70, 20);
  display.print("WiFi:");
  display.setCursor(70, 30);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("Online");
  } else {
    display.print("Offline");
  }

  display.setCursor(70, 40);
  display.print("Sent:");
  display.setCursor(70, 50);
  display.print(count);
  display.print("/");
  display.print(target);

  display.display();
}
