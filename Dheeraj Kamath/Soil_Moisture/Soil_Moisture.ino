#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

int WET= 16; // Wet Indicator at Digital pin D0
int DRY= 2;  // Dry Indicator at Digital pin D4

int sense_Pin = 0; // sensor input at Analog pin A0
int moisture = 0;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "rZPxOBhLnvlN-qHI6pgsOuOy5_kPELu3";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "dk1";
char pass[] = "12345678";

BlynkTimer timer;

// This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  moisture = analogRead(sense_Pin);
  moisture = moisture/10;
  
  if(moisture < 0){
    moisture = 0;
  }

  if(moisture > 128){
    moisture = 128;
  }else{
    moisture = 128 - moisture;
  }
  
  Blynk.virtualWrite(V0, moisture);
}

int value = 0;

void setup() {
   Serial.begin(9600);
   pinMode(WET, OUTPUT);
   pinMode(DRY, OUTPUT);
   Blynk.begin(auth, ssid, pass);

  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {
   Blynk.run();
   timer.run(); // Initiates BlynkTimer
}
