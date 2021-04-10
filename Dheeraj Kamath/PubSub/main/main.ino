/* Author: Dheeraj D Kamath
 * Upload date: 10 April 2021
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"        // including the library of DHT11 temperature and humidity sensor
#include <time.h>


// WIFI Configurations
const char* ssid = "dk1";
const char* password = "12345678";

#define DHT_TYPE         DHT11
#define LIGHT_SENSOR_PIN D1
#define MOISTURE_PIN     A0
#define TRIG_PIN         D3
#define ECHO_PIN         D5
#define DHT_DATA_PIN     D6
#define MOTOR_CTRL_PIN   D7

DHT dht(DHT_DATA_PIN, DHT_TYPE);

WiFiClientSecure espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

/* Global variables */
long lastMsg = 0;
long lastDHTUpdate = 0;
long lastSoilMoistureUpdate = 0;
char msg[50];
float humidity = 0;
float temperature = 0;
long distance = 0;
int lightStatus = 0;
float waterLevelPercentage = 0;
int value = 0;
const char* AWS_endpoint = "a2wez1ijy9ju6g-ats.iot.us-east-1.amazonaws.com"; //MQTT broker ip
const char* IOT_THING_NAME = "AgroThing";
const char* IOT_TOPIC = "GardenStatus";
int moisture = 0;
int pumpSwitch = 0;
bool mqttUpdate = false;

time_t t;
struct tm *ptm;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "rZPxOBhLnvlN-qHI6pgsOuOy5_kPELu3";

void callback(char* topic, byte* payload, unsigned int length);

PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set MQTT port number to 8883 as per //standard

BlynkTimer timer;
BlynkTimer mqttUpdateTimer;

// This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.
void BlynkAppUpdateEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V0, moisture);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, waterLevelPercentage);
}

void MqttUpdateEvent()
{
  mqttUpdate = true;
}

void callback(char* topic, byte* payload, unsigned int length) {
    
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}


