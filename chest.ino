#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Button.h>
#include "ESPRotary.h";

// WIFI
const char* ssid = "SSID"; // Enter your WiFi name
const char* password =  "password"; // Enter WiFi password

// MQTT
const char* mqttServer = "x.x.x.x";
const int mqttPort = 1883;
const char* mqttUser = "user";
const char* mqttPassword = "pass";
const char* tempTopic = "esp/chest/temp"; 
const char* humidityTopic = "esp/chest/humidity"; 
const char* setpointTopic = "esp/chest/setpoint";
long ms_publish;     
#define MQTT_PULISH_MS  10000 // cada cuanto ms envio un msg a mqtt
 
WiFiClient espClient;
PubSubClient client(espClient);


// temperature sensor
#define DHT_PIN 14 // D5
float temperature;
float humidity;
float last_temperature;
float last_humidity;
long ms_temp;
#define DHT_POLL_MS   10000

// display
#define TRUE  1
#define FALSE 0
int dht_changed = FALSE;
DHT dht(DHT_PIN,DHT22);

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);


// Display & Menu state machine
#define STATE_SHOW        1
#define STATE_SETPOINT    2

int state = STATE_SHOW;


// Control
float setpoint = 20.0;
float setpoint_changed = FALSE;
#define OFF 0
#define ON 1
int heat = OFF;  // caldera prendida o apagada

// button 
#define BUTTON_PIN  0 // D3
Button button(BUTTON_PIN);


// encoder

#define ROTARY_PIN1 12  // D6
#define ROTARY_PIN2 13  // D7
#define CLICKS_PER_STEP   4   // this number depends on your rotary encoder 
#define MIN_POS   100
#define MAX_POS   400
ESPRotary encoder = ESPRotary(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS);

// relay
#define RELAY_PIN 16 // D0
long ms_heat_change;
#define HYSTERESIS_MS  1000*60*1   // histeresys on off - 2 minutes
int heat_now = TRUE;
long ms_poll_control;
#define CONTROL_POLL_MS  10000  // ms cada cuanto ejecuta loop de control

/*
 * init functions
 */


void connectMQTT() {
    if(client.connected()) {
      return; 
    }

    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not ready to connect to MQTT");
      return;
    }
  
    while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ChestClient", mqttUser, mqttPassword )) {
 
      Serial.println("connected to MQTT");  

      client.subscribe(setpointTopic);
 
    } else {
 
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
 
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {

 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");

  payload[length] = 0;
  String str = String ((char *)payload);
  float new_setpoint = str.toFloat();
  if(new_setpoint > 0 ) {
    setpoint = new_setpoint;
  }

}

void initMQTT() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  connectMQTT();

  ms_publish = millis();
}


void connectWIFI() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.printDiag(Serial);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  /*
  Nota: si la conexión se establece y luego se pierde por algún motivo, ESP se reconectará automáticamente al último punto 
  de acceso utilizado una vez que vuelva a estar en línea. Esto se hará automáticamente mediante la librería WiFi, 
  sin intervención del usuario.
   */
}

void initOTA() {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
}

void initLCD() {
  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.setCursor(5,0);
  lcd.print("CHEST!");
  lcd.setCursor(1,1);
  lcd.print("Charlie's Nest");
}

void initDHT() {
  pinMode(DHT_PIN, INPUT);
  dht.begin();
  ms_temp = millis() - DHT_POLL_MS; // resto el poll period para que cuando arranque preguente enseguida por la temperatura
}

void ICACHE_RAM_ATTR encoderIRS()  {
  encoder.loop();
}

// on change
void rotate(ESPRotary& r) {
}


// on left or right rotattion
void showDirection(ESPRotary& r) {
}


void initEncoder() {
    button.begin();

    encoder.setChangedHandler(rotate);
    encoder.setLeftRotationHandler(showDirection);
    encoder.setRightRotationHandler(showDirection);

    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN1), encoderIRS, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN2), encoderIRS, CHANGE); 
}

void initRelay() {
    pinMode(RELAY_PIN,OUTPUT);
    digitalWrite(RELAY_PIN,LOW);
    ms_heat_change = millis();
}


