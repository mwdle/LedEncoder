# LedEncoder
Allows for gradient color control of a Common Anode RGB LED diode using a 5 pin 360° rotary encoder.    <br><br>    
<p align="center">
  <img src="demo.gif" alt="animated" />
</p>        

# Specifications and Pinout
All builds were created and tested using the PlatformIO IDE extension for VSCode and Espressif ESP8266 12-E board, mileage may vary on other boards and IDE's.
The following libraries are required:
* [Elegant OTA](https://github.com/ayushsharma82/ElegantOTA)
* [WebSerial](https://github.com/ayushsharma82/WebSerial)
* [EncoderStepCounter](https://github.com/M-Reimer/EncoderStepCounter)
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
## Pinout Info:
* ENCODER_PIN1 is the 'a' pin on your rotary encoder. This program uses pin 4, which corresponds to pin D2 on the ESP8266 12-E.
* ENCODER_PIN2 is the 'b' pin on your rotary encoder. This program uses pin 5, which corresponds to pin D1 on the ESP8266 12-E.
* button is the button pin on your rotary encoder. This program uses pin 0, which corresponds to pin D3 on the ESP8266 12-E.
* redPin is the red pin on your _Common Anode_ RGB LED diode. This program uses pin 14, which corresponds to pin D5 on the ESP8266 12-E.
* greenPin is the red pin on your _Common Anode_ RGB LED diode. This program uses pin 12, which corresponds to pin D6 on the ESP8266 12-E.
* bluePin is the red pin on your _Common Anode_ RGB LED diode. This program uses pin 13, which corresponds to pin D7 on the ESP8266 12-E.
## Network Info:
If you want WebSerial and ElegantOTA Connectivity, change the ssid and password variables to match your network settings. You may also need to change the subnet, gateway, and local_ip variables.
