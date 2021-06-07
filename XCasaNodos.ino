/*
   Codigo solopara ESP01
   no tiene watch dog
   no tiene interupciones
*/
#include <ESP8266WiFi.h>
#include "PubSubClient.h"

//GPIO0->0->FLASH
//GPIO1->TX->1
//GPIO2->2
//GPIO3->RX->3
//pinMode(3, OUTPUT);
//pinMode(1, OUTPUT);
//pinMode(0, OUTPUT);
//pinMode(2, OUTPUT);

double pendiente1;

String s_orden01;
String s_encender01;
String s_estado01;

String s_clienteNombre;

boolean b_esReset;

volatile boolean b_estado1ant;

const char mqtt_wifi_ssid[] = "Fibertel Moco";
const char mqtt_wifi_pass[] = "00492505506";
const char mqtt_broker[] = "homeassistant";
const int mqtt_port = 1883;
const char mqtt_user[] = "admin";
const char mqtt_pass[] = "3UreK49";
const char mqtt_clientid[] = "MQTTC004";

WiFiClient mqtt_wifiClient;
PubSubClient mqtt_client(mqtt_wifiClient);

char mqtt_payload[64];

const int LED_Az = LED_BUILTIN;// Herramientas/builtin led: debe ser el 1 para sp1 y 2 para sp-1s

const int REL_1 = 0;
const int Pul1 = 2; //OJO que active el Pulsador!!!

//para contra efecto rebote o DEBOUNCE
const unsigned int timeThreshold = 200;
unsigned long startTime1 = 0;

void mqtt_setup() {
  delay(10);
  WiFi.begin(mqtt_wifi_ssid, mqtt_wifi_pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  randomSeed(micros());
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_subscribe();
}

void mqtt_loop() {
  if (!mqtt_client.connected()) {
    digitalWrite(LED_Az, HIGH);
    mqtt_client.connect(mqtt_clientid, mqtt_user, mqtt_pass);
    mqtt_subscribe();
  }
  if (mqtt_client.connected()) {
    digitalWrite(LED_Az, LOW);
    mqtt_client.loop();
  }
}

double mqtt_payload2double(unsigned char *_payload, int _length) {
  int i;
  for (i = 0; i < _length && i < 64; i++) {
    mqtt_payload[i] = _payload[i];
  }
  mqtt_payload[i] = 0;
  return atof(mqtt_payload);
}

String mqtt_payload2string(unsigned char *_payload, int _length) {
  int i;
  for (i = 0; i < _length && i < 64; i++) {
    mqtt_payload[i] = _payload[i];
  }
  mqtt_payload[i] = 0;
  return String(mqtt_payload);
}
void mqtt_callback(char* _topic, unsigned char* _payload, unsigned int _payloadlength) {
  double v = mqtt_payload2double(_payload, _payloadlength);
  String vt = mqtt_payload2string(_payload, _payloadlength);
  if (String(_topic) == String(s_encender01))s_orden01 = vt;
}

void mqtt_subscribe() {
  mqtt_client.subscribe(String(s_encender01).c_str());
}

void Reles() {
  if (String(s_orden01).equals(String("ON"))) {
    digitalWrite(REL_1, LOW);
    b_estado1ant = true;
    mqtt_client.publish(String(s_estado01).c_str(), String(String("ON")).c_str());

  } else if (String(s_orden01).equals(String("OFF"))) {
    digitalWrite(REL_1, HIGH);
    b_estado1ant = false;
    mqtt_client.publish(String(s_estado01).c_str(), String(String("OFF")).c_str());
  }
  s_orden01 = String("");
}

void setup()
{
  pinMode(LED_Az, OUTPUT);
  pinMode(REL_1, OUTPUT);
  mqtt_setup();

  digitalWrite(LED_Az, HIGH);
  digitalWrite(REL_1, HIGH);

  // en estado ON es para que si estaba enncendi la luz antes de que se desconectara o se cortara la laimentaicon, se resetea a apagao.
  b_esReset = true;
  b_estado1ant = true;
  s_clienteNombre = String("MQTTC004");

  s_encender01 = String(s_clienteNombre) + String("/pulsador01");
  s_estado01 = String(s_clienteNombre) + String("/estado/pulsador01");

  s_orden01 = String("");
}


void loop()
{
  yield();

  mqtt_loop();
  if (b_esReset) {
    //si se conecta luego de un corte de alimentacion
    if (mqtt_client.connected()) {
      mqtt_client.publish(String(s_encender01).c_str(), String(String("OFF")).c_str());
    }
    b_esReset = false;
  } else {
    Reles();
  }
}
