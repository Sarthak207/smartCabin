//Smart Cabin Environment Management system

#include "DHT.h"
#include "WiFi.h"
#include "FirebaseESP32.h"
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

#define FIREBASE_HOST "your-app.firebaseio.com"
#define FIREBASE_AUTH "your-API-key"

FirebaseData firebaseData;             //Class and instance defined
FirebaseAuth auth;
FirebaseConfig Config;
FirebaseData streamData;

#define DHTPIN 25
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define WIFI_SSID "Your-WiFi-SSID"              //Wifi host name 
#define WIFI_PASSWORD "Password"               //password for host wifi

#define buzzer 32
#define ir 26
#define fanpin 14
#define fat 35
int t = 0;

PulseOximeter pox;

uint32_t tsLastReport = 0;

void setup() {
  Serial.begin(115200);

  pinMode(ir, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(fat,INPUT);

  ledcAttach(fanpin, 5000, 8);    // Attach fanpin to PWM channel

  Serial.println(F("DHTxx test!"));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                 //Wifi initialisation is being done here
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi");

  dht.begin();

  Config.host = FIREBASE_HOST;
  Config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&Config, nullptr);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase and DHT initialized");
  //Start a stream to listen to changes in fan_speed
  if (!Firebase.beginStream(streamData, "/HolisticCabin/fan_speed")) {
    Serial.println("Could not begin stream for fan_speed");
    Serial.println(streamData.errorReason());
  } else {
    Serial.println("Stream started for fan_speed");
  }

  // Serial.print("Initializing pulse oximeter..");
  // if (!pox.begin()) {
  //   Serial.println("FAILED");
  //   for (;;)
  //     ;
  // } else {
  //   Serial.println("SUCCESS");
  // }
  // pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  delay(1000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float f = dht.readTemperature(true);
  t = detect(t);
  //pulse();

  if (isnan(humidity) || isnan(temperature) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float hif = dht.computeHeatIndex(f, humidity);
  float hic = dht.computeHeatIndex(temperature, humidity, false);

  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperature);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  
  //send temperature and humidity data to the firebase 
  if (Firebase.setFloat(firebaseData, "/HolisticCabin/Temperature", temperature)) {
    Serial.println("Temperature sent to Firebase");
  } else {
    Serial.println("Failed to send temperature");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setFloat(firebaseData, "/HolisticCabin/Humidity", humidity)) {
    Serial.println("Humidity sent to Firebase");
  } else {
    Serial.println("Failed to send humidity");
    Serial.println(firebaseData.errorReason());
  }


  if (Firebase.getInt(firebaseData, "/HolisticCabin/fan_speed")) {
    int fanSpeed = firebaseData.intData();                                          //listen to the fan_speed from firebase
    ledcWrite(fanpin, map(fanSpeed, 0, 100, 0, 255));
    Serial.print("Fan Speed: ");
    Serial.println(fanSpeed);
  } else {
    Serial.print("Error getting data: ");
    Serial.println(firebaseData.errorReason());
  }
  t = detect(t);
  //pulse();
  delay(3000);  // Update every 5 seconds
}

int detect(int t) {                                                              //Detecing fatigueness using buzzer.Buzzer will start 
  int data = digitalRead(ir);    
  if(data!=HIGH){
    Serial.println("Object Closure!Stay Safe!");
  }
  int data1 = digitalRead(fat);                                             //buzzing as soon as driver feel sleepy.
  if ((t>=3) && (data1 == HIGH)) {
    digitalWrite(buzzer, HIGH);
    t = t + 1;
    delay(500);
  } else if ((data1 == HIGH && t < 3)) {
    digitalWrite(buzzer, LOW);
    t = t + 1;
    delay(500);
  } else {
    t=0;
    digitalWrite(buzzer, LOW);
  }
  return t;
}


void onBeatDetected() {
  Serial.println("Beat!");
}
void pulse() {
  // Make sure to call update as fast as possible
  pox.update();

  // Asynchronously dump heart rate and oxidation levels to the serial
  // For both, a value of 0 means "invalid"
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("Heart rate:");
    Serial.print(pox.getHeartRate());
    Serial.print("bpm / SpO2:");
    Serial.print(pox.getSpO2());
    Serial.println("%");
    tsLastReport = millis();
  }
}
