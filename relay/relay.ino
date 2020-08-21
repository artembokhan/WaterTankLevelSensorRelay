#include <LiquidCrystal.h>
#include <ESP8266WebServer.h>

const String  version = "AP-1.0";
const char*   ssid = "point-uz";
const char*   key = "nou8haiy";
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
unsigned long timerGeneral = 0,
              timerSensor = 0,
              timerStop = 0,
              timerStart = 0,
              sensorCounter = 0,
              onCounter = 0;

LiquidCrystal    lcd(12,13,4,0,2,14); 
ESP8266WebServer server(80);

void updateRelay() {
    // Stop Relay
    if ((distance < distanceStop ||                    // when tank becomes full
         millis() - timerSensor >= intervalIdle ||     // when no data from sensor
         millis() - timerStart > intervalMaxWorking || // when working too long
         distance == -1 ) &&                           // when sensor data is invalid
                             StatusRelay == "on") {    // when relay is on
            digitalWrite(relayPin, LOW);
            timerStop = millis();
            StatusRelay = "off";
    };

    // Start Relay
    if (distance > distanceStart &&                 // when tank becomes empty
        distance > 0 &&                             // when sensor data is valid
        StatusRelay == "off" &&                     // when relay is off
        millis() - timerSensor < intervalIdle &&    // when data from sensor is being recieved
        (millis() - timerStop  > intervalProtect || onCounter == 0 )) { // when time from last on was passed
            digitalWrite(relayPin, HIGH);
            timerStart = millis();
            StatusRelay = "on";
            onCounter++;
    };
}

void LCDStatus() {
    lcd.setCursor(0,0);
    lcd.print(StatusRelay             + " | " + StatusNetwork                                          + "                ");
    lcd.setCursor(0,1);
    lcd.print(String(distance) + "cm" + " | " + String(round((millis() - timerGeneral)/1000), 0) + "s" + "                ");
}

String IpAddress2String(const IPAddress& ipAddress) {
    return String(ipAddress[0]) + String(".") +\
           String(ipAddress[1]) + String(".") +\
           String(ipAddress[2]) + String(".") +\
           String(ipAddress[3]);
}

String WebStatus() {
    String msg, LastOn, LastOff;

    if (timerStart != 0) LastOn  = String((millis() - timerStart)/1000) + "s"; else LastOn  = "n/a";
    if (timerStop  != 0) LastOff = String((millis() - timerStop) /1000) + "s"; else LastOff = "n/a";
    
    msg += "Relay:               " + StatusRelay + "\n";
    msg += "Last Reply:          " + StatusNetwork + "\n";
    msg += "Distance:            " + String(distance)                       + "cm" + "\n";
    msg += "Sensor Conn Counter: " + String(sensorCounter)                         + "\n";
    msg += "Relay On Counter:    " + String(onCounter)                             + "\n";
    msg += "General Timer:       " + String((millis() - timerGeneral)/1000) + "s"  + "\n";
    msg += "Sensor Timer:        " + String((millis() - timerSensor) /1000) + "s"  + "\n";
    msg += "Last On Timer:       " + LastOn                                        + "\n";
    msg += "Last Off Timer:      " + LastOff                                       + "\n";
    msg += "Uptime:              " + String( millis()                /1000) + "s"  + "\n";
    msg += "\n";
    msg += "Start Distance:      " + String(distanceStart)                  + "cm" + "\n";
    msg += "Stop Distance:       " + String(distanceStop)                   + "cm" + "\n";
    msg += "Idle Interval:       " + String(intervalIdle/60/1000)           + "m"  + "\n";
    msg += "Protection Interval: " + String(intervalProtect/60/1000)        + "m"  + "\n";
    msg += "Max Interval:        " + String(intervalMaxWorking/60/1000)     + "m"  + "\n";
    return msg;
}

void updateGeneralTimer() { timerGeneral = millis(); }
void updateSensorTimer()  { timerSensor  = millis(); sensorCounter++; }

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
            if (distance < 0 || distance > distanceMax) { distance = -1; }
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
    WiFi.softAP(ssid, key);

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
//    delay(50);
}
