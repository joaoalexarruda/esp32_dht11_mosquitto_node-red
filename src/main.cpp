// Inclui bibliotecas necessárias
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

// Wi-Fi login e senha
const char *ssid = "joaoalex1";
const char *password = "joao1579";

// IPv4 e porta
#define mqtt_server "192.168.29.165"
#define mqtt_port 1883

// Pino utilizado no ESP32 e sensor DHT11
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Inicializar cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Buffer circular para leituras
const int numReadings = 10;
float temperatureReadings[numReadings];
float humidityReadings[numReadings];
int readingsIndex = 0;

void setup_wifi() {
  // Conectar ao Wi-Fi
  delay(10);
  Serial.println("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando...");
  }
  Serial.println("Conectado ao Wi-Fi");
}

void reconnect() {
  // Reconectar ao broker MQTT
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao broker MQTT");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void updateReadings(float *readings, float newValue) {
  // Atualizar o buffer circular de leituras
  readings[readingsIndex] = newValue;
  readingsIndex = (readingsIndex + 1) % numReadings;
}

float calculateMovingAverage(float *readings) {
  // Calcular a média móvel das leituras
  float sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += readings[i];
  }
  return sum / numReadings;
}

void setup() {
  // Inicializar o ESP32 e o sensor DHT
  Serial.begin(115200);
  dht.begin();
  // Inicializar o Wi-Fi e o MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  // Reconectar ao broker MQTT se a conexão cair
  if (!client.connected()) {
    reconnect();
  }

  // Ler temperatura e umidade do sensor DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    // Ignorar leituras inválidas
    Serial.println("Falha ao ler do sensor DHT");
    delay(2000);
    return;
  }

  // Atualizar leituras
  updateReadings(temperatureReadings, temperature);
  updateReadings(humidityReadings, humidity);

  // Calcular médias móveis
  float avgTemperature = calculateMovingAverage(temperatureReadings);
  float avgHumidity = calculateMovingAverage(humidityReadings);

  // Converter valores float para strings para payload MQTT
  char tempStr[8];
  dtostrf(temperature, 2, 2, tempStr);

  char humStr[8];
  dtostrf(humidity, 2, 2, humStr);

  char avgTempStr[8];
  dtostrf(avgTemperature, 2, 2, avgTempStr);

  char avgHumStr[8];
  dtostrf(avgHumidity, 2, 2, avgHumStr);

  // Tópicos MQTT para publicar os dados
  char tempTopic[] = "esp32/temperatura";
  char humTopic[] = "esp32/umidade";
  char avgTempTopic[] = "esp32/media_movel_temperatura";
  char avgHumTopic[] = "esp32/media_movel_umidade";

  // Publicar valores atuais e médias móveis nos tópicos MQTT
  client.publish(tempTopic, tempStr);
  client.publish(humTopic, humStr);
  client.publish(avgTempTopic, avgTempStr);
  client.publish(avgHumTopic, avgHumStr);

  // Imprimir informações de debug
  Serial.println("Publicado temperatura no tópico MQTT: " + String(tempTopic));
  Serial.println("Publicado umidade no tópico MQTT: " + String(humTopic));
  Serial.println("Publicada média móvel de temperatura no tópico MQTT: " + String(avgTempTopic));
  Serial.println("Publicada média móvel de umidade no tópico MQTT: " + String(avgHumTopic));

  delay(5000);
}
