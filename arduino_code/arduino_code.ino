#include <Wire.h>            // I2C communication
#include <Adafruit_GFX.h>    // OLED graphics library
#include <Adafruit_SSD1306.h> // OLED driver
#include <DHT.h>             // DHT sensor library
#include <Servo.h>           // Servo motor library
#include <ESP8266WiFi.h>     // WiFi library
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT Sensor settings
#define DHTPIN 2              // GPIO2 (D4 on NodeMCU)
#define DHTTYPE DHT11         // Change to DHT22 if needed
DHT dht(DHTPIN, DHTTYPE);

// MQ2 Gas Sensor settings
#define MQ2_PIN A0            // Analog pin A0

// Servo Motor settings
#define SERVO_PIN 12          // GPIO12 (D6)
Servo myServo;               // Create Servo object

// WiFi credentials
const char WIFI_SSID[] = "Wan";
const char WIFI_PASSWORD[] = "kannan03";

// AWS IoT Core credentials
const char THINGNAME[] = "ESP32_DHT11";
const char MQTT_HOST[] = "a27wav9oqf0gk0-ats.iot.us-east-2.amazonaws.com";

// MQTT topics
const char AWS_IOT_PUBLISH_TOPIC[] = "esp32/pub";  
const char AWS_IOT_SUBSCRIBE_TOPIC[] = "esp32/sub";  

// AWS IoT Certificates (Replace with your own)
const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAJ+pGD4B5k02LRAxc5fm0AJxiLb8MA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yNTAzMjAwNTA2
MDZaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDJHwI0I57DN009Xu61
oAhfdc4TLuOnjt7Q7rCLSj0QUSScQqAAhO1p3DqKBt5ZFgjJzTtwDT+4YVVO6+0i
nlX/79mrg7uUUfmai47VkHGavvYiqBWZc1YUuzpcD/QXtehDGlPpO+eQx+/2tOSf
Po9RxvzztQNzymsxJAcrJ27hHpuATDX9nRPXfBVB5YRjLwMlP6clhTXwYTWstPtw
XhX8qkBR55pAGsuvk12WHkZNydEKYqVpK3Xv15djeKK45rXCgDvUXsObO+kKNek/
vsTsRwDrFvSx0vYvH+/5VmX/uy45RyR1u+JsNS184h0eMv8ANgcgVit4bQM2vwIh
zihpAgMBAAGjYDBeMB8GA1UdIwQYMBaAFB5OdfEv8bfnAbtWIsot7VCusNVwMB0G
A1UdDgQWBBTTFX/91M6INSMM+gSK2SRth+QoWzAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAL1Bc+oCaSEcn17PH+yv5p8Yi
Ig2K+lKYSQa6JaSoJjttxnRc0crbN6tvJhA2srDvUu5/dXr2Hiv+uk5xCeKm9Yl/
cJBgzO2qzJlOIXMgjuamllwaFvRPrXC41BPwejBo1m8B93EEbwE6tAW0kmSPSbFV
ak9u0yFYVgz7VMXaGH/ORhFSDpzcHYyxFpzyZVXvQtt4VdbWzMxc1YB3RGBUjNeJ
5ERoClv7GbWu17BE7xUDKvAnMgXL7uxO5dSAX0if8udUykkZ0s7tybMDSlr5pF/n
Le9U+KRHooscsuZrW2fv6RfumnyguMJ4/KVWDsDKpqy9ILCXRsirr4xG2hjfJQ==
-----END CERTIFICATE-----
)EOF";

const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAyR8CNCOewzdNPV7utaAIX3XOEy7jp47e0O6wi0o9EFEknEKg
AITtadw6igbeWRYIyc07cA0/uGFVTuvtIp5V/+/Zq4O7lFH5mouO1ZBxmr72IqgV
mXNWFLs6XA/0F7XoQxpT6TvnkMfv9rTknz6PUcb887UDc8prMSQHKydu4R6bgEw1
/Z0T13wVQeWEYy8DJT+nJYU18GE1rLT7cF4V/KpAUeeaQBrLr5Ndlh5GTcnRCmKl
aSt179eXY3iiuOa1woA71F7DmzvpCjXpP77E7EcA6xb0sdL2Lx/v+VZl/7suOUck
dbvibDUtfOIdHjL/ADYHIFYreG0DNr8CIc4oaQIDAQABAoIBADgk3uGdO/m1G2vR
b4PmS9ASO9fhFA43dP4QU9pefBNtA3JXqwDaGsidgP39Et79prBnCXurRyl6im0A
sW8jklBjLXmSya+K/snkwZfevcf8HQvprjXrG5Gn/o+qqHJiBpeM5vXKEke5eMR/
FP9bYHjsurpxwqf1Tt0l+JZUaMCQ4xPZiYGQzbTxBpyIBs/4YXH92cO5T5d4fn/x
iegsLxpMX9e2KzpQRkUzTjfUKY7ROPE/FoXY1O5Ha1RVOBsouaKTG51aXZL64hkY
0Ur/+GnvwgixscE+piKlIwqIKFd1yZrb4Sjw8S1THg7HIM7Z9rHAAGG0HnNwtYXz
eH8/540CgYEA523RcYRrxyqLHDO+OonyCEidQY8iReSZ3aJWZh8kq9SCu2qGT5Yi
MSalCQYr6+dlhqq7nB94pw3v7t57txVDav3oCV3OvGsYsqwZZ56sphJcDHbudpyN
VdHUYbhgZ+u2sC+8f1HwbGr+o4jYQcr//+PG/zpyTLjQehxtklMMH7cCgYEA3nlu
QLqXTcxt1nUBnWLenro4wP63aIq4lPIJCGYdSPyoZEkCIHf3rAT0uB6u0Wjp3rvW
VeLKLeuuWU8mTihrNh/NxfSuS7TW+l3Kayi+AOE4PPHRc081/bT4hrdaHQkr1uWo
MqNzVQF5i6Xbi8on7zuvIWWuIp7ElW9v3XIluN8CgYEAz1XJoGO8k1CAFKIeuR9A
4T788/EiCXAGkVORCCi6l97pyLJk5gRhWZt/m5gV3WEPyhhHwFVJqJCJ9n0IZTv9
jGsxaMLMJm3kUJ5ynCzCDH7CTMB4X3deABbeqTWf2TqKD0qVG5PzS46H10cQR9Fw
jHYqoBpisG2bCOtTFxaj8/0CgYBTFy1LdaUfBHQdfdPdkViuFcUup7Oekvj/gCpJ
fcIASYUm93GY6NWS5ML1pVgLlFCMxMD3kN2MKxMR/hLDbCsmqtgy0ADs+5yzI6Z/
QVc9guB7OqHMnFAA8r9DqWU5pPw/pcSdLRjdLdDUjFykKBukKTlukshn2752RYmV
PpXCJwKBgEln7r985QNKIW2+e2SX3rRsAvAMWZ7oSVjZJeVmn0Cj3nTfQbY0ZSG+
Zasgg5qnvSmTJwkeoEwlPwxRw9hR6RtCg650GKzBOAh3Y0OUR//GgHiBowZroJwk
0W6hyaaYqVXTfHvWUNbv1WBn2ZXxB8MiBZYpI7k9lm4j9ym7d6bI
-----END RSA PRIVATE KEY-----
)EOF";

// WiFiClientSecure object
WiFiClientSecure net;

// X.509 certificate for AWS IoT
BearSSL::X509List cert(AWS_CERT_CA);
BearSSL::X509List client_crt(AWS_CERT_CRT);
BearSSL::PrivateKey key(AWS_CERT_PRIVATE);

// MQTT client
PubSubClient client(net);

// Publishing interval
const long interval = 5000;
unsigned long lastMillis = 0;

// Function to connect to AWS IoT Core
void connectAWS() {
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi connected!");

    // Set AWS IoT Core certificates
    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

    // Connect to AWS IoT Core MQTT broker
    client.setServer(MQTT_HOST, 8883);
    client.setCallback(messageReceived);

    Serial.println("Connecting to AWS IoT...");
    while (!client.connect(THINGNAME)) {
        Serial.print(".");
        delay(1000);
    }

    if (!client.connected()) {
        Serial.println("AWS IoT Timeout!");
        return;
    }

    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    Serial.println("\nAWS IoT Connected!");
}

// Callback function for MQTT messages
void messageReceived(char *topic, byte *payload, unsigned int length) {
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// Function to publish sensor data to AWS IoT
void publishMessage() {
    // Read sensor values
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int gasValue = analogRead(MQ2_PIN);

    // Display values on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(5, 5);
    display.print("Temp: ");
    display.print(temperature);
    display.println(" C");

    display.setCursor(5, 20);
    display.print("Hum:  ");
    display.print(humidity);
    display.println(" %");

    display.setCursor(5, 35);
    display.print("Gas: ");
    display.print(gasValue);
    display.display();

    // Print values to Serial Monitor
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%  Temperature: ");
    Serial.print(temperature);
    Serial.print("°C  Gas Level: ");
    Serial.println(gasValue);

    // Control Servo Motor based on Gas Level
    if (gasValue > 10) {
        Serial.println("Gas Detected! Moving Servo...");
        myServo.write(180);
    } else {
        myServo.write(0);
    }

    // Create JSON document for AWS IoT
    StaticJsonDocument<200> doc;
    doc["humidity"] = humidity;
    doc["temperature"] = temperature;
    doc["gas_concentration"] = gasValue;

    // Serialize JSON to a buffer
    char jsonBuffer[200];
    serializeJson(doc, jsonBuffer);

    // Publish message to AWS IoT
    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing Sensors, Servo & AWS IoT...");

    // Initialize DHT sensor
    dht.begin();

    // Initialize Servo
    myServo.attach(SERVO_PIN);
    myServo.write(0);

    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed!");
        for (;;);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.println("DHT & MQ2 Ready!");
    display.display();
    delay(2000);

    // Connect to AWS IoT
    connectAWS();
}

void loop() {
    if (millis() - lastMillis > interval) {
        lastMillis = millis();
        if (client.connected()) {
            publishMessage();
        }
    }
    client.loop();
}
