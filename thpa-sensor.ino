#include <ESP8266WiFi.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "secrets.h"


#define BME_DEFAULT_I2C_ADDRESS (0x76)
#define PRESSURE_AT_SEA_LEVEL_HPA (1013.25)


Adafruit_BME280 bme;
WiFiClientSecure wifi_client;
PubSubClient client(wifi_client);


void setup_bme() {
  if (!bme.begin(BME_DEFAULT_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor!");

    while (true);
  }
}

void setup_wifi() {
  WiFi.mode (WIFI_STA);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  Serial.print("Connecting to ");
  Serial.print(SECRET_WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  wifi_client.setFingerprint(SECRET_FINGERPRINT);

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT server.");
    if (client.connect(SECRET_MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD)) {
      Serial.println("Connected to MQTT server.");
    } else {
      delay(5000);
    }
  }
}

void send_data(float temperature, float humidity, float pressure, float altitude) {
  const int capacity = JSON_OBJECT_SIZE(4);
  StaticJsonDocument<capacity> doc;

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;
  doc["altitude"] = altitude;

  char buffer[128];
  serializeJson(doc, buffer);
  Serial.println(buffer);

  bool res = client.publish(SECRET_MQTT_TOPIC, buffer);
  Serial.println(res);
  yield();
}

void setup() {
  Serial.begin(115200);

  setup_bme();
  setup_wifi();

  client.setServer(SECRET_MQTT_HOST, SECRET_MQTT_PORT);
}


void loop() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure();
  float altitude = bme.readAltitude(PRESSURE_AT_SEA_LEVEL_HPA);

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println("%");

  Serial.print("Pressure = ");
  Serial.print(pressure / 100.0F);
  Serial.println("hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(altitude);
  Serial.println("m");
  Serial.println();

  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();

  send_data(temperature, humidity, pressure, altitude);

  delay(5000);
}