void setup_wifi() {

    delay(10);

    // We start by connecting to a WiFi network
    espClient.setBufferSizes(512, 512);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    timeClient.begin();
    
    while(!timeClient.update()){
        timeClient.forceUpdate();
    }

    espClient.setX509Time(timeClient.getEpochTime());

}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(IOT_THING_NAME)) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            snprintf (msg, 75, "{\"message\": \"Garden is online!\"}");
            client.publish(IOT_TOPIC, msg);
            // ... and resubscribe
            client.subscribe("inTopic");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");

            char buf[256];
            espClient.getLastSSLError(buf,256);
            Serial.print("WiFiClientSecure SSL error: ");
            Serial.println(buf);

            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void sensors_init(){
    
    dht.begin();

    pinMode(LIGHT_SENSOR_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

void light_read(){
    
    lightStatus = digitalRead(LIGHT_SENSOR_PIN);
}

void water_level_read(){
    
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    distance = pulseIn(ECHO_PIN, HIGH);
    distance = (distance/2) / 29.1;

    waterLevelPercentage = (distance * 100) / 30;

    if(distance > 500){
        distance = 500;
        waterLevelPercentage = 0;
        Serial.println("Out of Range: 500 is max");
    }

    if(distance < 2){
        distance = 0;
        waterLevelPercentage = 0;
        Serial.println("Out of Range: 2 is min");
    }

    if(distance > 30){
        waterLevelPercentage = 100;
    }

    waterLevelPercentage = 100 - waterLevelPercentage;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    Serial.print("Water level percentage: ");
    Serial.print(waterLevelPercentage);
    Serial.println(" %");
}

void dht_read(){
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();         
    Serial.print("Current humidity = ");
    Serial.print(humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(temperature); 
    Serial.println("C  ");
}

void soil_moisture_read(){

    moisture = analogRead(MOISTURE_PIN);
    moisture = moisture/10;
  
    /* Set minimum */
    if(moisture < 0){
        moisture = 0;
    }

    /* Invert logic */
    if(moisture > 128){
        moisture = 128;
    }else{
        moisture = 128 - moisture;
    }

    Serial.print("Soil moisture level: ");
    Serial.println(moisture);
}

// This function will be called every time Slider Widget
// in Blynk app writes values to the Virtual Pin V1
BLYNK_WRITE(V4)
{
  pumpSwitch = param.asInt(); // assigning incoming value from pin V1 to a variable

    if(pumpSwitch){
        digitalWrite(MOTOR_CTRL_PIN, 1);
    }else{
        digitalWrite(MOTOR_CTRL_PIN, 0);
    }
}


void setup() {

    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // Initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    setup_wifi();
    delay(1000);

    lastMsg = 0;

    t = time(NULL);
    ptm = localtime(&t);

    ptm->tm_year = 2021;
    ptm->tm_mon = 04;
    ptm->tm_mday = 10;
    ptm->tm_hour = 20;
    ptm->tm_min = 25;
    ptm->tm_sec = 0;

    /* Motor control configure */
    pinMode(MOTOR_CTRL_PIN, OUTPUT);

    sensors_init();

    Blynk.begin(auth, ssid, password);

    // Setup a function to be called every second
    timer.setInterval(1000L, BlynkAppUpdateEvent);
    mqttUpdateTimer.setInterval(5000L, MqttUpdateEvent);

    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
    }

    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

    // Load certificate file
    File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
    
    if (!cert) {
        Serial.println("Failed to open cert file");
    }
    else
        Serial.println("Success to open cert file");

    delay(1000);

    if (espClient.loadCertificate(cert))
        Serial.println("cert loaded");
    else
        Serial.println("cert not loaded");

    // Load private key file
    File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
    
    if (!private_key) {
        Serial.println("Failed to open private cert file");
    }
    else
        Serial.println("Success to open private cert file");

    delay(1000);

    if (espClient.loadPrivateKey(private_key))
        Serial.println("private key loaded");
    else
        Serial.println("private key not loaded");

    // Load CA file
    File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
    
    if (!ca) {
        Serial.println("Failed to open ca ");
    }
    else
        Serial.println("Success to open ca");

    delay(1000);

    if(espClient.loadCACert(ca))
        Serial.println("ca loaded");
    else
        Serial.println("ca failed");

    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

    dht_read();
    water_level_read();
    light_read();
}

void loop() {

    if (!client.connected()) {
        reconnect();
    }
    
    client.loop();

    dht_read();
    water_level_read();
    light_read();
    soil_moisture_read();

    long now = millis();

    if((now - lastMsg) > 1000){
        lastMsg = now;
        Serial.print("Entered dht: ");
        Serial.println(lastDHTUpdate);
        dht_read();
        water_level_read();
        light_read();
        lastSoilMoistureUpdate = now;
        soil_moisture_read();
        Serial.print("Entered moisture: ");
        Serial.println(lastSoilMoistureUpdate);
    }

    if(mqttUpdate){
        mqttUpdate = false;
        snprintf (msg, 150, "{\"data\": {\"timestamp\": %ld, \"temperature\": %d, \"humidity\": %d, \"light\": %d, \"moisture\": %d, \"water_level\": %d}}", 
        now, (int)temperature, (int)humidity, lightStatus, moisture, (int)waterLevelPercentage);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish(IOT_TOPIC, msg);
        Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
    }

    if((moisture < 25) || (pumpSwitch == 1)){
        digitalWrite(MOTOR_CTRL_PIN, 1);
    }else if((moisture > 25) && (pumpSwitch == 0)){
        digitalWrite(MOTOR_CTRL_PIN, 0);
    }

    Blynk.run();
    timer.run(); // Initiates BlynkTimer
    mqttUpdateTimer.run();
    
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(100); // wait for a second
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    delay(100); // wait for a second
}
