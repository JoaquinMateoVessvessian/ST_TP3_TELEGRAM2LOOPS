//SAGGIORATO & VESSVESSIAN & JUDCOVSKI
#include <UniversalTelegramBot.h>

#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <Wire.h>

TaskHandle_t Task1;
TaskHandle_t Task2;
#include <U8g2lib.h>
#include <U8x8lib.h>

#include <DHT.h>
#include <DHT_U.h>

#include <Adafruit_Sensor.h>
WiFiServer server(80);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
int Maquina_De_Estados(int EstadoBoton1, int EstadoBoton2, float TemperaturaActual);

#define DHTPIN 23
#define DHTTYPE DHT11

#define P1 2000
#define P2 2001
#define ESPERA1 2002
#define ESPERA2 2003
#define ESPERA3 2004
#define ESPERA4 2005
#define PASAJE1 2006
#define PASAJE2 2007
#define SUMA 2008
#define RESTA 2009

#define BOTON1 35
#define BOTON2 34

#define BOT_TOKEN "8088984847:AAFRTduncYhlQxacA6Kju-6CJfyp3j0XZiE"
#define CHAT_ID "7305304041"

int tiempo = 0;
int tiempoactual;
bool Mensaje = true;
float t;

const char *ssid = "ORT-IoT";
const char *password = "NuevaIOT$25";
const char *ntpServer = "pool.ntp.org";

int vumbral;
int estado;
int umbral = 28;
int boton1;
int boton2;
DHT dht(DHTPIN, DHTTYPE);

const unsigned long BOT_MTBS = 1000;  // mean time between scan messages

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;  // last time messages' scan has been done
bool Start = false;
void handleNewMessages(int numNewMessages) {
  float t = dht.readTemperature();
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/start") {
      String welcome = "Welcome to Universal Arduino Telegram Bot library, " + from_name + ".\n";
      welcome += "This is Chat Action Bot example.\n\n";
      welcome += "/t : para ver la temperatura actual\n";
      bot.sendMessage(chat_id, welcome);
    }
    if (text == "/t") {
      char st[20];
      sprintf(st, "Temperatura: %.2f", t);
      bot.sendMessage(chat_id, st);
    }
  }
}
void setup() {
  Serial.begin(115200);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  pinMode(BOTON1, INPUT);
  pinMode(BOTON2, INPUT);
  u8g2.begin();
  dht.begin();
  estado = P1;
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  WiFi.begin(ssid, password);
  xTaskCreatePinnedToCore(
    Task1code, /* Task function. */
    "Task1",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */
  delay(500);

  xTaskCreatePinnedToCore(
    Task2code, /* Task function. */
    "Task2",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task2,    /* Task handle to keep track of created task */
    1);        /* pin task to core 1 */
  delay(500);
}

//LOOP PARA TELEGRAM
void Task1code(void *pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    t = dht.readTemperature();
      if (t > umbral && Mensaje == true) {
        char st[50];
        sprintf(st, "Se superó el valor umbral y la temperatura actual es %.2f °C", t);
        bot.sendMessage(CHAT_ID, st);
        Mensaje = false;
      }
      if (millis() - bot_lasttime > BOT_MTBS) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        if (numNewMessages > 0) {
          handleNewMessages(numNewMessages);
          bot_lasttime = millis();
        } 
      }
      if (umbral > t){
        Mensaje = true;
      }
  }
}

//LOOP PARA MAQUINA DE ESTADOS
void Task2code(void *pvParameters) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    digitalWrite(25, HIGH);
    t = dht.readTemperature();
    boton1 = digitalRead(BOTON1);
    boton2 = digitalRead(BOTON2);
    umbral = Maquina_De_Estados(boton1, boton2, t);
  }
}

void loop() {
}

int Maquina_De_Estados(int EstadoBoton1, int EstadoBoton2, float TemperaturaActual) {
  char stringt[10];
  char stringu[10];
  tiempoactual = millis();
  switch (estado) {
    case P1:
      sprintf(stringt, "%.2f", TemperaturaActual);
      sprintf(stringu, "%d", umbral);
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(15, 15, "Temp:");
      u8g2.drawStr(60, 15, stringt);
      u8g2.drawStr(15, 30, "Umbral:");
      u8g2.drawStr(70, 30, stringu);
      u8g2.sendBuffer();
      if (TemperaturaActual >= umbral) {
        digitalWrite(25, HIGH);
      }
      else {
       digitalWrite(25, LOW); 
      }
      if (EstadoBoton1 == LOW) {
        estado = ESPERA1;
      }
      break;
    case ESPERA1:
      Serial.println("ESPERA1");
      if (EstadoBoton1 == HIGH) {
        tiempo = tiempoactual;
        estado = PASAJE1;
      }
      break;
    case PASAJE1:
      Serial.println(tiempoactual - tiempo);
      Serial.println("PASAJE1");
      if (EstadoBoton2 == LOW) {
        tiempo = tiempoactual;
        estado = ESPERA2;
      }
      if (tiempoactual - tiempo >= 5000) {
        if (EstadoBoton1 == HIGH) {
          tiempo = tiempoactual;
          estado = P1;
        }
      }
      break;
    case ESPERA2:
      Serial.println("ESPERA2");
      if (EstadoBoton2 == HIGH) {
        tiempo = tiempoactual;
        estado = PASAJE2;
      }
      break;
    case PASAJE2:
      Serial.println(tiempoactual - tiempo);
      Serial.println("PASAJE2");
      if (EstadoBoton1 == LOW) {
        tiempo = tiempoactual;
        estado = ESPERA3;
      }
      if (tiempoactual - tiempo >= 5000) {
        if (EstadoBoton2 == HIGH) {
          tiempo = tiempoactual;
          estado = P1;
        }
      }
      break;
    case ESPERA3:
      Serial.println("ESPERA3");
      if (EstadoBoton1 == HIGH) {
        estado = P2;
      }
      break;
    case ESPERA4:
      if (EstadoBoton1 == HIGH && EstadoBoton2 == HIGH) {
        estado = P1;
      }
      break;
    case P2:
      Serial.println(umbral);
      sprintf(stringu, "%d", umbral);
      u8g2.clearBuffer();
      u8g2.drawStr(15, 15, "Umbral:");
      u8g2.drawStr(60, 15, stringu);
      u8g2.sendBuffer();
      if (EstadoBoton1 == LOW && EstadoBoton2 == HIGH) {
        estado = SUMA;
      }
      if (EstadoBoton2 == LOW && EstadoBoton1 == HIGH) {
        estado = RESTA;
      }
      if (EstadoBoton1 == LOW && EstadoBoton2 == LOW) {
        estado = ESPERA4;
      }
      break;
    case SUMA:
      if (EstadoBoton1 == HIGH) {
        umbral = umbral + 1;
        estado = P2;
      }
      if (EstadoBoton1 == LOW && EstadoBoton2 == LOW) {
        estado = ESPERA4;
      }
      break;
    case RESTA:
      if (EstadoBoton2 == HIGH) {
        umbral = umbral - 1;
        estado = P2;
      }
      if (EstadoBoton1 == LOW && EstadoBoton2 == LOW) {
        estado = ESPERA4;
      }
      break;
  }
  return umbral;
}