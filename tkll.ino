#include <HttpClient.h>
#include <b64.h>

#define BLYNK_TEMPLATE_ID "TMPLELEo1tsp"
#define BLYNK_DEVICE_NAME "ESP8266"
#define BLYNK_AUTH_TOKEN "P2RvqjUYNfcd5EnuBM8gd9AtYZeTqSw0"
#define BLYNK_PRINT Serial
#define LED D8

#include <Adafruit_Sensor.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <BlynkSimpleEsp8266.h>
#include "DHTesp.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// #include <WiFiClientSecure.h>
// #include "timecontrol.h" //for further code refinement

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Thang Bom";
char pass[] = "chabietgi";

//define ...
BlynkTimer timer;
DHTesp dht;

//URL for API call
// const char* host="random-data-api.com";
// const char* url="/api/address/random_address";
const char* host="api.openweathermap.org";
const char* url="/data/3.0/onecall?lat=10.88076941140911&lon=106.80537669859868&exclude=minutely,hourly,daily,alerts&units=metric&appid=2cc9395542e23ef301caa84436b0f7da";


//define constant for time fetching
const long utcOffsetInSeconds = 7*60*60; //7 as in for UTC+7
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; //days within a week


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);



// constant
// int LED = D8; 
int sensorpin=A0;
bool water = false;
int sensorData;
int output;
int var ;
bool eventTrigger = false;
bool eventTriggerTemp = false;
double humidity;
double temperature;
double moisture;
bool current_Weather=false; //currently not raining

// User's input
double soilMoisture = 100;
double maxSoilMoisture = 100;
double minTemp = 0;
double maxTemp = 100;

// Read value from blynk app
BLYNK_WRITE(V4)
{
  maxSoilMoisture = param.asDouble();
//  Serial.print("maxSoilMoisture");
//  Serial.println(maxSoilMoisture);
}
BLYNK_WRITE(V5)
{
  minTemp = param.asDouble();
  Serial.println(minTemp);
}
BLYNK_WRITE(V6)
{
  maxTemp = param.asDouble();
  Serial.println(maxTemp);
}
BLYNK_WRITE(V7)
{
  soilMoisture = param.asDouble();
  Serial.println(soilMoisture);
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent()
{
  Blynk.virtualWrite(V8, millis() / 1000);

  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
//  Serial.print("humidity ");
//  Serial.println(humidity);
//  Serial.print("temperature ");
//  Serial.println(temperature); 
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  
  sensorData = analogRead(sensorpin);
  moisture = map(sensorData,0,1023,100,0);
//  Serial.print("moisture ");
//  Serial.println(moisture);
  Blynk.virtualWrite(V3,moisture);
}
//Deserialization
bool deserialization_json(String json){ //extract information from the string containing data from API calls
  // String input;

  StaticJsonDocument<768> doc;

  DeserializationError error = deserializeJson(doc, json); //deserializing the string

  if (error) { //error handling
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.f_str());
  return false;
  }

  float lat = doc["lat"]; // 33.44
  float lon = doc["lon"]; // -94.04
  const char* timezone = doc["timezone"]; // "America/Chicago"
  int timezone_offset = doc["timezone_offset"]; // -21600

  JsonObject current = doc["current"];
  long current_dt = current["dt"]; // 1668661886
  long current_sunrise = current["sunrise"]; // 1668602922
  long current_sunset = current["sunset"]; // 1668640408
  float current_temp = current["temp"]; // 3.92
  float current_feels_like = current["feels_like"]; // 1.56
  int current_pressure = current["pressure"]; //y 1031
  int current_humidity = current["humidity"]; // 67
  float current_dew_point = current["dew_point"]; // -1.45
  int current_uvi = current["uvi"]; // 0
  int current_clouds = current["clouds"]; // 0
  int current_visibility = current["visibility"]; // 10000
  float current_wind_speed = current["wind_speed"]; // 2.57
  int current_wind_deg = current["wind_deg"]; // 330

  JsonObject current_weather_0 = current["weather"][0];
  int current_weather_0_id = current_weather_0["id"]; // 800
  const char* current_weather_0_main = current_weather_0["main"]; // "Clear" 
  // Serial.println(current_weather_0_main[0]); 
  const char* current_weather_0_description = current_weather_0["description"]; // "clear sky"
  const char* current_weather_0_icon = current_weather_0["icon"]; // "01n"
  if(strcmp(current_weather_0_main,"Rain")==0){ //char* comparision
    return true;
  }
  return false;
}

//HTTP GET
void httpGETRequest(){
  String json="Blank!"; //string contains json 
  WiFiClient client; //define a wifi client
  const int httpPort=80; //port 80 is for HTTP
  if(!client.connect(host,httpPort)){ //error handling
    Serial.println("Connection Failed!");
    return;        
  }
  // Serial.print("Requesting URL: ");
  // Serial.println(url); // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n"); //specify HTTP GET, which path to fetch data from the host, host name and closing connection
  unsigned long timeout = millis();
  while (client.available() == 0) { //timeout handling if connection takes too long
    if (millis() - timeout > 10000){
      Serial.println(">>> Client Timeout !");
      client.stop(); return; 
    } 
  }
  while (client.available()){     //we only need the last line of the response
    json = client.readStringUntil('\r'); 
  } 
  // String json=
  // "{\"lat\":10.8808,\"lon\":106.8054,\"timezone\":\"Asia/Ho_Chi_Minh\",\"timezone_offset\":25200,\"current\":{\"dt\":1668727603,\"sunrise\":1668725382,\"sunset\":1668767177,\"temp\":25.88,\"feels_like\":26.98,\"pressure\":1009,\"humidity\":94,\"dew_point\":24.84,\"uvi\":0,\"clouds\":20,\"visibility\":4900,\"wind_speed\":1.54,\"wind_deg\":0,\"weather\":[{\"id\":701,\"main\":\"Mist\",\"description\":\"mist\",\"icon\":\"50d\"}]}}";
  // Serial.print(json);  
  current_Weather= deserialization_json(json); //if current_Weather==1 it's raining
}
// check if watering time is suitable
bool suitableTime(){
  // Serial.print("Current time is: ");  
  // Serial.print(timeClient.getHours());  
  if(timeClient.getHours()<=8 || timeClient.getHours()>=16){ //earlier than 9 or later than 4
    // Serial.println(" TIME OK!");
    return true;
  }
  // Serial.println(" TIME NOT OK!");
  return false;
}
void Water(){
  // Serial.print("Current Weather:");
  // Serial.println (current_Weather);
  if(suitableTime() && !current_Weather){
    if(moisture < soilMoisture && water == false){
      digitalWrite(LED,1);
    }
    else if(moisture >= soilMoisture){
      digitalWrite(LED,0);
      water = false;
    }
  } 
}
void sendNotify(){
  if(moisture > maxSoilMoisture && eventTrigger == false){
    Serial.println("max mois");
    eventTrigger = true;
    Blynk.logEvent("maxmoisture","Soild Moisture is higher max");
  }
  else if(moisture <= maxSoilMoisture){
    eventTrigger = false;
  }
  if(temperature< minTemp && eventTriggerTemp==false){
    eventTriggerTemp = true;
    Serial.println("min temp");
    Blynk.logEvent("mintemp","Temperatur is lower min");
  }
  else if(temperature > maxTemp && eventTriggerTemp == false){
    eventTriggerTemp = true;
    Serial.println("max temp");
    Blynk.logEvent("maxtemp","Temperatur is higher max");
  }
  else if(temperature<= maxTemp && temperature>=minTemp){
    eventTriggerTemp = false;
  }
}
void loop()
{
  Blynk.run();
  timer.run();

  timeClient.update();
  

  // Serial.print(daysOfTheWeek[timeClient.getDay()]);
  // Serial.print(", ");
  // Serial.print(timeClient.getHours());
  // Serial.print(":");
  // Serial.print(timeClient.getMinutes());
  // Serial.print(":");
  // Serial.println(timeClient.getSeconds());
  
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
}
void setup()
{
  // Debug console
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  timeClient.begin();
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);
  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);
  timer.setInterval(1000L, Water);
  timer.setInterval(1000L, sendNotify);
  timer.setInterval(20000L, httpGETRequest);
  pinMode(LED,OUTPUT); 
  dht.setup(5, DHTesp::DHT11);
}
