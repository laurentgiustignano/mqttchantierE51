#include <Arduino.h>
#include "PubSubClient.h"
#include "WiFi.h"
#include "config.h"
#include <DHT.h>
#include <DHT_U.h>
#include <config.h>

// Prédéclaration
class PubSubClient;
class DHT;

// Wifi
char *ssid=SSID;                 // Your personal network SSID
const char *wifi_password=SSID_PWD; // Your personal network password

// MQTT
const char *mqtt_server=IP_MQTT_SERVER;  // IP of the MQTT broker
const char *clientID="espclient"; // MQTT clientMqtt ID
char payload[100];

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;

DHT dht(DHTPIN, DHTTYPE);

// 1883 is the listener port for the Broker
PubSubClient clientMqtt(mqtt_server, PORT_MQTT_SERVER, wifiClient);

// Custom function to connet to the MQTT broker via WiFi
void connect_MQTT() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    clientMqtt.setServer(mqtt_server, 1883);
    clientMqtt.connect(clientID);

}

void setup() {
    dht.begin();
    WiFi.begin(ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    connect_MQTT();
    pinMode(PORTE_PIN, INPUT);
}

void loop() {
    if (!clientMqtt.connected()) {
        connect_MQTT();
    }

    auto lectureTemperature=(unsigned short) dht.readTemperature();
    auto etatPorte=digitalRead(PORTE_PIN);
    static auto compteur=0;
    static auto sommeTemperature=0;
    static unsigned short mesurePrecedente=0;
    static unsigned short moyennePrecedente=0;
    static unsigned short etatPortePrecedent=0;

    if (etatPortePrecedent != etatPorte) {
        etatPortePrecedent=etatPorte;
        sprintf(payload, "%d", etatPorte);
        clientMqtt.publish(TOPIC_PORTE_ETAT, payload);
    }

    if (mesurePrecedente != lectureTemperature) {
        mesurePrecedente=lectureTemperature;
        sprintf(payload, "%d", lectureTemperature);
        clientMqtt.publish(TOPIC_TEMPERATURE_MESURE, payload);
    }

    if (compteur < 20) {
        sommeTemperature+=lectureTemperature;
        compteur++;
    }
    else {
        auto moyenneTemperature=sommeTemperature / compteur;

        if (moyennePrecedente != moyenneTemperature) {
            sprintf(payload, "%d", moyenneTemperature);
            clientMqtt.publish(TOPIC_TEMPERATURE_MOYENNE, payload);
            moyennePrecedente=moyenneTemperature;
        }
        compteur=0;
        sommeTemperature=0;
    }

    delay(1000);
    clientMqtt.loop();
}