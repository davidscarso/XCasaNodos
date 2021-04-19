#include <ESP8266WiFi.h>
#include "PubSubClient.h"
// TODO:
// Al reconectar se debe actualizar el estado en la nueva si es que se cambio mientras no habia conexion.
// ver de colocar un bandera para estado de ultima modificacion, local o web

double pendiente1;
double pendiente2;

String s_orden01;
String s_orden02;
String s_encender01;
String s_encender02;

String s_clienteNombre;
String s_estado01;
String s_estado02;

boolean b_esReset;
volatile boolean b_estado1ant;
volatile boolean b_estado2ant;

const char mqtt_wifi_ssid[] = "Fibertel Moco";
const char mqtt_wifi_pass[] = "00492505506";

const char mqtt_broker[] = "homeassistant";
const int mqtt_port = 1883;
const char mqtt_user[] = "admin";
const char mqtt_pass[] = "eureka9";
const char mqtt_clientid[] = "MQTTC001";

WiFiClient mqtt_wifiClient;
PubSubClient mqtt_client(mqtt_wifiClient);

char mqtt_payload[64];

const int LED_Az = D8;
const int REL_1 = D7;
const int REL_2 = D1;

const int Pul1 = D5;
const int Pul2 = D6;

//para contra efecto rebote o DEBOUNCE
const int timeThreshold = 200;
long startTime1 = 0;
long startTime2 = 0;

void mqtt_setup() {
  delay(10);;
  WiFi.begin(mqtt_wifi_ssid, mqtt_wifi_pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  randomSeed(micros());
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_subscribe();
}

void mqtt_loop() {
  if (!mqtt_client.connected()) {
    digitalWrite(LED_Az, LOW);
    mqtt_client.connect(mqtt_clientid, mqtt_user, mqtt_pass);
    mqtt_subscribe();
  }
  if (mqtt_client.connected()) {
    digitalWrite(LED_Az, HIGH);

    // ACA COMPARAR ESTADOS SI EL ESTADO DEL LED CAMBIO DURANTE LA DESCONECION, ENVIAR NUEVAMENTE

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
  if (String(_topic) == String(s_encender02))s_orden02 = vt;
}

void mqtt_subscribe() {
  mqtt_client.subscribe(String(s_encender01).c_str());
  mqtt_client.subscribe(String(s_encender02).c_str());
}

void IRAM_ATTR Pulsado1() {

  if (millis() - startTime1 > timeThreshold)
  {
    if (b_estado1ant) {
      // off en circuito cerrado On es circuito abierto!
      digitalWrite(REL_1, HIGH);

      b_estado1ant = false;
      if (mqtt_client.connected()) {
        mqtt_client.publish(String(s_encender01).c_str(), String(String("OFF")).c_str());
      }
    } else {
      digitalWrite(REL_1, LOW);

      b_estado1ant = true;
      if (mqtt_client.connected()) {
        mqtt_client.publish(String(s_encender01).c_str(), String(String("ON")).c_str());
      }
    }
    s_orden01 = String("");

    startTime1 = millis();
  }
}

void IRAM_ATTR Pulsado2() {
  if (millis() - startTime2 > timeThreshold)
  {
    if (b_estado2ant) {
      // off en circuito cerrado On es circuito abierto!
      digitalWrite(REL_2, HIGH);

      b_estado2ant = false;
      if (mqtt_client.connected()) {
        mqtt_client.publish(String(s_encender02).c_str(), String(String("OFF")).c_str());
      }

    } else {
      digitalWrite(REL_2, LOW);

      b_estado2ant = true;
      if (mqtt_client.connected()) {
        mqtt_client.publish(String(s_encender02).c_str(), String(String("ON")).c_str());
      }
    }
    s_orden02 = String("");

    startTime2 = millis();
  }

}

void Reles() {
  //if (mqtt_client.connected()) {
  //digitalWrite(LED_Az, HIGH);
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
  if (String(s_orden02).equals(String("ON"))) {
    digitalWrite(REL_2, LOW);
    b_estado2ant = true;
    mqtt_client.publish(String(s_estado02).c_str(), String(String("ON")).c_str());

  } else if (String(s_orden02).equals(String("OFF"))) {
    digitalWrite(REL_2, HIGH);
    b_estado2ant = false;
    mqtt_client.publish(String(s_estado02).c_str(), String(String("OFF")).c_str());
  }
  s_orden02 = String("");

  //} else {
  // digitalWrite(LED_Az, LOW);
  //}
}

void setup()
{
  pinMode(LED_Az, OUTPUT);
  pinMode(REL_1, OUTPUT);
  pinMode(REL_2, OUTPUT);
  mqtt_setup();

  pinMode(Pul1, INPUT_PULLUP);
  pinMode(Pul2, INPUT_PULLUP);


  attachInterrupt(digitalPinToInterrupt(Pul1), Pulsado1, RISING);
  attachInterrupt(digitalPinToInterrupt(Pul2), Pulsado2, RISING);

  ESP.wdtDisable();
  digitalWrite(LED_Az, LOW);

  digitalWrite(REL_1, HIGH);
  digitalWrite(REL_2, HIGH);

  // en estado ON es para que si estaba enncendi la luz antes de que se desconectara o se cortara la laimentaicon, se resetea a apagao.
  b_esReset = true;
  b_estado1ant = true;
  b_estado2ant = true;
  s_clienteNombre = String("MQTTC001");

  s_encender01 = String(s_clienteNombre) + String("/pulsador01");
  s_encender02 = String(s_clienteNombre) + String("/pulsador02");
  s_estado01 = String(s_clienteNombre) + String("/estado/pulsador01");
  s_estado02 = String(s_clienteNombre) + String("/estado/pulsador02");
  s_orden01 = String("");
  s_orden02 = String("");

}


void loop()
{
  yield();

  mqtt_loop();
  if (b_esReset) {
    if (mqtt_client.connected()) {
      mqtt_client.publish(String(s_encender01).c_str(), String(String("OFF")).c_str());
      mqtt_client.publish(String(s_encender02).c_str(), String(String("OFF")).c_str());
    }
    b_esReset = false;

  } else {

    Reles();

  }
  ESP.wdtFeed();

}
