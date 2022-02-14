  #include <Wire.h>
  #include <WiFi.h>
  #include <PubSubClient.h>
  
  #define sda 21
  #define scl 22
  
  #define LED_Blue 12
  #define LED_Green 13
  #define LED_Red 16
  
  #define MMA_ADDRESS        0x4C
  #define MMA_MODE_REG       0x07
  #define MMA_OUTX_REG       0x00
  #define MMA_OUTY_REG       0x01
  #define MMA_OUTZ_REG       0x02
  
  uint16_t Resultat ; 
  uint8_t maxTemp = 24;
  
  TwoWire mysensorI2C=TwoWire(0);
  
  const char* mqttServer = "z96e4d99.us-east-1.emqx.cloud";
  const int mqttPort = 15281;
  const char* mqttUser = "insatmqtt";
  const char* mqttPassword = "insat2021";
  
  const char* ssid = "AndroidAp";
  const char* password = "01234567";   
   
  int Wifi_ReConnect=0;
  int Mqtt_ReConnect=0;
  
  
  WiFiClient espClient;
  PubSubClient client(espClient);

    void writeI2cReg(uint8_t RegAddr, uint8_t Value){
    
          mysensorI2C.beginTransmission(MMA_ADDRESS);
          mysensorI2C.write(RegAddr);
          mysensorI2C.write(Value);
          if (mysensorI2C.endTransmission(true)!=0){
            Serial.println ("problem writing to I2C device");
            exit(0);
          }
   }

   uint8_t readI2cReg(uint8_t RegAddr){    
          mysensorI2C.beginTransmission(MMA_ADDRESS);
          mysensorI2C.write(RegAddr);
           if (mysensorI2C.endTransmission(false)){ //if !=0
                Serial.println ("Problem writing without stop"); 
                exit(0);
            }
        mysensorI2C.requestFrom(MMA_ADDRESS,0x01);
        return(mysensorI2C.read());
   }
  
  
  void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");
  
    connectmqtttopic();
    mysensorI2C.begin(sda,scl);
  
    pinMode(LED_Blue, OUTPUT);
    pinMode(LED_Green, OUTPUT);
    pinMode(LED_Red, OUTPUT);
  
    digitalWrite(LED_Blue, HIGH);   
    digitalWrite(LED_Green, HIGH);  
    digitalWrite(LED_Red, HIGH);   

    // enable MMA7760
    writeI2cReg(MMA_MODE_REG, 0x01);
    Serial.println("MMA7760 ENABLED");

    
  }
  
  void connectmqtttopic(){
  
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
   
    while (!client.connected()) {
      Serial.println("Connecting to MQTT...");
      if (client.connect("MyEspIot", mqttUser, mqttPassword )) {//"WifiEspOnOff"
        Serial.println("connected");  
      } else {
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
        
      }
    }
   
    //client.publish("teulcwyg/commande/wifiesp_1", "OFF");
  
    if (client.subscribe("iotdevice/service/led") ||
      client.subscribe("iotdevice/service/temperature") ||
      client.subscribe("iotdevice/service/accelaration")) {
      Serial.println ("Suscribed to System. Welcome!");
    }   
  }
  
  void callback(char* topic, byte* payload, unsigned int length) {
  
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
      char topicpl;
  
  for (int i = 0; i < length; i++) {
      Serial.println((char)payload[i]);
      topicpl=(char)payload[i];
    }  
    if (topicpl=='R'){   
    digitalWrite(LED_Red,LOW); 
    digitalWrite(LED_Green,HIGH); 
    digitalWrite(LED_Blue,HIGH); 
    Serial.println("Red led ON");
    }
    else if (topicpl=='G'){   
    digitalWrite(LED_Red,HIGH); 
    digitalWrite(LED_Green,LOW); 
    digitalWrite(LED_Blue,HIGH); 
    Serial.println("Green led ON");
    }
    else if (topicpl=='B'){   
    digitalWrite(LED_Red,HIGH); 
    digitalWrite(LED_Green,HIGH); 
    digitalWrite(LED_Blue,LOW); 
    Serial.println("Blue led ON");
    } else   if (topicpl=='A'){   
    digitalWrite(LED_Red,LOW); 
    digitalWrite(LED_Green,LOW); 
    digitalWrite(LED_Blue,LOW); 
    Serial.println("All leds ON");
    }
  
   
    Serial.println();
    Serial.println("-----------------------");
  
  }
  
  void reconnect(){
    WiFi.begin(ssid, password);
  
      while (WiFi.status() != WL_CONNECTED)
      {
      Serial.println("WAITING WIFI CONNECTION .....");
      delay(1000);        
      }
        connectmqtttopic();
  }

  
  uint16_t readtemperature (uint8_t RegAddr)
  {  
    mysensorI2C.beginTransmission(RegAddr);
    mysensorI2C.write(0x00);
    mysensorI2C.endTransmission(false);
    mysensorI2C.requestFrom(RegAddr,2); 
    uint8_t highbyte = mysensorI2C.read();
    uint8_t lowbyte = mysensorI2C.read();   
    mysensorI2C.endTransmission(true);
    uint16_t resultat = lowbyte | (highbyte<<8) ;
    resultat= resultat /32;
    uint8_t temp;
    int16_t temperature;
    if (temp & (1<<10) ==0){
         temperature = temp*0.125 ;}
    else{
          temperature = (temp/(1<<15))*0.125 ;
     }  
    return(temperature) ; 
    }
  
  void loop() {
    // put your main code here, to run repeatedly:
  
  
     Serial.print ("Beginning loop with Wifi  ");
     Serial.print(Wifi_ReConnect);
     Serial.print (" Reconnexions and MQTT ");
     Serial.print (Mqtt_ReConnect);
     Serial.println(" Reconnexions" );
  
    if (WiFi.status() != WL_CONNECTED) {
      Wifi_ReConnect++;
      Serial.println("WIFI CONNECTION LOST ... WAITING");
      reconnect();
        }
  
    // control led
    if(client.subscribe("iotdevice/service/led")){
      client.loop();  
    }

    // check temperature and publish
    if (readtemperature(0x48)>maxTemp){
      String temp_str= String(readtemperature(0x48));
      client.publish("iotdevice/service/temperature",temp_str.c_str());
      delay(5000);
    }

    // calculate acceleration & print them to serial
    int8_t outX = (readI2cReg(MMA_OUTX_REG) <<2) ;
    float accelX = (outX/4)*1.5/32;

    int8_t outY = (readI2cReg(MMA_OUTY_REG)<<2)  ;
    float accelY = (outY/4)*1.5/32; 

    int8_t outZ = (readI2cReg(MMA_OUTZ_REG)<<2) ;
    float accelZ = (outZ/4)*1.5/32;  

    Serial.print (" X axis : ");Serial.println(accelX);
    Serial.print (" Y axis : ");Serial.println(accelY);
    Serial.print (" Z axis : ");Serial.println(accelZ);

    Serial.println("----------------------------");
    
    delay(3000);
  }
