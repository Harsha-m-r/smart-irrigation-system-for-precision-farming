#include "arduino_secrets.h"
/*
  Sketch generated by the Arduino IoT Cloud Thing "Untitled"
  https://create.arduino.cc/cloud/things/0449e276-1c63-4fae-a27f-03fbde0ca262 

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  float humidity;
  float temp;
  int node1;
  int node2;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/

#include "thingProperties.h"
#include "DHT.h"
#include <U8g2lib.h> // Include U8G2 library

#define DHTPIN 4          // Pin where the DHT11 is connected (GPIO4)
#define DHTTYPE DHT11     // Define DHT type

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
HardwareSerial mySerial(1); // Use UART1

// Relay pins for Node 1 and Node 2
#define RELAY1_PIN 5  // GPIO5 for Node 1 relay
#define RELAY2_PIN 18 // GPIO18 for Node 2 relay

// Relay state variables
bool relay1State = false;
bool relay2State = false;

// Initialize OLED display using U8G2_SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);  
  mySerial.begin(115200, SERIAL_8N1, 16, 17); // RX -> GPIO16, TX -> GPIO17

  // Set relay pins as outputs
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Make sure relays are initially turned off
  digitalWrite(RELAY1_PIN, HIGH); // Assume HIGH turns off the relay
  digitalWrite(RELAY2_PIN, HIGH);

  // Initialize the DHT11 sensor
  dht.begin();

  // Initialize the OLED display
  u8g2.begin();

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  ArduinoCloud.update();
  
  // Read temperature and humidity from DHT11
  float temperature = dht.readTemperature();
  float humidityValue = dht.readHumidity();

  // Check if DHT11 reading is valid
  if (isnan(temperature) || isnan(humidityValue)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Update cloud variables
  ::humidity = humidityValue;
  ::temp = temperature;

  // Read from serial input
  if (mySerial.available()) {
    String receivedData = mySerial.readStringUntil('\n');
    Serial.println("Received from ESP8266: " + receivedData);

    // Parse the received data
    int nodeId = 0;
    int soilMoisture = 0;
    int soilMoisturePercentage = 0;

    // Extract values from receivedData string
    sscanf(receivedData.c_str(), "Node ID: %d - Soil Moisture: %d - Percentage: %d", &nodeId, &soilMoisture, &soilMoisturePercentage);

    // Assign node values and update relays based on first-come-first-serve logic
    if (nodeId == 1) {
      ::node1 = soilMoisturePercentage; // Update cloud variable node1
      
      // Relay control for Node 1
      if (soilMoisturePercentage < 40 && !relay1State && !relay2State) {
        digitalWrite(RELAY1_PIN, LOW); // Turn ON relay 1
        relay1State = true;
        Serial.println("Relay 1 turned ON for Node 1");
      } else if (soilMoisturePercentage >= 40 && relay1State) {
        digitalWrite(RELAY1_PIN, HIGH); // Turn OFF relay 1
        relay1State = false;
        Serial.println("Relay 1 turned OFF for Node 1");
      }
      
    } else if (nodeId == 2) {
      ::node2 = soilMoisturePercentage; // Update cloud variable node2
      
      // Relay control for Node 2
      if (soilMoisturePercentage < 40 && !relay1State && !relay2State) {
        digitalWrite(RELAY2_PIN, LOW); // Turn ON relay 2
        relay2State = true;
        Serial.println("Relay 2 turned ON for Node 2");
      } else if (soilMoisturePercentage >= 40 && relay2State) {
        digitalWrite(RELAY2_PIN, HIGH); // Turn OFF relay 2
        relay2State = false;
        Serial.println("Relay 2 turned OFF for Node 2");
      }
    }
  }

  // Update the OLED display
  displayStatus(temperature, humidityValue, node1, node2, relay1State, relay2State);

  delay(2000); // Wait 2 seconds before the next reading
}

// Function to update the OLED display
void displayStatus(float temperature, float humidity, int node1Moisture, int node2Moisture, bool relay1, bool relay2) {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.drawLine(64, 1, 64, 63);
  u8g2.drawLine(0, 10, 126, 10);
  
  // Display farm labels
  u8g2.setFont(u8g2_font_t0_11_tr);
  u8g2.drawStr(68, 9, "Farm:   2");
  u8g2.drawStr(2, 9, "Farm:   1");
  
  // Temperature display
  u8g2.setFont(u8g2_font_profont10_tr);
  if (temperature >= 20 && temperature <= 35) {
    u8g2.drawStr(2, 19, "temp: OK");
    u8g2.drawStr(66, 19, "temp: OK");
  } else {
    u8g2.drawStr(2, 19, "temp: NOT OK");
    u8g2.drawStr(66, 19, "temp: NOT OK");
  }

  // Humidity display
  if (humidity >= 55 && humidity <= 90) {
    u8g2.drawStr(3, 32, "hmd: OK");
    u8g2.drawStr(68, 32, "hmd: OK");
  } else {
    u8g2.drawStr(3, 32, "hmd: NOT OK");
    u8g2.drawStr(68, 32, "hmd: NOT OK");
  }

  // Moisture for Node 1
  if (node1Moisture >= 40) {
    u8g2.drawStr(3, 44, "moist1: OK");
  } else {
    u8g2.drawStr(3, 44, "moist1: NOT OK");
  }
  
  // Moisture for Node 2
  if (node2Moisture >= 40) {
    u8g2.drawStr(68, 44, "moist2: OK");
  } else {
    u8g2.drawStr(68, 44, "moist2: NOT OK");
  }

  // Relay status
  u8g2.drawStr(3, 55, relay1 ? "mtsat1: ON" : "mtsat1: OFF");
  u8g2.drawStr(68, 55, relay2 ? "mtsat2: ON" : "mtsat2: OFF");

  // Send buffer to display
  u8g2.sendBuffer();
}
