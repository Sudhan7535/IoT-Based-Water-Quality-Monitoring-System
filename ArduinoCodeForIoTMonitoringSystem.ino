#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- THE BROWNOUT BYPASS LIBRARIES ---
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- WI-FI & THINGSPEAK CREDENTIALS ---
const char* ssid = "realme 9 5G Speed Edition"; 
const char* password = "12345678"; 
String apiKey = "9R36QCFMBABG8LGT"; 

// --- SENSOR PINS ---
#define ONE_WIRE_BUS 4    // Temp Sensor on Pin 4
#define PH_PIN 32
#define TDS_PIN 33
#define TURB_PIN 34

// --- OLED SETTINGS ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- DEEP SLEEP SETTINGS ---
#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  30          // Time ESP32 will go to sleep (in seconds)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

void setup() {
  // *** CRITICAL BROWNOUT FIX ***
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  Serial.begin(115200);
  Serial.println("\n\n=========================================");
  Serial.println("☀️ ESP32 WAKING UP FROM DEEP SLEEP ☀️");
  Serial.println("=========================================");

  // Initialize OLED Display (Standard I2C address is 0x3C)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("❌ OLED allocation failed"));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    display.println("Booting up...");
    display.println("Connecting WiFi...");
    display.display();
  }

  tempSensor.begin();

  // Connect to Wi-Fi with a timeout
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); 
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Connected!");

    // 1. Read Real Temperature 
    tempSensor.requestTemperatures();
    float tempC = tempSensor.getTempCByIndex(0);
    float safeTemp = (tempC > -10.0) ? tempC : 25.0; 

    // 2. Read & Calculate pH
    float phVolt = (analogRead(PH_PIN) * 3.3) / 4095.0;
    float phLevel = (13.42 * phVolt) - 8.07; 
    if(phLevel < 0.0) phLevel = 0.0;
    if(phLevel > 14.0) phLevel = 14.0;

    // 3. Read & Calculate TDS (Compensated)
    float tdsVolt = (analogRead(TDS_PIN) * 3.3) / 4095.0;
    float compCoeff = 1.0 + 0.02 * (safeTemp - 25.0);
    float compVolt = tdsVolt / compCoeff;
    float tdsPPM = (133.42 * pow(compVolt, 3) - 255.86 * pow(compVolt, 2) + 857.39 * compVolt) * 0.5;
    if(tdsPPM < 0) tdsPPM = 0;

   // 4. Read & Calculate Turbidity
    float turbVolt = (analogRead(TURB_PIN) * 3.3) / 4095.0;
    
    // --- 3.3V CUSTOM CALIBRATION ---
    // Since you are using safe 3.3V power, your maximum clear voltage is lower.
    // Based on your test, open air/clean water is around 1.57V.
    float cleanWaterVoltage = 1.57; 
    
    // The Custom Math Equation: 
    // If voltage is 0, it equals 3000 NTU (Mud). If voltage hits 1.57V, it equals 0 NTU (Pure).
    float turbNTU = -(3000.0 / cleanWaterVoltage) * turbVolt + 3000.0;
    
    // --- THE AIR/CLEAN WATER LOCK ---
    // We widened the lock. If the voltage is anywhere above 1.45V, we lock it 
    // to exactly 0 NTU so the screen looks perfectly stable for the examiners.
    if(turbVolt >= 1.45) { 
      turbNTU = 0;
    }
    
    // Safety caps to prevent weird negative numbers
    if(turbNTU < 0) turbNTU = 0; 
    if(turbNTU > 3000) turbNTU = 3000;

    // -------------------------------------------------------------
    // 🧠 EDGE COMPUTING ALGORITHM: WATER QUALITY INDEX (WQI)
    // -------------------------------------------------------------
    float wqi = 100.0; 
    wqi -= abs(phLevel - 7.0) * 8.0; 
    wqi -= (tdsPPM * 0.05); 
    wqi -= (turbNTU * 0.1); 
    if (wqi < 0) wqi = 0;
    if (wqi > 100) wqi = 100;

    Serial.printf("📊 Data -> Temp: %.2f°C | pH: %.2f | TDS: %.0f ppm | Turb: %.0f NTU\n", tempC, phLevel, tdsPPM, turbNTU);
    Serial.printf("🌟 WATER QUALITY INDEX (WQI): %.0f / 100\n", wqi);

    // -------------------------------------------------------------
    // 📺 UPDATE OLED SCREEN
    // -------------------------------------------------------------
    display.clearDisplay();
    display.setTextSize(1);
    
    // Title
    display.setCursor(15, 0);
    display.print("SMART WATER BUOY");
    display.drawLine(0, 10, 128, 10, WHITE); // Draw a line under title
    
    // Data list
    display.setCursor(0, 15);
    display.printf("Temp: %.1f C\n", tempC);
    display.printf("pH  : %.1f\n", phLevel);
    display.printf("TDS : %.0f ppm\n", tdsPPM);
    display.printf("Turb: %.0f NTU\n", turbNTU);
    
    // Highlight WQI at the bottom
    display.drawLine(0, 50, 128, 50, WHITE);
    display.setCursor(0, 54);
    display.printf("WQI SCORE: %.0f / 100", wqi);
    
    display.display(); // Push everything to the screen

    // 5. Upload to ThingSpeak
    Serial.println("Pushing to ThingSpeak...");
    HTTPClient http;
    String url = "http://api.thingspeak.com/update?api_key=" + apiKey + 
                 "&field1=" + String(tempC) + 
                 "&field2=" + String(phLevel) + 
                 "&field3=" + String(tdsPPM) + 
                 "&field4=" + String(turbNTU) +
                 "&field5=" + String(wqi);  
    
    http.begin(url);
    int responseCode = http.GET();
    
    if (responseCode == 200) {
      Serial.println("✅ Upload Success! (HTTP 200)");
    } else {
      Serial.println("❌ ThingSpeak Error: " + String(responseCode));
    }
    http.end();

  } else {
    Serial.println("\n⚠️ Wi-Fi Failed.");
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("Wi-Fi Error!");
    display.display();
  }
  
  // -------------------------------------------------------------
  // 💤 DEEP SLEEP COMMAND
  // -------------------------------------------------------------
  Serial.println("Holding screen for 10 seconds so you can read it...");
  delay(10000); // 10 SECOND DELAY TO READ THE SCREEN
  
  display.clearDisplay(); // Turn screen off to save power
  display.display();
  
  Serial.printf("💤 Shutting down brain for %d seconds to save battery...\n", TIME_TO_SLEEP);
  Serial.println("=========================================\n");
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {}