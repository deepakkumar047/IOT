#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "DHT.h"
#include "MQ135.h"

#define ANALOGPIN A0

//#define DHTPIN 0     // what digital pin we're connected to (D3)
//#define DHTTYPE DHT11

MQ135 gasSensor = MQ135(ANALOGPIN);
//DHT dht(DHTPIN, DHTTYPE);
DHT dht;

char auth[] = "b706d4af4e7946f887fbc28ee038b2c0";
SimpleTimer timer;

const char *ssid =  "IOT_TESTING";
const char *pass =  "0123456789";

const uint16_t port = 4321;
const char *host = "192.168.43.64"; // local server's IP

WiFiClient client;

void sendSensor()
{
  //float h = dht.readHumidity();
  //float t = dht.readTemperature();
  float h = dht.getHumidity();
  float t = dht.getTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println("\n\nFailed to read from DHT sensor!");
    return;
  }

  Serial.println("");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %     ");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" °C ");
  Serial.print("    ");
  //float rzero = gasSensor.getRZero(); //this to get the rzero value, uncomment this to get ppm value
  float ppm = gasSensor.getPPM(); // this to get ppm value, uncomment this to get rzero value
  //Serial.println(rzero); // this to display the rzero value continuously, uncomment this to get ppm value
  Serial.print("Air Quality: ");
  Serial.print(ppm);
  Serial.print(" ppm");

  //Sending data to local server
  if(!client.connected())   //if client is not connected to local server
  {
      Serial.println("\n\nConnecting to local server..."); 
      //Serial.println(host);
      if(!client.connect(host, port))
      {
        Serial.println("\nConnection to local server failed!");
        //Serial.println("Wait for 2 sec...");
        //delay(1000);
      }
      else
        Serial.println("Connetion to local server is successful.");
  }
  else  //client is connected
  {
    char buff[16];
    memset(buff, 0, 16);
    dtostrf(h, 15, 2, buff);
    client.write(buff, 16);
  
    memset(buff, 0, 16);
    dtostrf(t, 15, 2, buff);
    client.write(buff, 16);
  
    memset(buff, 0, 16);
    dtostrf(ppm, 15, 2, buff);
    client.write(buff, 16);
  }
  //Send data to Blynk
  Blynk.virtualWrite(V5, h);  //V5 is for Humidity
  Blynk.virtualWrite(V6, t);  //V6 is for Temperature
  Blynk.virtualWrite(V7, ppm);  //V7 is for Air Quality
}

void setup() 
{
  Serial.begin(9600);
  delay(100);          
  Serial.print("\nConnecting to: ");
  Serial.println(ssid); 

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Connecting to Blynk server...");
  Blynk.begin(auth, ssid, pass);
  Serial.println("Connection to Blynk server is successful.");

  Serial.println("Connecting to local server..."); 
  //Serial.println(host);
  if(!client.connect(host, port))
  {
    Serial.println("Connection to local server failed!");
    //Serial.println("Wait for 2 sec...");
    //delay(1000);
  }
  else
    Serial.println("Connetion to loacl server is successful.");

  Serial.println("\nDHT11 & MQ135 Testing:");

  //dht.begin();
  dht.setup(D1);  //PIN D1 of NodeMCU
  timer.setInterval(2000L, sendSensor);
}
 
void loop() 
{
  Blynk.run(); // Initiates Blynk
  timer.run(); // Initiates SimpleTimer
}
