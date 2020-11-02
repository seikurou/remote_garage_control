/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  You’ll need:
   - Blynk App (download from AppStore or Google Play)
   - ESP8266 board
   - Decide how to connect to Blynk
     (USB, Ethernet, Wi-Fi, Bluetooth, ...)

  There is a bunch of great example sketches included to show you how to get
  started. Think of them as LEGO bricks  and combine them as you wish.
  For example, take the Ethernet Shield sketch and combine it with the
  Servo example, or choose a USB sketch and add a code from SendData
  example.
 *************************************************************/

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
#define DHTTYPE DHT11

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "keys.h"

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char *auth = auth_secret;

// Your WiFi credentials.
// Set password to "" for open networks.
char *ssid = ssid_secret;
char *pass = pass_secret;

const static double MILLIS_TO_INCH = .0067515;

const int trigPin = 0;
const int echoPin = 4;
const int DHTPin = 5;

const static int VPIN_STATE = 1; // Blynk virtual pin denoting garage state
const static int VPIN_TIME = 2; // Blynk virtual pin denoting time since last cycle
const static int VPIN_TEMP = 3;
const static int VPIN_HUMID = 4;
const static int VPIN_STATE_RAW = 5;

BlynkTimer timer;
DHT dht(DHTPin, DHTTYPE);

boolean isGarageOpen = false;

int textNotifyTimerId = 0;
boolean notified = false;

unsigned long last_garage_cycle = 0; //the system time of the last cycle, in milliseconds

const static int MEASURE_INTERVAL = 1000; //how often to measure distance, in milliseconds

const static int OPEN_THRESHOLD = 16; // if sonar reading is less than OPEN_THRESHOLD inches, then garage is open

const static long THIRTY_MINUTES = 1800000; //milliseconds

void setup()
{
  digitalWrite(14, HIGH);
  // Debug console
  //  Serial.begin(9600);

  Blynk.begin(auth, ssid, pass);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);
  timer.setInterval(MEASURE_INTERVAL, updateUltrasonic);
  timer.setInterval(MEASURE_INTERVAL/2, updateDHT);
}


void loop()
{
  Blynk.run();
  timer.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
}

// GET READING
void updateUltrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long rawDist = pulseIn(echoPin, HIGH);
  rawDist = rawDist * MILLIS_TO_INCH;
  checkForNewState(rawDist < OPEN_THRESHOLD); 
}

// newState is true if garage is open
// checks if garage has changed state
void checkForNewState(bool newState) {
  if (isGarageOpen != newState) {
    isGarageOpen = newState;
    garageCycle();
  }
  sendGarageTime();
}

// called if the garage has transitioned into a different state
void garageCycle() {
  last_garage_cycle = millis();
  if (isGarageOpen) {
    Blynk.virtualWrite(VPIN_STATE, "Garage is open");
    Blynk.virtualWrite(VPIN_STATE_RAW, 1);
    textNotifyTimerId = timer.setTimeout(THIRTY_MINUTES, textNotifyOpen);
    notified = false;
  } else {
    Blynk.virtualWrite(VPIN_STATE, "Garage is closed");
    Blynk.virtualWrite(VPIN_STATE_RAW, 0);
    if (!notified) {
      timer.disable(textNotifyTimerId);
    }
  }  
}

// updates virtual pin notifying of last cycle.
void sendGarageTime() {
  String msg;
  if (isGarageOpen) {
    msg = "Opened ";
  } else {
    msg = "Closed ";
  }
  msg += formatMillis(millis() - last_garage_cycle) + " ago.";
  Blynk.virtualWrite(VPIN_TIME, msg); //time since last cycle
}

void textNotifyOpen() {
  Blynk.notify("The garage has been open for over 30 minutes!");
  notified = true;
}

void updateDHT() {
  float newT = dht.readTemperature();
  float newH = dht.readHumidity();
  if (!isnan(newT)) {
    Blynk.virtualWrite(VPIN_TEMP, newT);
  }
  if (!isnan(newH)) {
    Blynk.virtualWrite(VPIN_HUMID, newH);
  }
}

String formatMillis(unsigned long m) {
  m = m / 1000;
  int hours = m / 3600;
  m = m % 3600;
  int minutes = m / 60;
  int seconds = m % 60;
  return String(hours) + " hr, " + String(minutes) + " min, " + String(seconds) + " sec";
}
