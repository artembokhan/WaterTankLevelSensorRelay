#include <LiquidCrystal.h>
#include <ESP8266WebServer.h>

const String  version = "AP-1.0";
const char*   ssid = "point-uz";
const char*   key = "nou8haiy";
const int     channel = 2;
const int     relayPin = 5;
const int     distanceStart = 100, // Distance to turn on relay [cm] 
              distanceStop  = 30,  // Distance to turn off relay [cm]
              distanceMax = 400;   // Max appropriate sensor distance
const
unsigned long intervalIdle    =     1 * 60 * 60 * 1000, // 1 minutes: max working time without sensor update [ms]
              intervalProtect =     1 * 60 * 60 * 1000, // 1 hour:    protection time of relay [ms]
              intervalMaxWorking =  4 * 60 * 60 * 1000; // 4 hours:   max working time

int           distance=-1;
String        StatusRelay = "off",
              StatusNetwork = "n/a";
uint64_t      timerGeneral = 0,
              timerSensor = 0,
              timerStop = 0,
              timerStart = 0;
unsigned long sensorCounter = 0,
              onCounter = 0;

IPAddress addr_local(192,168,4,1);
IPAddress addr_gw(192,168,4,1);
IPAddress addr_netmask(255,255,255,0);

const char* wifi_codes[] = {
    "IDLE",
    "NO_SSID",
    "SCAN_COMPLETED",
    "CONNECTED",
    "CONNECT_FAILED",
    "CONNECTION_LOST",
    "DISCONNECTED"
};

LiquidCrystal    lcd(12,13,4,0,2,14); 
ESP8266WebServer server(80);

uint64_t millis64() {
    static uint32_t low32, high32;
    uint32_t new_low32 = millis();
    if (new_low32 < low32) high32++;
    low32 = new_low32;
    return (uint64_t) high32 << 32 | low32;
}

String uint64ToString(uint64_t input) {
    String result = "";
    uint8_t base = 10;

    do {
        char c = input % base;
        input /= base;

        if (c < 10)
            c +='0';
        else
            c += 'A' - 10;
        result = c + result;
    } while (input);

    return result;
}

void updateRelay() {
    // Stop Relay
    if ((distance < distanceStop ||                    // when tank becomes full
         millis64() - timerSensor >= intervalIdle ||     // when no data from sensor
         millis64() - timerStart > intervalMaxWorking || // when working too long
         distance == -1 ) &&                           // when sensor data is invalid
                             StatusRelay == "on") {    // when relay is on
            digitalWrite(relayPin, LOW);
            timerStop = millis64();
            StatusRelay = "off";
    };

    // Start Relay
    if (distance > distanceStart &&                 // when tank becomes empty
        distance > 0 &&                             // when sensor data is valid
        StatusRelay == "off" &&                     // when relay is off
        millis64() - timerSensor < intervalIdle &&    // when data from sensor is being recieved
        (millis64() - timerStop  > intervalProtect || onCounter == 0 )) { // when time from last on was passed
            digitalWrite(relayPin, HIGH);
            timerStart = millis64();
            StatusRelay = "on";
            onCounter++;
    };
}

void LCDStatus() {
    lcd.setCursor(0,0);
    lcd.print(StatusRelay             + " | " + StatusNetwork                                          + "                ");
    lcd.setCursor(0,1);
    lcd.print(String(distance) + "cm" + " | " + uint64ToString((millis64() - timerGeneral)/1000) + "s" + "                ");
}

String IpAddress2String(const IPAddress& ipAddress) {
    return String(ipAddress[0]) + String(".") +\
           String(ipAddress[1]) + String(".") +\
           String(ipAddress[2]) + String(".") +\
           String(ipAddress[3]);
}

