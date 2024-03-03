#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSerial.h>

#include <EncoderStepCounter.h>
#include <string>

#define ENCODER_PIN1 4
#define ENCODER_PIN2 5
#define button 0
#define pot A0

// Initialize rotary encoder
EncoderStepCounter encoder(ENCODER_PIN1, ENCODER_PIN2);

unsigned long lastButtonPress = 0;
unsigned long lastPrint = 0;
int lastpos = 195;
int lastPotPos = analogRead(pot);

const char* ssid = "WWN";
const char* password = "2163East6595SouthSlC84121";

IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);
IPAddress local_IP(192, 168, 0, 98);

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

// LED variables
const int redPin = 14;
const int bluePin = 13;
const int greenPin = 12;

int red = 195;
int blue = 195;
int green = 195;

bool lightIsOn = true;

void onOTAStart() {
}

void onOTAProgress(size_t current, size_t final) {
}

void onOTAEnd(bool success) {
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(size_t i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}

void HSVtoRGB(float H, float S,float V) {
    if(H>360 || H<0 || S>100 || S<0 || V>100 || V<0){
        return;
    }
    float s = S/100;
    float v = V/100;
    float C = s*v;
    float X = C*(1-abs(fmod(H/60.0, 2)-1));
    float m = v-C;
    float r,g,b;
    if(H >= 0 && H < 60){
        r = C,g = X,b = 0;
    }
    else if(H >= 60 && H < 120){
        r = X,g = C,b = 0;
    }
    else if(H >= 120 && H < 180){
        r = 0,g = C,b = X;
    }
    else if(H >= 180 && H < 240){
        r = 0,g = X,b = C;
    }
    else if(H >= 240 && H < 300){
        r = X,g = 0,b = C;
    }
    else{
        r = C,g = 0,b = X;
    }
    red = (r+m)*255;
    green = (g+m)*255;
    blue = (b+m)*255;
}

// Call tick on every change interrupt
void interrupt() {
  encoder.tick();
}


void setup(void) {
  Serial.begin(115200);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Wifi Connected! IP Address: " + WiFi.localIP().toString());

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  // Initialize WebSerial
  WebSerial.begin(&server);
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);

  // Start server
  server.begin();

  // LED pin Setup
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Initialize Encoder Button Pin
  pinMode(button, INPUT_PULLUP);

  // Initialize rotary encoder
  encoder.begin();
}

// Function to set the RGB color using PWM
void setColor(int red, int green, int blue) {
  analogWrite(redPin, 255 - red);      // Invert the logic for common anode
  analogWrite(greenPin, 255 - green);  // Invert the logic for common anode
  analogWrite(bluePin, 255 - blue);    // Invert the logic for common anode
}

void loop(void) {
    ElegantOTA.loop();

    encoder.tick();
    // read encoder position and adjust LED color if necessary
    signed char pos = encoder.getPosition() / 3;
    // Read Potentiometer position:
    int potPos = map(analogRead(pot), 0, 1024, 0, 100);
    if (lightIsOn && (pos != lastpos || potPos != lastPotPos)) {
      lastpos = pos;
      lastPotPos = potPos;
      int colorValue = map(lastpos, -42, 42, 60, 330);
      HSVtoRGB(colorValue, 100.0, lastPotPos);
      setColor(red, green, blue);
    }

    if(millis() - lastPrint > 300) {
      WebSerial.printf("P-Pos: %d   E-Pos: %d   R: %d   G: %d   B: %d\n", lastPotPos, lastpos, red, green, blue);
      lastPrint = millis();
    }

    int btnState = digitalRead(button);
    if (btnState == LOW) {
        if (millis() - lastButtonPress > 50) {
          if (lightIsOn) {
              HSVtoRGB(0.0, 0.0, 0.0);
              setColor(red, green, blue);
              lightIsOn = false;
          }
          else {
              HSVtoRGB(map(lastpos, -42, 42, 60, 330), 100.0, 100.0);
              setColor(red, green, blue);
              lightIsOn = true;
          }
        }
        lastButtonPress = millis();
    }
}
