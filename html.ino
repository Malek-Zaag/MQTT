#include"SPIFFS.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Wire.h>
#include <PubSubClient.h>


const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "password";
const char* PARAM_INPUT_3 = "tmax";


AsyncWebServer server(80);

#define LM75_REG_TEMP (0x00) // Temperature Register
#define LM75_REG_CONF (0x01) // Configuration Register
#define LM75_ADDR     (0x48) // LM75 address
//I2C0 PINS
#define sda 21
#define scl 22
TwoWire Mysensor_I2C = TwoWire(0);



const char* ssid = "AndroidAp";
const char* password="01234567";

const char* mqttServer = "farmer.cloudmqtt.com";
//ACCOUNT PPP.GL.RT


//ACCOUNT EMIR
const int mqttPort = 15281;
const char* mqttUser = "insatmqtt";
const char* mqttPassword = "insat2021";


const int ledPin = 12;

// Set LED GPIO
#define LED_Blue 12
#define LED_Green 13
#define LED_Red 16

float Tmax=20;


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    ssid: <input type="text" name="ssid">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    password: <input type="text" name="password">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    maxTemp: <input type="text" name="tmax">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
} 








WiFiClient espClient;
PubSubClient client(espClient);




void callback(char* topic, byte* payload, unsigned int length) {
  
 char newtopic;
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);


  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    newtopic=(char)payload[i];
  }

  if(newtopic=='G')
  {
  digitalWrite(LED_Blue, HIGH);   
  digitalWrite(LED_Green, LOW);  
  digitalWrite(LED_Red, HIGH);   
  }
  else if (newtopic=='R'){
  digitalWrite(LED_Blue, HIGH);   
  digitalWrite(LED_Green, HIGH);  
  digitalWrite(LED_Red, LOW);   
  }
  else if (newtopic='B')
  { digitalWrite(LED_Blue, LOW);   
  digitalWrite(LED_Green, HIGH);  
  digitalWrite(LED_Red, HIGH);
    }
else{
   digitalWrite(LED_Blue, LOW);   
  digitalWrite(LED_Green, LOW);  
  digitalWrite(LED_Red, LOW);
  }


   Serial.println("");
    
  
 
  
}



  void write1Byte(uint8_t RegAddr, uint8_t Value){
    
         Mysensor_I2C.beginTransmission(LM75_ADDR);
         Mysensor_I2C.write(RegAddr);
         Mysensor_I2C.write(Value);
          if (Mysensor_I2C.endTransmission(true)!=0){
            Serial.println ("problem writing to I2C device");
            exit(0);
          }
   }
  
    uint16_t read2Bytes(uint8_t RegAddr){
          Mysensor_I2C.beginTransmission(LM75_ADDR);
          Mysensor_I2C.write(RegAddr);
          if (Mysensor_I2C.endTransmission(false)){ //if !=0
                Serial.println ("Problem writing without stop"); 
                exit(0);
            }
          Mysensor_I2C.requestFrom(LM75_ADDR,0x02);
          uint8_t resh = (Mysensor_I2C.read());
          uint8_t resl = (Mysensor_I2C.read());
           
          uint16_t resultat = (resh<<8)+resl;
          return resultat;}
      
    float calculTemp(){

          //Read temperature conversion result
          int16_t tempout = (int16_t)read2Bytes(0x00);
          tempout = tempout>>5;

          Serial.print("LM75 TEMP Conversion value  :");
          Serial.println(tempout);

          float Temperature = tempout*0.125;
          Serial.print("temperature  is :");
          Serial.println(Temperature);
          
          Serial.println("----------------------------");
          return Temperature;
}


void setup() {
     
  // Initialize serial port
  Serial.begin(115200);

  
  Mysensor_I2C.begin(sda, scl, 400000);
  pinMode(ledPin, OUTPUT);
// initialize the 3 digital pins (LEDS RGB).
  pinMode(LED_Blue, OUTPUT);
  pinMode(LED_Green, OUTPUT);
  pinMode(LED_Red, OUTPUT);

  //THE Three LEDS OFF. (For the RGB Leds: HIGH=OFF, LOW=ON)
  digitalWrite(LED_Blue, HIGH);   
  digitalWrite(LED_Green, HIGH);  
  digitalWrite(LED_Red, HIGH);   
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED)
  {delay(500);}

  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());




  


   // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });




  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      ssid= inputMessage.c_str()  ;
    }

    

    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
     password=inputMessage.c_str();
    }


    
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      maxTemp=inputMessage.toFloat();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }


    
     Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
  

  
delay(100000);



WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED)
  {Serial.print("NO CONNECTION");}


  
  
  Serial.println(Tmax);
  Serial.print(ssid);

   client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

   while (!client.connected()) {
  
  if (client.connect("MyEspIot", mqttUser, mqttPassword )) {
      //"WifiEspOnOff"
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
}}
if(client.subscribe("iotdevice/service/led"))
{
  Serial.print("subscribed_to_topic");
      client.publish("temp/salle255","");}

}

void loop() {


  client.loop();
   float temp = calculTemp();
  String sTemp = String(temp,3);
  char chTemp[6];
  sTemp.toCharArray(chTemp,6);
  Serial.println(temp);
 if(temp>Tmax)
  {client.publish("temp/salle255",chTemp);}
  
  
  delay(1000);
}
  // put your main code here, to run repeatedly:
