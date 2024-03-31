#include <Servo.h>
#include <MQ135.h>
#include <DHT.h>
#include <WiFiManager.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
/* Configuration of NTP */
#define MY_NTP_SERVER "at.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#include <time.h>  // for time() ctime()


LiquidCrystal_I2C lcd(0x27, 20, 4);  //SCL=D1 SDA=D2
WiFiManager wifiManager;

#define API_KEY "AIzaSyA15QW_8OQrNUDo2J1_GZY6rJG-7cf9H_M"
#define DATABASE_URL "https://poultry-management-syste-a2d22-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

#define DHTPIN D4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#define PIN_MQ135 A0
MQ135 mq135_sensor(PIN_MQ135);
Servo myServo;

char buffer[30];
char datas[30];
uint8_t MINTEMP = 0;
uint8_t MAXTEMP = 0;
uint8_t WATERSTATUS = 0;
uint8_t FEEDSTATUS = 0;
uint8_t ROTATION = 0;
uint8_t TIMES = 0;
uint8_t ISAUTO = 0;
uint8_t FAN = 0;
uint8_t BULB = 0;
uint8_t WATERTIME = 0;
uint8_t FANTIME = 0;
uint8_t LIGHTTIME = 0;
uint8_t CARBONVALUE = 0;
uint8_t CARBONTIME = 0;
int bulbstat = 0;
int fanstat = 0;
int waterstat = 0;
int feedstat = 0;
int carbonstat = 0;
time_t now;  // this are the seconds since Epoch (1970) - UTC
tm tm;       // the structure tm holds time information in a more convenient way
char dates[30];
char times[30];
char act[60];
void showTime() {
  if (WiFi.status() == WL_CONNECTED) {
    // The broiler was fed.-1970/1/1-8:0:34
    time(&now);              // read the current time
    localtime_r(&now, &tm);  // update the structure tm with the current time
    sprintf(dates, "-%d/%d/%d-%d:%d:%d", (tm.tm_year + 1900), (tm.tm_mon + 1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    sprintf(times, "%d:%d:%d", tm.tm_hour, tm.tm_min, tm.tm_sec);

    Serial.println(dates);
    Serial.println();
  }
}


void addActivity(String msg) {
  if (Firebase.RTDB.pushString(&fbdo, "ActivityLog", msg)) {
    Serial.println("Added Activity " + msg);

  } else {
    Serial.println("Activity Error: " + fbdo.errorReason());
  }
}

void GPIOPIN() {
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  lcd.begin();
  dht.begin();
  myServo.attach(D3);
  delay(500);
}
void WELCOME() {
  lcd.setCursor(6, 0);
  lcd.print("POULTRY");
  lcd.setCursor(1, 1);
  lcd.print("MONITORING SYSTEM");
  lcd.setCursor(3, 2);
  lcd.print("USING ARDUINO &");
  lcd.setCursor(1, 3);
  lcd.print("ANDROID TECHNOLOGY");
  delay(5000);
  lcd.clear();
  lcd.setCursor(9, 1);
  lcd.print("BY");
  lcd.setCursor(5, 2);
  lcd.print("JOLLYCODE");
}
void WiFiConfig() {
  wifiManager.autoConnect("Jollycode Connection");
  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.println("Connected");
  delay(2000);
  lcd.clear();
}
void FirebaseConfiguration() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
void FirebaseData(float temperature, float carbon) {
  Serial.println("-------------------------------------------------");

  if (Firebase.RTDB.getInt(&fbdo, "Settings/Carbon/Min")) {
    CARBONVALUE = fbdo.intData();
    if (Firebase.RTDB.getInt(&fbdo, "Settings/Carbon/Time")) {
      CARBONTIME = fbdo.intData();
      if (carbon > CARBONVALUE && FAN == 0) {
        //FAN for CARBON CONFIG
        carbonstat = carbonstat + 1;
        if (carbonstat == 1) {
          sprintf(act, "%s %s", "C02 gets High", dates);
          addActivity(act);
        }
        digitalWrite(D6, HIGH);
        delay(10);
        delay(CARBONTIME * 1000);
        digitalWrite(D6, LOW);
      } else {
        carbonstat = 0;
        digitalWrite(D6, LOW);
        delay(10);
      }
    }
  }


  if (Firebase.RTDB.getInt(&fbdo, "Settings/isAutomatic")) {
    ISAUTO = fbdo.intData();
    if (Firebase.RTDB.getInt(&fbdo, "Settings/Light/Time")) {
      LIGHTTIME = fbdo.intData();
      if (Firebase.RTDB.getInt(&fbdo, "Settings/Fan/Time")) {
        FANTIME = fbdo.intData();

        if (ISAUTO == 0) {
          //FAN
          if (Firebase.RTDB.getInt(&fbdo, "Status/FAN")) {
            FAN = fbdo.intData();

            if (FAN == 1) {
              //FAN CONFIG
              fanstat = fanstat + 1;
              if (fanstat == 1) {
                sprintf(act, "%s %s", "Ventilation turns on", dates);
                addActivity(act);
              }
              digitalWrite(D6, HIGH);
              delay(10);
            } else {
              fanstat = 0;
              Firebase.RTDB.setInt(&fbdo, "Status/FAN", 0);
              digitalWrite(D6, LOW);
              delay(10);
            }
          }

          //BULB
          if (Firebase.RTDB.getInt(&fbdo, "Status/LIGHT")) {
            BULB = fbdo.intData();
            //TURN OF AND ON THE FAN RELAYS
            if (BULB == 1) {
              //FAN CONFIG
              bulbstat = bulbstat + 1;
              if (bulbstat == 1) {
                sprintf(act, "%s %s", "Heat lamps turn on", dates);
                addActivity(act);
              }
              digitalWrite(D5, HIGH);
              delay(10);
            } else {
              bulbstat = 0;
              Firebase.RTDB.setInt(&fbdo, "Status/LIGHT", 0);
              digitalWrite(D5, LOW);
              delay(10);
            }
          }
          //GET WATER STAT AND TIME
          if (Firebase.RTDB.getInt(&fbdo, "Status/WATER")) {
            WATERSTATUS = fbdo.intData();
            if (Firebase.RTDB.getInt(&fbdo, "Settings/Water/Time")) {
              WATERTIME = fbdo.intData();

              //TURN OF AND ON THE WATER RELAYS
              if (WATERSTATUS == 1) {
                //WATER CONFIG
                waterstat = waterstat + 1;
                if (waterstat == 1) {
                  sprintf(act, "%s %s", "Water pump turns on", dates);
                  addActivity(act);
                }
                digitalWrite(D7, HIGH);
                delay(WATERTIME * 1000);
                digitalWrite(D7, LOW);
                Firebase.RTDB.setInt(&fbdo, "Status/WATER", 0);
                Serial.println("ON PUMP");
                delay(10);
              } else {
                waterstat = 0;
                digitalWrite(D7, LOW);
                delay(100);
                Serial.println("OFF PUMP");
              }
            }
          }
          if (Firebase.RTDB.getInt(&fbdo, "Status/FEED")) {
            FEEDSTATUS = fbdo.intData();
            if (Firebase.RTDB.getInt(&fbdo, "Settings/Servo/Time")) {
              TIMES = fbdo.intData();

              //TURN ON AND OFF THE SERVO MOTOR
              if (FEEDSTATUS == 1) {
                if (Firebase.RTDB.getInt(&fbdo, "Settings/Servo/Rotate")) {
                  ROTATION = fbdo.intData();
                  //SERVO CONFIG
                  feedstat = feedstat + 1;
                  if (feedstat == 1) {
                    sprintf(act, "%s %s", "The broiler was fed", dates);
                    addActivity(act);
                  }
                  myServo.write(ROTATION);
                  delay(TIMES * 1000);
                  myServo.write(0);
                  Firebase.RTDB.setInt(&fbdo, "Status/FEED", 0);
                  Serial.println("ON FEED");
                  delay(10);
                } else {
                  feedstat = 0;
                  myServo.write(0);
                  Serial.println("OFF FEED");
                  delay(10);
                }
              }
            }
          }

        } else if (ISAUTO == 1) {
          if (Firebase.RTDB.getInt(&fbdo, "Settings/Temperature/Min")) {
            MINTEMP = fbdo.intData();
            if (Firebase.RTDB.getInt(&fbdo, "Settings/Temperature/Max")) {
              MAXTEMP = fbdo.intData();

              if (temperature < MINTEMP) {
                //ON ANG LIGHT
                digitalWrite(D5, HIGH);
                delay(LIGHTTIME * 1000);
                digitalWrite(D5, LOW);
                Firebase.RTDB.setInt(&fbdo, "Settings/Light/Time", 0);
              } else if (temperature > MAXTEMP) {
                digitalWrite(D6, HIGH);
                delay(FANTIME * 1000);
                digitalWrite(D6, LOW);
                Firebase.RTDB.setInt(&fbdo, "Settings/Fan/Time", 0);
              } else {
                digitalWrite(D5, LOW);
                digitalWrite(D6, LOW);
              }
            }
          }
          int time1 = strcmp(times, "06:00:00");
          int time2 = strcmp(times, "12:00:00");
          int time3 = strcmp(times, "18:00:00");
          if (time1 == 0 || time2 == 0 || time3 == 0) {  //FOR FEED TIME AUTO
            if (Firebase.RTDB.getInt(&fbdo, "Settings/Servo/Time")) {
              TIMES = fbdo.intData();
              if (Firebase.RTDB.getInt(&fbdo, "Settings/Servo/Rotate")) {
                ROTATION = fbdo.intData();
                //SERVO CONFIG
                sprintf(act, "%s %s", "The broiler was fed", dates);
                addActivity(act);
                myServo.write(ROTATION);
                delay(TIMES * 1000);
                myServo.write(0);
                Firebase.RTDB.setInt(&fbdo, "Status/FEED", 0);
                Serial.println("ON FEED");
                delay(10);
              }
            }
          }

          if (time1 == 0 || time3 == 0) {
            //GET WATER STAT AND TIME
            if (Firebase.RTDB.getInt(&fbdo, "Settings/Water/Time")) {
              WATERTIME = fbdo.intData();
              //WATER CONFIG
              sprintf(act, "%s %s", "Water pump turns on", dates);
              addActivity(act);
              digitalWrite(D7, HIGH);
              delay(WATERTIME * 1000);
              digitalWrite(D7, LOW);
              Firebase.RTDB.setInt(&fbdo, "Status/WATER", 0);
              Serial.println("ON PUMP");
              delay(10);
            }
          }
        }
      }
    }
  }
}




void setup() {
  Serial.begin(9600);
  configTime(MY_TZ, MY_NTP_SERVER);  // --> Here is the IMPORTANT ONE LINER needed in your sketch!
  GPIOPIN();
  WELCOME();
  WiFiConfig();
  FirebaseConfiguration();
}

void loop() {
  showTime();
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    //STORE DATA TO RTDB
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float correctedPPM = mq135_sensor.getPPM();

    lcd.setCursor(0, 0);
    sprintf(datas, "C02: %d PPM", int(correctedPPM));
    lcd.print(datas);
    lcd.setCursor(0, 1);
    sprintf(datas, "TEMP: %d C", int(temperature));
    lcd.print(datas);
    lcd.setCursor(0, 2);
    sprintf(datas, "HUMID: %d%%", int(humidity));
    lcd.print(datas);

    if (Firebase.RTDB.setFloat(&fbdo, "temperature", temperature)) {
      Serial.print("Success Storing Temp: ");
      Serial.println(temperature);
    } else {
      Serial.println("Error Temp: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "humidity", humidity)) {
      Serial.print("Success Storing humidity: ");
      Serial.println(humidity);
    } else {
      Serial.println("Error humidity: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "carbonDioxide", correctedPPM)) {
      Serial.print("Success Storing carbonDioxide: ");
      Serial.println(correctedPPM);
    } else {
      Serial.println("Error carbonDioxide: " + fbdo.errorReason());
    }

    //READ DATA
    FirebaseData(temperature, correctedPPM);
  }
}