void setup()
{

  Serial.begin(115200);
  Serial.setDebugOutput(true);


  initLCD();
  initDHT();
  connectWIFI();
  initOTA();
  initMQTT();
  initEncoder();
  initRelay();
}

/*
 * Main functions
 */
void pollTemperature() {

    if(ms_temp + DHT_POLL_MS < millis() ) {
      ms_temp = millis();
      last_temperature = temperature;
      last_humidity = humidity;  
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();
      if(last_temperature != temperature || last_humidity != humidity) {
        dht_changed = TRUE;
      } else {
        dht_changed = FALSE;  
      }
      return;
    }
    dht_changed=0; // si no hago un poleo entonces la temperatura no cambio
}

void log2Serial() {
    if(dht_changed) {
      Serial.print("Temperatura: ");
      Serial.print(temperature);
      Serial.print(" Humedad: ");
      Serial.println(humidity);
    }
}

// MQTT publish
void publishSetpoint(float sp) {
  client.publish(setpointTopic, String(sp).c_str(),true);
}

void publishTempHum(float t, float h) {
  client.publish(tempTopic, String(t).c_str(),true);
  client.publish(humidityTopic, String(h).c_str(),true);
}

void publishMQTT() {
    if(ms_publish + MQTT_PULISH_MS < millis()) {
        publishTempHum(temperature,humidity);
        publishSetpoint(setpoint);
        ms_publish = millis();
    }
}


void showTempLCD(int force) {
  if(dht_changed || force) {
    lcd.clear();// clear previous values from screen
    lcd.print("Temp:");
    lcd.setCursor(9,0);
    lcd.print(String(temperature).c_str()); 
    lcd.setCursor(15,0);
    lcd.print("o");
    lcd.setCursor(0,1);
    lcd.print("Humedad:");
    lcd.setCursor(9,1);
    lcd.print(String(humidity).c_str()); 
    lcd.setCursor(15,1);
    lcd.print("%");
  }
}

void showSetpointLCD(int force) {
    if(setpoint_changed || force) {
        lcd.clear();// clear previous values from screen
        lcd.print("Setpoint:");
        lcd.setCursor(10,0);
        lcd.print(String(setpoint).c_str()); 
    }

}


void display() {
  switch (state) {
    case STATE_SHOW:
      showTempLCD(FALSE);
      break;
    case STATE_SETPOINT:
      showSetpointLCD(FALSE);
      break;
  }
}

void checkState() {
  if ( button.pressed() )  {
      Serial.println("Button pressed");
      switch (state) {
        case STATE_SHOW:
          encoder.resetPosition((int)(setpoint*10*CLICKS_PER_STEP));
          state = STATE_SETPOINT;
          showSetpointLCD(TRUE);
          break;
        case STATE_SETPOINT:
          state = STATE_SHOW;
          showTempLCD(TRUE);
          break;
    }
  }

  if(state == STATE_SETPOINT) {
    float new_setpoint = (float)encoder.getPosition()/10;
    if(new_setpoint != setpoint) {
        setpoint = new_setpoint;
        setpoint_changed = TRUE;
    } else {
      setpoint_changed = FALSE;
    }
  }
  
}

void controlHeat() {
  long ms =  millis();

  // el check de control se hace cada CONTROL_POLL_MS
  if(ms_poll_control + CONTROL_POLL_MS < ms) {
    ms_poll_control = ms;

  
    // se tona una accion si al memos paso HYSTERESIS_MS desde la ultima accion 
    if( ms_heat_change + HYSTERESIS_MS < ms || heat_now ) {
  
      int status = digitalRead(RELAY_PIN);
      if(temperature < setpoint ) {
        if(status == LOW) {
          heat_now = FALSE;
          digitalWrite(RELAY_PIN,HIGH);
          ms_heat_change = ms;
        }
      } else {
        if(status == HIGH) {
          digitalWrite(RELAY_PIN,LOW);
          ms_heat_change = ms;
        }
      }
    } 

    
  }




  
}


/*
 * LOOP
 */

void loop() {
  pollTemperature();  
  log2Serial();
  ArduinoOTA.handle(); 
  if(  ! client.loop() ) {
    Serial.println("MQTT disconnected");
    connectMQTT();
  } else {
      publishMQTT();
  }

  checkState();
  display();
  controlHeat();
}
