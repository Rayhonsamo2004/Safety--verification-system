#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <time.h>  // Time library for getting real-time clock

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1      
#define SCREEN_ADDRESS 0x3C  
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 2             
#define DHTTYPE DHT11        
DHT dht(DHTPIN, DHTTYPE);

#define MQ2_PIN A0           
#define SERVO_PIN 12         
Servo myServo;              

#define WIFI_SSID "Wan"
#define WIFI_PASSWORD "kannan03"

#define API_KEY "AIzaSyBeOlk-9-FExGl-ZroEvGkn3AqfbkBrENo"
#define FIREBASE_PROJECT_ID "safetyverification-c8fe7"
#define USER_EMAIL "rayhonsam10@gmail.com"
#define USER_PASSWORD "samo@2004"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastUploadTime = 0; 
const unsigned long uploadInterval = 60000; // Upload every 60 seconds (1 minute)

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing Sensors, Wi-Fi & Firebase...");

    dht.begin();
    myServo.attach(SERVO_PIN);
    myServo.write(0);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed!");
        for (;;);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.println("Connecting to Wi-Fi...");
    display.display();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    display.clearDisplay();
    display.setCursor(10, 10);
    display.println("Wi-Fi Connected!");
    display.display();
    delay(2000);

    configTime(0, 0, "pool.ntp.org"); // Set time from NTP server
}

void loop() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(); 
    int gasValue = analogRead(MQ2_PIN);

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%  Temperature: ");
    Serial.print(temperature);
    Serial.print("°C  Gas Level: ");
    Serial.println(gasValue);

    if (gasValue > 10) {  
        Serial.println("Gas Detected! Moving Servo...");
        myServo.write(180);
    } else {
        myServo.write(0);
    }

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

    if (millis() - lastUploadTime >= uploadInterval) {
        uploadDataToFirestore(temperature, humidity, gasValue);
        lastUploadTime = millis(); 
    }
}

String getFormattedTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return "00-00-00-00-00-00";
    }
    char timeString[20];
    sprintf(timeString, "%02d-%02d-%02d-%02d-%02d-%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year % 100, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(timeString);
}

void uploadDataToFirestore(float temperature, float humidity, int gasValue) {
    FirebaseJson content;
    String timestamp = getFormattedTime();
    content.set("fields/Temperature/stringValue", String(temperature, 2));
    content.set("fields/Humidity/stringValue", String(humidity, 2));
    content.set("fields/GasLevel/stringValue", String(gasValue));
    content.set("fields/Timestamp/stringValue", timestamp);

    String documentPath = "EspData/SensorReadings_" + timestamp;

    Serial.print("Uploading to Firebase... ");

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
        Serial.printf("Success!\n%s\n", fbdo.payload().c_str());
    } else {
        Serial.println(fbdo.errorReason());
    }
}
