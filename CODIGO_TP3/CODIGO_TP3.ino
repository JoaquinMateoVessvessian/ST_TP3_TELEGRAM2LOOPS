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
void Maquina_De_Estados(int EstadoBoton1, int EstadoBoton2, float TemperaturaActual);

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
  float t = dht.readTemperature(); //LEO LA TEMPERATURA 
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
      char st[20]; //caratacteres que tiene st 
      sprintf(st, "Temperatura: %.2f", t); //paso a t(float) a una variable string de 20 caracteres,
      bot.sendMessage(chat_id, st); // envio el mensaje
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
    t = dht.readTemperature(); // guardo en t el valor de la temperatura
      if (t > umbral && Mensaje == true) { //Si la temperatura es mayor a umbral y el todavia no se habia enviado un mensaje sobre el cambio de temperatura...
        char st[70]; //le asigno 70 caracteres a st
        sprintf(st, "Se superó el valor umbral y la temperatura actual es %.2f °C", t); 
        bot.sendMessage(CHAT_ID, st);
        Mensaje = false; //Paso el mensaje a false, esto significa que ya se mando un mensaje
      }
      if (millis() - bot_lasttime > BOT_MTBS) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1); //se guarda la cantidad de mensajes recibidos.
        if (numNewMessages > 0) { //si se recibio un mensaje por telegram...
          handleNewMessages(numNewMessages); //se llama a la funcion
          bot_lasttime = millis();
        } 
      }
      if (umbral > t){
        Mensaje = true; //si la temperatura vuelve a ser menor que el umbral, se rehabilita Mensaje para enviar un mensaje cuando se supere el valor umbral.
      }
  }
}

//LOOP PARA MAQUINA DE ESTADOS
void Task2code(void *pvParameters) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    t = dht.readTemperature();
    boton1 = digitalRead(BOTON1);
    boton2 = digitalRead(BOTON2);
    Maquina_De_Estados(boton1, boton2, t);
  }
}

void loop() {
}

void Maquina_De_Estados(int EstadoBoton1, int EstadoBoton2, float TemperaturaActual) {
  char stringt[10];
  char stringu[10];
  tiempoactual = millis(); //se comienza a contar el tiempo
  switch (estado) {
    case P1:
      sprintf(stringt, "%.2f", TemperaturaActual);
      sprintf(stringu, "%d", umbral); 
      u8g2.clearBuffer(); //se reinicia la pantalla
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(15, 15, "Temp:");
      u8g2.drawStr(60, 15, stringt);
      u8g2.drawStr(15, 30, "Umbral:");
      u8g2.drawStr(70, 30, stringu);
      u8g2.sendBuffer(); //se "dibuja" sobre la pantalla
      if (TemperaturaActual >= umbral) { //Si la temperatura es mayor a umbral se prende el LED
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
        tiempo = tiempoactual; //guardo el tiempo actual en tiempo antes de hacer el cambio para que cuadno este en el siguiente estado se empiece a contar desde "0" (en teoria)
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
      if (tiempoactual - tiempo >= 5000) { //Si pasan 5 segundos y no se presiono el siguiente boton, se vuelve a la pantalla 1
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
}