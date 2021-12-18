////////////////////  Librerias ////////////////////
#include <Arduino.h>
#include <WiFi.h>
#include <Esp32MQTTClient.h>
#include <WiFiClientSecure.h>
#include "Adafruit_Sensor.h"   
#include "DHT.h"                
#include "iot_configs.h"      


////////////////////  Definiciones ////////////////////
#define INTERVAL 10000        // Tiempo entre cada mensaje enviado
#define MESSAGE_MAX_LEN 256   // Tamaño del mensaje
#define LEDPIN 2              // Pin del Led
DHT dht(26,DHT11);            // Intancia del sensor DHT11
#define HUMIDITYSENSOR1 32    // Sensor de humedad 1
#define HUMIDITYSENSOR2 33    // Sensor de humedad 2
#define LDR 35                // Sensor de iluminación 
#define fan_in1 5             // Puente H, ventilador
#define fan_in2 18
#define valve_pin 19          // Electro válvula controlado con relé
#define light_pin 21          // Foco controlado con relé

////////////////////  Variables ////////////////////
// Credenciales definidas en iot_configs.h
const char* ssid     = IOT_CONFIG_WIFI_SSID;
const char* password = IOT_CONFIG_WIFI_PASSWORD;
static const char* connectionString = DEVICE_CONNECTION_STRING;

// Variables para la comunicación
const char *messageData = "{\"messageId\":%d, \"temperature\":%f, \"air_humidity\":%f, \"heat\":%f, \"gnd_humidity1\":%i, \"gnd_humidity2\":%i, \"illuminance\":%i, \"fan_state\":\"%s\", \"light_state\":\"%s\", \"valve_state\":\"%s\", \"mode_state\":\"%s\"}";  
// ↑↑↑ Estructura del mensaje, se puede editar para enviar toda la telemetria que se necesite. Siempre respetando el formato JSON ↑↑↑
static bool hasIoTHub = false;
static bool hasWifi = false;
int messageCount = 1;
static bool messageSending = true;
static uint64_t send_interval_ms;

// Variables de estado
boolean LED = false;
boolean valve = false;
boolean light = false;
boolean fan1 = false;
boolean fan2 = false; 
boolean automode = false;


//////////////////////////////////////////////////////
////////////////////  Funciones  ////////////////////
////////////////////////////////////////////////////

// Función de Conexiones 
void setup_connections (){
  //Conexión Wifi
  Serial.println ("Iniciando conección Wifi...");
  delay (10);
  WiFi.mode (WIFI_AP);
  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
    hasWifi = false;
  }
  hasWifi = true;
  Serial.println ("Conexión exitosa");
  Serial.print ("La dirección IP es: ");
  Serial.println (WiFi.localIP());
  
  // Conexión a IoT Central
  Serial.println ("Conectando a Azure IoT Central");
  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString, true)) {
    hasIoTHub = false;
    Serial.println("La conexión a Azure IoT Central ha fallado");
    return;
  }
  hasIoTHub = true;
}

void self_maintenance (float temperatureDH11, float air_humidity, float heat, int humedad1, int humedad2, int light){
  // Enciende la lampara
  if (temperatureDH11 < 15){
    digitalWrite (light_pin, HIGH);
  } else {
    digitalWrite (light_pin, LOW);
  }

  // Enciende el ventilador
  if (temperatureDH11 > 25){
    digitalWrite (fan1, HIGH);
  } else {
    digitalWrite (fan1, LOW);
  }

  // Abre la valvula para espacio 1
  if (humedad1 < 30){
    digitalWrite (valve_pin, HIGH);
    delay (2000);
    digitalWrite (valve_pin, LOW);
  } 

  // Abre la valvula para espacio 2
  if (humedad2 < 30){
    digitalWrite (valve_pin, HIGH);
    delay (2000);
    digitalWrite (valve_pin, LOW);
  } 
}

// Confirmación de que Azure recibió correctamente el mensaje
static void SendConfirmationCallback (IOTHUB_CLIENT_CONFIRMATION_RESULT result) {
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
    Serial.println ("Send Confirmation Callback finished.");
  }
}

// Se recibió un mensaje de Azure
static void MessageCallback (const char* payLoad, int size) {
  Serial.println ("Message callback:");
  Serial.println (payLoad);
}

// Se ejecuta si hay actividad en el device twin
static void DeviceTwinCallback (DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size) {
  char *temp = (char *) malloc (size + 1);
  if (temp == NULL) {
    return;
  }
  memcpy (temp, payLoad, size);
  temp [size] = '\0';
  // Imprime el ensaje del device twin 
  Serial.println ("Device twin callback active");
  Serial.println (temp);
  free (temp);
}

