//Импорты библиотек
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//Указываем тип датчика
#define DHTTYPE DHT11

//Пины для датчиков
#define DHTPIN D4
#define SensorPin A0

//Пины для реле
#define PIN_RELAY1 5
#define PIN_RELAY2 4

//создаем объект датчика
DHT dht(DHTPIN, DHTTYPE);

//Константы для mqtt
const char* ssid = "Mixa";
const char* password = "00000000";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* controlTopic = "/isu/mg-control/devices/6440030a6661e89b4496461a/control";
const char* dataTopic = "/isu/mg-control/devices/6440030a6661e89b4496461a/data";

WiFiClient espClient;
PubSubClient client(espClient);

String receivedMessage;
bool isLightOn;
bool isDry;

void setup()
{
  Serial.begin(9600);

  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  digitalWrite(PIN_RELAY1, HIGH);
  digitalWrite(PIN_RELAY2, HIGH);
  dht.begin();


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");


  client.setServer(mqttServer, mqttPort);
  client.setCallback(on_message);

  while (!client.connected()) {
    if (client.connect("ESP8266Client")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(controlTopic);
    } else {
      Serial.print("Failed to connect to MQTT broker. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}


void loop()
{
  // Подключение к MQTT брокеру, если не подключено
  if (!client.connected()) {
    Serial.print("Подключение к MQTT брокеру ");
    Serial.println(mqttServer);
    if (client.connect("nodemcu-client")) {
      Serial.println("Подключено");
      client.subscribe(controlTopic);
    } else {
      Serial.print("Ошибка подключения, код ошибки = ");
      Serial.println(client.state());
      delay(5000);
      return;
    }
  }

  delay(5000);
  float sensorValue = analogRead(SensorPin);
  float h = dht.readHumidity();     // считываем влажность
  float t = dht.readTemperature();  // считываем температуру
  Serial.print("Влажность воздуха: ");
  Serial.print(h);
  Serial.println("%\t");
  Serial.print("Температура воздуха: ");
  Serial.print(t);
  Serial.println("°C");
  Serial.print("Влажность земли: ");
  Serial.println(sensorValue);

  if (sensorValue < 800){
    Serial.println("Wet");
    isDry = false;
  }
  else if (sensorValue > 850){
    Serial.println("Dry");
    digitalWrite(PIN_RELAY1, LOW);
    delay(5000);
    digitalWrite(PIN_RELAY1, HIGH);
    isDry = true;
  }
  char message[200]; // Создаем массив символов для хранения отформатированной строки

  // Форматируем строку с использованием переменных
  sprintf(message, "{\"air\":{\"temp\":%.2f,\"humidity\":%.2f},\"soil\":{\"isDry\":%d},\"isLightOn\":%d}", t, h, isDry ? 1 : 0, isLightOn ? 1 : 0);

  // Отправляем отформатированную строку в MQTT
  client.publish(dataTopic, message);



  // Получение сообщений из топика MQTT
  client.loop();
}

void on_message(char* topic, byte* payload, unsigned int length) {
  // Обработка полученного сообщения
  receivedMessage = ""; // Сброс предыдущего значения
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }
  
  // Вывод сообщения на Serial порт
  Serial.println("Received message on topic: " + String(topic));
  Serial.print("Message payload: ");
  Serial.println(receivedMessage);
  if (receivedMessage == "light on"){
    digitalWrite(PIN_RELAY2, LOW);
    isLightOn = true;
  }
  else {
    digitalWrite(PIN_RELAY2, HIGH);
    isLightOn = false;
  }
}