String WebStatus() {
    String msg, LastOn, LastOff;

    if (timerStart != 0) LastOn  = uint64ToString((millis64() - timerStart)/1000) + "s"; else LastOn  = "n/a";
    if (timerStop  != 0) LastOff = uint64ToString((millis64() - timerStop) /1000) + "s"; else LastOff = "n/a";
    
    msg += "Relay:               " + StatusRelay + "\n";
    msg += "Last Reply:          " + StatusNetwork + "\n";
    msg += "Distance:            " + String(distance)                       + "cm" + "\n";
    msg += "Sensor Conn Counter: " + String(sensorCounter)                         + "\n";
    msg += "Relay On Counter:    " + String(onCounter)                             + "\n";
    msg += "General Timer:       " + uint64ToString((millis64() - timerGeneral)/1000) + "s"  + "\n";
    msg += "Sensor Timer:        " + uint64ToString((millis64() - timerSensor) /1000) + "s"  + "\n";
    msg += "Last On Timer:       " + LastOn                                        + "\n";
    msg += "Last Off Timer:      " + LastOff                                       + "\n";
    msg += "Uptime:              " + uint64ToString( millis64()                /1000) + "s"  + "\n";
    msg += "\n";
    msg += "Start Distance:      " + String(distanceStart)                  + "cm" + "\n";
    msg += "Stop Distance:       " + String(distanceStop)                   + "cm" + "\n";
    msg += "Idle Interval:       " + String(intervalIdle/60/1000)           + "m"  + "\n";
    msg += "Protection Interval: " + String(intervalProtect/60/1000)        + "m"  + "\n";
    msg += "Max Interval:        " + String(intervalMaxWorking/60/1000)     + "m"  + "\n";
    return msg;
}

void updateGeneralTimer() { timerGeneral = millis64(); }
void updateSensorTimer()  { timerSensor  = millis64(); sensorCounter++; }

void handle_Status() {
    server.send(200, "text/plain", WebStatus()); 
    StatusNetwork = "status/200";
    updateGeneralTimer();
}

void handle_Sensor() {
    boolean has_distnace = 0;
    for (uint8_t i = 0; i < server.args(); i++) {
        if (server.argName(i) == "distance") {
            distance = server.arg(i).toInt();
            if (distance <= 0 || distance > distanceMax) { distance = -1; }
            has_distnace = 1;
            server.send(200, "text/plain", "UPDATED");
            StatusNetwork = "sensor/200";
            updateGeneralTimer();
            updateSensorTimer();
        }
    }
    if (has_distnace == 0) {
        server.send(400, "text/plain", "No distance in request");
        StatusNetwork = "sensor/400";
        updateGeneralTimer();
    }
}

void handle_NotFound(){
    server.send(404, "text/plain", "Not found");
    StatusNetwork = "notfnd/404";
    updateGeneralTimer();
}

void setup() {
    // Init display
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("boot: " + version);
    delay(1000);

    // Init relay pin
    digitalWrite(relayPin, LOW);
    pinMode(relayPin, OUTPUT);

    // Init WiFi Access Point
    lcd.clear();
    lcd.print("init wi-fi");
    delay(1000);
    
    lcd.clear();
    lcd.print("ap: " + String(ssid));
    lcd.setCursor(0,1);
    lcd.print("key: " + String(key));
    delay(5000);

    lcd.clear();
    lcd.print("ap: " + String(ssid));

    // Start Access Point
    WiFi.mode(WIFI_OFF);
    WiFi.setPhyMode(WIFI_PHY_MODE_11B);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(addr_local, addr_gw, addr_netmask);
    WiFi.softAP(ssid, key, channel);

    IPAddress ipaddress = WiFi.softAPIP();
    lcd.setCursor(0,1);
    lcd.print("ip: " + IpAddress2String(ipaddress));

    server.on("/", handle_Status);
    server.on("/sensor", handle_Sensor);
    server.onNotFound(handle_NotFound);
    
    // Start webserver
    server.begin();

    delay(1000);
}

void loop() {
    LCDStatus();
    server.handleClient();
    updateRelay();
}