// Se ejecuta cuando se recibe un mensaje de IoT Central. Se usa para ejecutar los comandos de la nube
static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size) {
  LogInfo ("Try to invoke method %s", methodName);                           // methodName es el nombre del comando enviado desde IoT Central.
  const char *responseMessage = "\"Successfully invoke device method\"";    // Se envia confirmación de recibido
  int result = 200;                                                         

  // If para detectar que comando se ejecuto y reaccionar a el
  if (strcmp (methodName, "start") == 0) {  // Activa el envío de telemetria
    LogInfo("Start sending telemetry");
    messageSending = true;
  } 
  
  else if (strcmp (methodName, "stop") == 0) {  //Detiene el envio de telemetria
    LogInfo("Stop sending telemetry");
    messageSending = false;
  }
  
  else if (strcmp (methodName, "echo") == 0) {  // Imprime un Echo
    Serial.println("echo command detected");
    Serial.print("Executed direct method payload: ");
    Serial.println((const char *)payload);
  }

  else if (strcmp (methodName, "toggleLED") == 0) { // Enciende el led en placa, para pruebas
    Serial.println ("Cambio de estado del LED");
    LED = !LED;
    digitalWrite (LEDPIN, LED);
  }

  else if (strcmp (methodName, "fan") == 0) { // Enciende el ventilador
    Serial.println ("Cambio de estado del ventilador");
    fan1 = !fan1;
    digitalWrite (fan_in1, fan1);
  }

  else if (strcmp (methodName, "light") == 0) { // Enciende la lampara
    Serial.println ("Cambio de estado en la lampara");
    light = !light;
    digitalWrite (light_pin, light);
  }

  else if (strcmp (methodName, "valve") == 0) { // Enciende la valvula por un periodo corto de tiempo
    Serial.println ("Se ha encendido el sistema de riego");
    valve = true;
    digitalWrite (valve_pin, valve);
    delay (2000);
    valve = false;
    digitalWrite (valve_pin, valve);
  }

  else if (strcmp (methodName, "automode") == 0) { // Cambio de modo automático
    Serial.println ("Cambio de modo de control");
    automode = !automode;
  }

  else { // No se encontró ningún comando
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage);
  *response = (unsigned char *) strdup (responseMessage);
  return result;
}


////////////////////  Función setup ////////////////////
void setup() {
  Serial.begin (9600);
  dht.begin ();

  // Definición de pines
  pinMode (LEDPIN, OUTPUT);
  pinMode (HUMIDITYSENSOR1, INPUT);
  pinMode (HUMIDITYSENSOR2, INPUT);  
  pinMode (LDR, INPUT);
  pinMode (fan_in1, OUTPUT);
  pinMode (fan_in2, OUTPUT);
  pinMode (valve_pin, OUTPUT);
  pinMode (light_pin, OUTPUT);

  // Inicia configuración de dispositivo
  digitalWrite (LEDPIN, HIGH);
  Serial.println ("ESP32 - Invernadero");
  Serial.println ("Initializing...");
  
  // Se hacen las conexiones
  setup_connections ();

  // Iniciamos las funciones Callback para MQTT. Las suscripciones están en la libreria Esp32MQTTClient
  Esp32MQTTClient_SetSendConfirmationCallback (SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback (MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback (DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback (DeviceMethodCallback);
  Serial.println ("Start sending events.");
  send_interval_ms = millis();

  digitalWrite(LEDPIN, LOW);
}


////////////////////  Función loop ////////////////////
void loop() {
  if (hasWifi && hasIoTHub) {
    if (messageSending && (int)(millis() - send_interval_ms) >= INTERVAL) {  

      // Leemos estado de los sensores
      float temperature = dht.readTemperature();
      float air_humidity = dht.readHumidity();
      float heat = dht.computeHeatIndex (temperature, air_humidity, false);
      int gnd_humidity1 = map (analogRead (HUMIDITYSENSOR1), 0, 1023, 1, 100);
      int gnd_humidity2 = map (analogRead (HUMIDITYSENSOR2), 0, 1023, 1, 100);
      int illuminance = map (analogRead (LDR), 0, 1023, 1, 100);
      
      String fan_state;
      String light_state;
      String valve_state;
      String mode_state;


      // Se define el estado de los actuadores
      if (fan1){                           // Estdo del ventilador 
        fan_state = "On";
      } else {
        fan_state = "Off";
      }

      if (light){                         // Estdo de la lampara
        light_state = "On";
      } else {
        light_state = "Off";
      }

      if (valve){                         // Estdo de la válvula 
        valve_state = "On";
      } else {
        valve_state = "Off";
      }
 
      //Se comprueba si hay modo automático o manual
      if (automode){
        mode_state = "On";
        self_maintenance (temperature, air_humidity, heat, gnd_humidity1, gnd_humidity2, illuminance);
      } else {
        mode_state = "Off";
      }
    

      // Creación y envio de mensaje a la nube
      char messagePayload [MESSAGE_MAX_LEN];
      snprintf (messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, temperature, air_humidity, heat, gnd_humidity1, gnd_humidity2, illuminance, fan_state, light_state, valve_state, mode_state);
      Serial.println (messagePayload);
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate (messagePayload, MESSAGE);
      Esp32MQTTClient_SendEventInstance (message);
      send_interval_ms = millis ();

    } else {
      Esp32MQTTClient_Check ();                                                                        
    }
  } else {
    setup_connections (); // Reconectar en caso de caida
  }
  delay(10);
}