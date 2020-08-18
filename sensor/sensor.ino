#include <ArduinoSort.h> // https://github.com/emilv/ArduinoSort
#include <LiquidCrystal.h>

#define triggerPin 1 // TX pin
#define echoPin 3 // RX pin
#define donePin 26
#define measurements 21

String version = "SENSOR-1.0";

int distance [measurements], median;

// Init display
LiquidCrystal lcd(12,13,17,16,27,14);

void setup() {
    // Init timer pin
    digitalWrite(donePin, LOW);
    pinMode(donePin, OUTPUT);

    // Init ultrasonic sensor pins
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // Init display
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Boot: " + version);
    delay(500);
}

void loop(){
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
        distance[k] = pulseIn(echoPin, HIGH) * 0.034 / 2;

        lcd.setCursor(11,0);
        lcd.print(String(k+1));

        lcd.setCursor(11,1);
        lcd.print(String(distance[k]) + "      ");
        delay(50);
    }

    sortArray(distance, measurements);

    median = distance[measurements/2];

    lcd.clear();
    lcd.print("Distance: " + String(median) + "cm");

    delay(2000);
    lcd.clear();
    
    lcd.clear();
    lcd.print("Turning off");

    delay(1000);

    digitalWrite(donePin, HIGH); // Send OFF to timer
}
