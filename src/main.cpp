#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MCP23017.h>
#include <WiFi.h>
#include <AsyncAPDS9306.h> // luminosity library
#include <AsyncSI7021.h>   // humidity library

// Defining the I2C Address of the MCP
#define MCP23017_ADDR 0x20
//#define Band 915E6 ////433E6 for Asia, 866E6 for Europe, 915E6 for North America
// Define an instance of the MCP23017 class
MCP23017 mcp = MCP23017(MCP23017_ADDR);

void portMode(MCP23017Port port, uint8_t directions, uint8_t pullups = 0xFF, uint8_t inverted = 0x00);
void writeRegister(MCP23017Register reg, uint8_t value);

AsyncSI7021 sensorHumidity;
AsyncAPDS9306 sensor;
const APDS9306_ALS_GAIN_t again = APDS9306_ALS_GAIN_1;
const APDS9306_ALS_MEAS_RES_t atime = APDS9306_ALS_MEAS_RES_16BIT_25MS;

// Replace with your network credentials
String apiKey = ""; // Enter your Write API key from ThingSpeak
const char *ssid = "";       // replace with your wifi ssid and wpa2 key
const char *password = "";
const char *server = "184.106.153.149"; //"api.thingspeak.com";
WiFiClient client;

void setup()
{
  // Join I2C bus - more on this in later chapter 8)
  Wire.begin();
  // Initialize MCP
  mcp.init();
  // Port A as Output (LEDS) || Port B as Input (Switches & Buttons)
  mcp.portMode(MCP23017Port::A, 0);          // 0 = Pin is configured as an output.
  mcp.portMode(MCP23017Port::B, 0b11111111); // 1 = Pin is configured as an input.
  // Reset Port A & B
  mcp.writeRegister(MCP23017Register::GPIO_A, 0x00); //Reset port A
  mcp.writeRegister(MCP23017Register::GPIO_B, 0x00); //Reset port B

  Serial.begin(9600);
  if (sensor.begin(again, atime))
  {
    Serial.print("\n\nSensor found!\n");
  }
  else
  {
    Serial.print("\n Sensor not found!\n");
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(2000);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  // ------------------------------ Luminosity ------------------------------
  unsigned long startTime;
  unsigned long duration;

  Serial.println("Starting Luminosity Measurement...");
  startTime = millis();

  float lux = 0;
  float total = 0;
  for (int i = 0; i < 15; i++)
  {
    AsyncAPDS9306Data data = sensor.syncLuminosityMeasurement();
    lux = data.calculateLux();
    total += lux;
    Serial.print("Lux data point ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(lux, 2);
    Serial.println("lux");
    delay(1000);
  }

  float averageLux = total / 15;

  duration = millis() - startTime;

  Serial.print("Luminosity: ");
  Serial.print(averageLux, 2);
  Serial.println("lux");
  Serial.println("---------------------");
  // ------------------------------ /Luminosity ------------------------------

  // ------------------------------ Humidity ------------------------------
  Serial.println("Starting Humidity Measurement...");
  while (!sensorHumidity.begin())
  {
  } // If sensorHumidity measurement is not ready, continue

  duration = millis() - startTime; // Measurement time

  sensorHumidity.startHumidityMeasurement(); // Get sensorHumidity measurement
  bool isSi7021Measuring = true;

  float humidity;
  do
  {
    if (isSi7021Measuring && sensorHumidity.isMeasurementReady())
    {
      humidity = sensorHumidity.readTemperatureFromHumidityMeasurement();
      isSi7021Measuring = false;
    }
  } while (isSi7021Measuring);

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("% (Relative Humidity)");
  Serial.println("----------------------------------------------------");
  delay(2000);
  // ------------------------------ /Humidity ------------------------------

  if (client.connect(server, 80)) // "184.106.153.149" or api.thingspeak.com
  {
    String postStr = apiKey;
    postStr += "&field7=";
    postStr += String(averageLux);
    postStr += "&field8=";
    postStr += String(humidity);
    postStr += "\r\n\r\n\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  delay(15000);
}
