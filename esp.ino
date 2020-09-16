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


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "";
char pass[] = "";

const static double MILLIS_TO_INCH = .0067515;

const int trigPin = 0;
const int echoPin = 4;

boolean isGarageOpen = false;

unsigned long time_of_last_garage_cycle = 0;

//cycle through 1 rest, 2 clear trig, 3 write trig pin, and 4 wait for echo to measure sensor
unsigned long current_interval = 0; //used for all different states of ultrasonic sensor
unsigned long timer_start_time = 0;
const static int REST_TIME = 1000000; //how often to measure distance
boolean isResting = true; //ultrasonic sensor not active
const static int CLEAR_TIME = 2;
boolean isClearingTrigPin = false;
const static int WRITE_TIME = 10;
boolean isWritingTrigPin = false;
const static int TIMEOUT = 20000; //if ultrasonic sensor takes longer than 20 ms
//to return value assume distance is large enough for garage to be open
//boolean isAwaitingEcho = false; //commented because not using interrupts to measure ultrasonic


boolean text_sent = false; //ensure notification of garage open >30 min only sent once

//int time1 = 0; //start and
//int time2 = 0; //end time for measuring ultrasonic

//void ICACHE_RAM_ATTR echoFall ();
//void ICACHE_RAM_ATTR echoRise ();

void setup()
{
  digitalWrite(5, HIGH);
  // Debug console
  //  Serial.begin(9600);

  Blynk.begin(auth, ssid, pass);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);
  //  pinMode(echoPin, INPUT_PULLUP);
  //  attachInterrupt(digitalPinToInterrupt(echoPin), echoFall, FALLING);
  //  attachInterrupt(digitalPinToInterrupt(echoPin), echoRise, RISING);

  current_interval = REST_TIME; //schedule first ultrasonic sensor measurement
  timer_start_time = micros();
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);
}


void loop()
{
  updateUltrasonic();
  Blynk.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
}

void updateUltrasonic() {
  unsigned long t = micros() - timer_start_time;
  if (t >= current_interval) {
    if (isResting) {
      isClearingTrigPin = true;
      isResting = false;
      digitalWrite(trigPin, LOW);
      timer_start_time = micros();
      current_interval = CLEAR_TIME;
    }
    else if (isClearingTrigPin) {
      isClearingTrigPin = false;
      isWritingTrigPin = true;
      digitalWrite(trigPin, HIGH);
      timer_start_time = micros();
      current_interval = WRITE_TIME;

    }
    else if (isWritingTrigPin) {
      isWritingTrigPin = false;
      digitalWrite(trigPin, LOW);
      //      isAwaitingEcho = true;
      //      time_of_next_event = millis() + TIMEOUT;
      unsigned long measuredDist = pulseIn(echoPin, HIGH);
      measuredDist = measuredDist * MILLIS_TO_INCH;
      //      Serial.println(measuredDist);
      boolean state;
      String msg;
      if (measuredDist > 16) {
        state = false;
        msg = "Closed ";
      }
      else {
        state = true;
        msg = "Opened ";
      }
      if (state != isGarageOpen) { //garage changed state aka cycled
        time_of_last_garage_cycle = millis();
        isGarageOpen = state;
        Blynk.virtualWrite(1, isGarageOpen);
        if (isGarageOpen) {
          Blynk.virtualWrite(1, "Garage is open");
          Blynk.virtualWrite(3, 1);
        }
        else {
          Blynk.virtualWrite(1, "Garage is closed");
          Blynk.virtualWrite(3, 0);
          text_sent = false; //reset email notif
        }
      }
      msg += formatMillis(millis() - time_of_last_garage_cycle) + " ago";
      Blynk.virtualWrite(2, msg); //time since last cycle

      if (isGarageOpen && !text_sent && millis() - time_of_last_garage_cycle >= 1800000) {
        Blynk.notify("your garage has been open for over 30 minutes");
        text_sent = true;
      }

      //      isAwaitingEcho = false;
      isResting = true;
      timer_start_time = micros();
      current_interval = REST_TIME;
    }
    //    else if (isAwaitingEcho) { //reached only if timeout occurs
    //      Serial.println("timeout");
    //      echoFall();
    //    }
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
//void echoRise() {
//  Serial.println("echo rise");
//  time1 = millis();
//}
//void echoFall() {
//  time2 = millis();
//  int measuredDist = (time2-time1) * MILLIS_TO_INCH;
//  Serial.println(measuredDist);
//  boolean state;
//  if (measuredDist > 10) {
//    state = true;
//  }
//  else {
//    state = false;
//  }
//  if (state != isGarageOpen) {
//    time_of_last_garage_cycle = millis();
//  }
//  Blynk.virtualWrite(1, isGarageOpen);
//
//  isAwaitingEcho = false;
//  isResting = true;
//  time_of_next_event = millis() + REST_TIME;
//}
