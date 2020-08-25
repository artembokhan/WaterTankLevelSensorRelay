#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort
#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define triggerPin   1 // TX pin
#define echoPin      3 // RX pin
#define donePin      5
#define measurements 11

#define ipaddress IPAddress(192, 168, 5, 20)
#define gateway   IPAddress(192, 168, 5, 254)
#define netmask   IPAddress(255, 255, 255, 0)

/*
#define ipaddress IPAddress(192, 168, 4, 20)
#define gateway   IPAddress(192, 168, 4, 1)
#define netmask   IPAddress(255, 255, 255, 0)
*/
const String version    = "SENSOR-1.0";
const char*  ssid       = "extender-uz";
//const char*  ssid       = "point-uz";
const char*  key        = "nou8haiy";
const String relay_addr = "192.168.4.1";

int distance [measurements],
    median,
    channel = 2;

const char* wifi_codes[] = {
    "IDLE           ",
    "NO_SSID        ",
    "SCAN_COMPLETED ",
    "CONNECTED      ",
    "CONNECT_FAILED ",
    "CONNECTION_LOST",
    "DISCONNECTED   "
};

// Init display
//LiquidCrystal lcd(12,13,17,16,27,14); // espduino32
LiquidCrystal    lcd(12,13,4,0,2,14);   // wemos d1 r2

WiFiClient client;
HTTPClient http;

void setup() {
    // Init display
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Boot: " + version);

    // Init timer pin
    digitalWrite(donePin, LOW);
    pinMode(donePin, OUTPUT);

    // Init ultrasonic sensor pins
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);

    WiFi.mode(WIFI_OFF);
    WiFi.setPhyMode(WIFI_PHY_MODE_11B);
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.config(ipaddress, gateway, netmask);

    delay(300);
}

void loop(){
    WiFi.disconnect();
    WiFi.begin(ssid, key, channel);
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
        int status = WiFi.status();
        lcd.setCursor(0,1);
        lcd.print("WiFi: " + String(wifi_codes[status]));
        if (status == WL_CONNECTED) {
            lcd.setCursor(0,1);
            lcd.print("WiFi: done      ");
            delay(300);
            lcd.setCursor(0,1);
            lcd.print("WiFi: rssi " + String(WiFi.RSSI()));
            delay(300);
            break;
        }
        if (status != WL_DISCONNECTED) {
            lcd.setCursor(0,1);
            lcd.print("WiFi: " + String(wifi_codes[status]));
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
        http.begin(client, "http://" + relay_addr + "/sensor?distance=" + String(median));
        int httpCode = http.GET();
        lcd.setCursor(0,1);
        lcd.print("http: " + String(httpCode) + "          ");
        http.end();
    }

    delay(1000);
    
    lcd.setCursor(0,1);
    lcd.print("Turning off     ");

    delay(300);

    digitalWrite(donePin, HIGH); // Send OFF to timer

    delay(10000); // debug
}
