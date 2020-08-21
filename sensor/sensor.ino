#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort
#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define triggerPin 1 // TX pin
#define echoPin 3 // RX pin
#define donePin 5
#define measurements 11

const String version = "SENSOR-1.0";
const char*  ssid = "point-uz";
const char*  key = "nou8haiy";

// static data to connect wifi quickly
#define ipaddress IPAddress(192, 168, 4, 200)
#define gateway   IPAddress(192, 168, 4,   1)
#define mask      IPAddress(255, 255, 255, 0)
uint8_t mac[6] = { 0xCE, 0x50, 0xE3, 0x69, 0x67, 0x5b }; // router's mac-address 
int channel = 2; // wifi channel

int distance [measurements], median;

// Init display
//LiquidCrystal lcd(12,13,17,16,27,14); // espduino32
LiquidCrystal    lcd(12,13,4,0,2,14);   // wemos d1 r2

void setup() {
    // Init display
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Boot: " + version);

    WiFi.config(ipaddress, gateway, mask);

    // Init timer pin
    digitalWrite(donePin, LOW);
    pinMode(donePin, OUTPUT);

    // Init ultrasonic sensor pins
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);

    delay(300);
}

void loop(){
    WiFi.persistent(false);
    WiFi.enableInsecureWEP();
    WiFi.begin(ssid, key, channel, mac, true);
    lcd.clear();

    // Measure distance
    lcd.print("Measuring:");
    
    lcd.setCursor(0,1);
    lcd.print("Distance:");

    for (int k = 0; k < measurements; k++) {
        digitalWrite(triggerPin, LOW);
        delayMicroseconds(5);

        // send signal
        digitalWrite(triggerPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(triggerPin, LOW);

        // recieve echo
        distance[k] = pulseIn(echoPin, HIGH, 100000) * 0.034 / 2;

        lcd.setCursor(11,0);
        lcd.print(String(k+1));

        lcd.setCursor(11,1);
        lcd.print(String(distance[k]) + "      ");
        delay(10);
    }

    sortArray(distance, measurements);

    median = distance[(measurements + 1) / 2 - 1];

    lcd.clear();
    lcd.print("Distance: " + String(median) + "cm");

    for (int i=0; i<=500; i++) {
        lcd.setCursor(0,1);
        lcd.print("WiFi: " + String(WiFi.status()));
        if (WiFi.status() == WL_CONNECTED) {
            lcd.setCursor(0,1);
            lcd.print("WiFi: done      ");
            delay(300);
            lcd.setCursor(0,1);
            lcd.print("WiFi: rssi " + String(WiFi.RSSI()));
            delay(300);
            break;
        }
        delay(50);
    }

    if (WiFi.status() != WL_CONNECTED) {
        lcd.setCursor(0,1);
        lcd.print("WiFi: failed     ");
    }

    else {
        WiFiClient client;
        HTTPClient http;
 
        http.begin(client, "http://" + WiFi.gatewayIP().toString() + "/sensor?distance=" + String(median));

        int httpCode = http.GET();

        lcd.setCursor(0,1);
        lcd.print("http: " + String(httpCode) + "          ");
 
        http.end();
    }
    
    WiFi.disconnect();
    delay(1000);
    
    lcd.setCursor(0,1);
    lcd.print("Turning off     ");

    delay(300);

    digitalWrite(donePin, HIGH); // Send OFF to timer

    delay(10000); // debug
}
