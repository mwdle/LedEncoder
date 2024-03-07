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

#define encoderPin1 D2
#define encoderPin2 D1
#define button D3
#define pot A0
#define errorLed D0

using std::smatch;
using std::regex;
using std::regex_match;
using std::string;

// Internet related setup:
const char* ssid = "";
const char* password = "";

IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);
IPAddress ip(192, 168, 0, 98);

AsyncWebServer server(80);

// Initialize rotary encoder
EncoderStepCounter encoder(encoderPin1, encoderPin2);

unsigned long lastButtonPress = 0;
unsigned long lastPotPoll = 0;
unsigned long lastWifiCheck = 0;
int lastEncoderPos = 0;
int lastPotPos = 100;

// LED variables
const int redPin1 = D5;
const int bluePin1 = D7;
const int greenPin1 = D6;

bool lightIsOn = true;
volatile bool buttonClicked = false;

struct RGB {
  int r, g, b;
  RGB(int _r, int _g, int _b) {
    r = _r;
    g = _g;
    b = _b;
  }
};

RGB currentColor(255, 0, 0);
RGB off(0, 0, 0);

// Converts HSV into RGB and returns the result.
RGB hsvToRgb(double H, double S, double V) {
  if(H>360 || H<0 || S>100 || S<0 || V>100 || V<0){
      return {0, 0, 0};
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
  return {(int)((r+m)*255), (int)((g+m)*255), (int)((b+m)*255)};
}

// Set given rgb colors to the given rgb pins.
void setColor(int red, int green, int blue, int rpin, int gpin, int bpin) {
  analogWrite(rpin, 255 - red);      // Invert the logic for common anode
  analogWrite(gpin, 255 - green);  // Invert the logic for common anode
  analogWrite(bpin, 255 - blue);    // Invert the logic for common anode
}

// Handles encoder button presses.
void IRAM_ATTR buttonPressed() {
  if (millis() - lastButtonPress > 500) {
    buttonClicked = true;
    lastButtonPress = millis();
  }
}

// Handles incoming commands from WebSerial.
// Supports "/color <r> <g> <b>" for changing LED colors.
// Supports "/restart" for restarting the ESP device.
// Littered with ESP.wdtFeed() (resets the software watchdog timer), preventing unexpected resets during delays handling many messages in succession.
void recvMsg(uint8_t *data, size_t len) {
  string msg;
  msg.reserve(len);
  for(size_t i=0; i < len; i++){
    msg += char(data[i]);
    ESP.wdtFeed();
  }
  // Regex for /color <r> <g> <b> and /restart commands available via WebSerial.
  regex colorCmd(R"(^/color (\d{1,3}) (\d{1,3}) (\d{1,3})$)");
  regex restartCmd(R"(/restart)");
  regex stateCmd(R"(/state)");
  smatch match;
  ESP.wdtFeed();
  // If the data matches one of the predefined commands, handle it accordingly
  if (regex_match(msg, match, colorCmd)) {
    ESP.wdtFeed();
    currentColor.r = constrain(stoi(match[1]), 0, 255); 
    currentColor.g = constrain(stoi(match[2]), 0, 255); 
    currentColor.b = constrain(stoi(match[3]), 0, 255);
    ESP.wdtFeed();
    setColor(currentColor.r, currentColor.g, currentColor.b, redPin1, greenPin1, bluePin1);
    WebSerial.printf("Color command received. The new color: (%d, %d, %d) has been set.\n", currentColor.r, currentColor.g, currentColor.b);
  }
  else if (regex_match(msg, match, restartCmd)) {
    WebSerial.println("Restart command received. Restarting . . . \n");
    ESP.restart();
  }  
  else if (regex_match(msg, match, stateCmd)) WebSerial.printf("The light is%s on.\n", lightIsOn?"":" not");
  else { WebSerial.print("New message: "); WebSerial.println(msg.c_str()); }
  ESP.wdtFeed();
}

void setup(void) {
  // Initialize Error LED Pin
  pinMode(errorLed, OUTPUT); 

  Serial.begin(115200);

  // Initialize Wi-Fi
  WiFi.config(ip, gateway, subnet, gateway);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(errorLed, LOW);   // Turn on the red onboard LED by making the voltage LOW
    delay(500);                    
    digitalWrite(errorLed, HIGH);  // Turn off the red onboard LED by making the voltage HIGH
    delay(800);
  }
  digitalWrite(errorLed, HIGH);
  Serial.println("Wifi Connected! IP Address: " + WiFi.localIP().toString());

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);

  // Initialize WebSerial
  WebSerial.begin(&server);
  // Attach incoming message callback
  WebSerial.msgCallback(recvMsg);

  // Start server
  server.begin();

  // LED pin Setup
  pinMode(redPin1, OUTPUT);
  pinMode(greenPin1, OUTPUT);
  pinMode(bluePin1, OUTPUT);

  // Initialize Encoder Button Pin
  pinMode(button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), buttonPressed, FALLING);

  // Initialize rotary encoder to lowest position (-128 == red LED color)
  encoder.begin();
  encoder.setPosition(-128);

  setColor(currentColor.r, currentColor.g, currentColor.b, redPin1, greenPin1, bluePin1);
}

void loop(void) {
  ElegantOTA.loop();

  if(millis() - lastWifiCheck > 3000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) digitalWrite(errorLed, LOW); // Turn on red onboard LED by setting voltage LOW            
    else digitalWrite(errorLed, HIGH); // Turn off red onboard LED by setting voltage HIGH
  } 

  if (buttonClicked) {
    if (lightIsOn) lightIsOn = false;
    else
    { 
      lightIsOn = true;
      lastEncoderPos += 1;
    }
    buttonClicked = false;
  }

  // Check Potentiometer position (my potentiometer is finicky so I constrain its minimum to 23 to ensure its minimum state will completely turn off the LED.)
  int potPos = lastPotPos;
  if (millis() - lastPotPoll > 50) {
    lastPotPoll = millis();
    potPos = map(constrain(analogRead(pot), 23, 1023), 23, 1023, 0, 100);
  }

  // Read encoder position and adjust LED color if necessary
  if (lightIsOn) {
      encoder.tick();
      signed char pos = encoder.getPosition();
    if (pos != lastEncoderPos || potPos != lastPotPos) {
      lastEncoderPos = pos; lastPotPos = potPos;
      currentColor = hsvToRgb(map(lastEncoderPos, -128, 127, 0, 360), 100.0, lastPotPos);
      setColor(currentColor.r, currentColor.g, currentColor.b, redPin1, greenPin1, bluePin1);
      WebSerial.printf("R: %d   G: %d   B: %d\n", currentColor.r, currentColor.g, currentColor.b);
    }
  }
  else {
    if (!(currentColor.r == off.r && currentColor.g == off.g && currentColor.b == off.b)) {
      currentColor.r = off.r;
      currentColor.g = off.g;
      currentColor.b = off.b;
      setColor(currentColor.r, currentColor.g, currentColor.b, redPin1, greenPin1, bluePin1);
    }
  }
}