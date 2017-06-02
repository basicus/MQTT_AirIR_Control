#include <IRremoteESP8266.h>  // IR LED
#include <ESP8266WiFi.h>      // WiFi
#include <PubSubClient.h>     // MQTT
#include "Tadiran.h"          // Gree HVAC remote control codes Thanks to @dpressle

// WiFi connection params
const char *ssid = "****";   // Имя вайфай точки доступа
const char *pass = "****"; // Пароль от точки доступа

// MQTT специфичные параметры
#define BUFFER_SIZE 100
const char *mqtt_server = "192.168.100.2"; // IP сервера MQTT в локальной сети
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = ""; // Логин от сервер
const char *mqtt_pass = ""; // Пароль от сервера
const char *device_id = "ESP8266-01-first";
WiFiClient espClient;
PubSubClient client(espClient);
long lastReconnectAttempt = 0; // переменная с количеством попыток переподключения

// IRDA параметры
IRsend irsend(2); //  GPIO2

// HVAC Settings (установка значений по-умолчанию)
int temperature = 27;
int mode = 4; // 0-Auto | 1-Cold | 2-Dry | 3-Fan | 4-Heat
int fanspeed = 1; //0-Auto | 1-Low | 2-Medium | 3-High
boolean power = true;
// Для проверки при включении выставляем значения по-умолчанию.
Tadiran tadiran(mode, fanspeed, temperature, STATE_on);

// WiFi функция переподключения к сети
void WIFI_Connect()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // меняем режим. иначе светит точка AI THINKER
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  for (int i = 0; i < 30; i++)
  {
    if ( WiFi.status() != WL_CONNECTED ) {
      delay ( 250 );
      Serial.print ( "." );
      delay ( 250 );
    }
  }

  if ( WiFi.status() == WL_CONNECTED ) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.print(WiFi.localIP());
    Serial.println("");
  }
  else {
    Serial.println("");
    Serial.println("Can't connect to WiFi.");
  }
}

void p_cur_settings() {
Serial.print("Temp/Mode/FanSpeed/Status: ");
Serial.print(temperature);
Serial.print("/");
Serial.print(mode);
Serial.print("/");
Serial.print(fanspeed);
Serial.print("/");
Serial.println(power);

}
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // Обработка принятого сообщения MQTT
  int changed = 0;
  payload[length] = '\0';
  String message = (char *)payload;
  
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);
  Serial.println();

  
  if (strcmp(topic,"/devices/hvac_gree/controls/enabled") == 0) {
    if ( message == "1" ) {
      Serial.println("HVAC on");
      power = true;}
      else
      { Serial.println("HVAC off"); power = false;
        }
    changed = 1;
  }
  if (strcmp(topic,"/devices/hvac_gree/controls/mode") == 0)  {
    if (message.toInt()>=0 ){
      mode = message.toInt();
      mode = 4;
      changed = 1;
    }
  }
 
  if (strcmp(topic,"/devices/hvac_gree/controls/fanspeed") == 0) {
      if (message.toInt()>=0 ) {
      fanspeed = message.toInt();
      fanspeed = 0;
      changed = 1;
    }
  }
  if (strcmp(topic,"/devices/hvac_gree/controls/temperature") == 0) {
  if (message.toInt()>0) {
      temperature = message.toInt();
      if (temperature >30) { temperature = 30;}
      if (temperature <16) {temperature =16;}
      changed = 1;
    }
  }
  if (changed==1) { 
    p_cur_settings();
    tadiran.setTemeprature(temperature);
    tadiran.setMode(mode);
    tadiran.setFan(fanspeed);
    tadiran.setState(power);
    // Отправляем дважды (для уверенности)
    irsend.sendRaw(tadiran.codes, TADIRAN_BUFFER_SIZE, 38);
    delay (100);
    irsend.sendRaw(tadiran.codes, TADIRAN_BUFFER_SIZE, 38);
    }
}


boolean reconnect() {
  if (client.connect(device_id)) {
    // Если подключились подписываемся на топики
    client.subscribe("/devices/hvac_gree/controls/enabled");
    client.subscribe("/devices/hvac_gree/controls/mode");
    client.subscribe("/devices/hvac_gree/controls/fanspeed");
    client.subscribe("/devices/hvac_gree/controls/temperature");
    Serial.print("Sucessfully connect MQTT. Error = ");
    Serial.println(client.state());
  } else {
    Serial.print("Failed to connect MQTT. Error = ");
    Serial.println(client.state());
  }
return client.connected();
}


// Установка параметров
void setup()
{
  Serial.begin(115200);
  Serial.println("");
  pinMode(2, OUTPUT); // GPIO2 - выход
  if (WiFi.status() != WL_CONNECTED) {
    WIFI_Connect();
  }
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
  lastReconnectAttempt = 0;
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    WIFI_Connect();
  }

  // Проверка существует ли подключение
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Попытка переподключения
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
}
