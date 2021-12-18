#ifndef IOT_CONFIGS_H  
#define IOT_CONFIGS_H

// Credenciales Wifi
#define IOT_CONFIG_WIFI_SSID  "Tu SSID" 
#define IOT_CONFIG_WIFI_PASSWORD  "Tu contraseña" 

/*- Para generar una string conection de IoT Central es necesario obtener primero un Device Key
    El cual se puede obtener con el siguiente comando en AzureCLI:  
    "az iot central device compute-device-key --primary-key <enrollment group primary key> --device-id <device ID>"
    Documentación completa en: https://docs.microsoft.com/en-us/azure/iot-central/core/concepts-get-connected

  - Despues se genera una Conection string usando dps-keygen. Comando para instalarlo: 
    "npm i -g dps-keygen"
    Una vez instalado se usa el siguiente comando:
    "dps-keygen -di:dev1 -dk:devicekeyhere -si:scopeidhere"
    Documentación completa en: https://github.com/Azure/dps-keygen

  - Advertencia: Esta no es la manera más segura de conectarse a IoT Central, sin embargo es una forma 
    muy practica*/
// Conection String
#define DEVICE_CONNECTION_STRING  "Tu String Conection" 

#endif 