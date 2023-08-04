#include <Arduino.h>
#include <OneWire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <hx711.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#define MSG_BUFFER_SIZE	(50)
#define ONE_WIRE_BUS 0
#define DOUT0 1
#define CLK0 2
#define DOUT1 6
#define CLK1 7
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
HX711 scale0(DOUT0, CLK0);
HX711 scale1(DOUT1, CLK1);
float weight0, weight1, total;
float calibration_factor0 = -109810; // Must be fix
float calibration_factor1 = -110330; // Must be fix
uint8_t GPIO_Pin = 9;
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Pass our oneWire reference to Dallas Temperature.
const char* ssid = "QUANGHAO";
const char* password = "12345.tqh";
const char* mqtt_server = "broker.mqtt-dashboard.com";
int count = 0;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(18, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(18, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void IntCallback() {
  scale0.set_scale();
  scale0.tare(); //Reset the scale to 0
  scale1.set_scale();
  scale1.tare(); //Reset the scale to 0
}
void setup(void)
{
  pinMode(19, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  sensors.begin();
  pinMode(GPIO_Pin, INPUT_PULLUP); 
  // attachInterrupt(digitalPinToInterrupt(GPIO_Pin), IntCallback, FALLING);
  Wire.begin(10, 8);
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  scale0.set_scale();
  scale0.tare();
  scale1.set_scale();
  scale1.tare();
  long zero_factor0 = scale0.read_average(); //Get a baseline reading
  long zero_factor1 = scale1.read_average();
  lcd.begin(20,4);
  lcd.setCursor(1,1);
  lcd.print("Weighing Scale");
  delay(3000);
  lcd.clear();

}


void loop(void) {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  digitalWrite(19, LOW);
  scale0.set_scale(calibration_factor0); //Adjust to this calibration factor
  weight0 = scale0.get_units(5); 
  scale1.set_scale(calibration_factor1); //Adjust to this calibration factor
  weight1 = scale1.get_units(5); 
  total = weight0+weight1;
  sensors.requestTemperatures(); 
  float temp;
  temp = sensors.getTempCByIndex(0);
   if ( digitalRead(GPIO_Pin) == LOW)
{
  scale0.set_scale();
  scale0.tare(); //Reset the scale to 0
  scale1.set_scale();
  scale1.tare(); //Reset the scale to 0
}
 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weight0: ");
  lcd.print(weight0);
  lcd.print(" KG");
  lcd.setCursor(0, 1);
  lcd.print("Weight1: ");
  lcd.print(weight1);
  lcd.print(" KG  ");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Total: ");
  lcd.print(total);
  lcd.print(" KG  ");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("*C");
  delay(2000);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long now = millis(); 
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "%.2f", weight0);
    client.publish("QUANGHAOTS1/W0", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%.2f", weight1);
    client.publish("QUANGHAOTS1/W1", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%.2f", total);
    client.publish("QUANGHAOTS1/TT", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%.2f", temp);
    client.publish("QUANGHAOTS1/TEMP", msg);
  }
  digitalWrite(19, HIGH); 
}