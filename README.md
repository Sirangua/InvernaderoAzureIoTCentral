# InvernaderoAzureIoTCentral
Este es el código utilizado en una ESP32 para un invernadero conectado a Azure IoT Central. El framework utilizado es Arduino.

Como conexiones fisicas se tiene dos sensores de humedad de suelo, un sensor de luminosidad (fotoresistencia) y un sensore de temperatura y humedad. Para la correcta lectura de estos sensores es necesario usar un divisor de voltaje para ajustar la salida de 5V a 3.3V que recibe la tarjeta.

Los actuadores utilizados fueron: un ventilador USB con alimentación externa controlado con un puente H, una electrovalvula con alimentación externa de 12V controlada por un relevador y una lampara LED conectada directamenbte a la corriente y controlada por un relvador. 

## Documentos de referencia
- Documentación de Azure IoT Central. (s. f.). Developer tools, technical documentation and coding 
examples | Microsoft Docs. https://docs.microsoft.com/es-es/azure/iot-central/
- Device connectivity in Azure IoT Central. (s. f.). Developer tools, technical documentation and coding 
examples | Microsoft Docs. https://docs.microsoft.com/en-us/azure/iot-central/core/concepts-get-connected
- GitHub - Azure/dps-keygen: Helper tool to deal with the device keys. (s. f.). GitHub. 
https://github.com/Azure/dps-keygen
