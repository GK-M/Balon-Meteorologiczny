#include <Wire.h> 
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <RTClib.h>
#include <GY521.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS_PIN 5  // Pin CS karty SD
#define BMP280_I2C_ADDRESS 0x76
#define MQ2_ANALOG_PIN 34
#define MQ2_DIGITAL_PIN 25
#define SEA_LEVEL_PRESSURE_HPA 972

unsigned long lastSensorRead = 0;
unsigned long lastSmsSent = 0;
const unsigned long sensorInterval = 10000; // 10 sekund
const unsigned long smsInterval = 600000; // 15 minut 


GY521 sensor(0x69);
Adafruit_BMP280 bmp;
Adafruit_AHTX0 aht;
RTC_DS3231 rtc;
TinyGPSPlus gps;
HardwareSerial sim800l(1);
HardwareSerial mySerial(0);
String currentFile;

  void sendLocationSMS(float lat, float lon) {
      sim800l.println("AT+CMGF=1"); // Tryb SMS
      delay(1000);
      sim800l.println("AT+CMGS=\"+48736044423\"");
      delay(1000);
      sim800l.print("Lokalizacja: https://www.google.com/maps?q=");
      sim800l.print(lat, 6);
      sim800l.print(",");
      sim800l.print(lon, 6);
      delay(500);
      sim800l.write(26); // Kod zakończenia wiadomości
      delay(5000);
    }

void setup() {
  //Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 1, 3); // RX=1, TX=3 dla GPS
  sim800l.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17 dla SIM800L
  Wire.begin();
  delay(100);

  // Inicjalizacja SIM800L
  sim800l.println("AT");
  delay(1000);
  sim800l.println("AT+CMGF=1"); // Ustawienie trybu SMS
  delay(1000);
  

  // Inicjalizacja GY521 (MPU6050)
  while (!sensor.wakeup()) {
    Serial.print(millis());
    Serial.println(" Nie znaleziono GY521");
  }
  sensor.setAccelSensitivity(2);
  sensor.setGyroSensitivity(2);
  sensor.setThrottle();

  // Inicjalizacja BMP280
  if (!bmp.begin(BMP280_I2C_ADDRESS)) {
    Serial.println("BMP280 nie wykryty!");
    while (1);
  }

  // Inicjalizacja AHT10
  if (!aht.begin()) {
    Serial.println("AHT10 nie wykryty!");
    while (1);
  }

  // Inicjalizacja RTC
  if (!rtc.begin()) {
    Serial.println("DS3231 nie wykryty!");
    while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inicjalizacja SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Błąd inicjalizacji karty SD!");
    while (1);
  }
  Serial.println("Karta SD gotowa");

  // Generowanie nagłówka
  int fileIndex = 1;
  do {
    currentFile = "/dane" + String(fileIndex) + ".txt";
    fileIndex++;
  } while (SD.exists(currentFile.c_str()));

  Serial.print("Tworzę plik: "); Serial.println(currentFile);

  // Tworzenie pliku z nagłówkiem
  File dataFile = SD.open(currentFile.c_str(), FILE_WRITE); // Tworzymy plik
  if (dataFile) {
    dataFile.println("RTC_Data\tRTC_Czas\tGPS_Lat\tGPS_Lon\t gps_wysokosc\tgps_speed\tgps_course\tgps_satelite\tgps_hdop(dokładność)\t GPS_Data\tGPS_Godzina\tAccel_X\tAccel_Y\tAccel_Z\tGyro_X\tGyro_Y\tGyro_Z\tTemp_MPU\tTemp_BMP\tPressure\tWysokosc_wyliczona\tTemp_AHT\tWilgotnosc\tMQ2_Analog\tMQ2_Digital");
    dataFile.close();
    Serial.println("Nagłówek zapisany!");
  } else {
    Serial.println("Błąd tworzenia pliku!");
  }
}

    void loop() {
    
    while (mySerial.available()) 
    {
    gps.encode(mySerial.read());
    }
    unsigned long currentMillis = millis();

    // Pobieranie danych z GPS - lokalizacja 
    float gps_lat = gps.location.isValid() ? gps.location.lat() : 0.0; // szerokość geograficzna
    float gps_lon = gps.location.isValid() ? gps.location.lng() : 0.0; // długość geograficzna
    float gps_altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0; // wysokość
    float gps_speed = gps.speed.isValid() ? gps.speed.kmph() : 0.0; // prędkość 
    float gps_course = gps.course.isValid() ? gps.course.deg() : 0.0; // kurs (azymut kompasu) 
  

    if (currentMillis - lastSmsSent >= smsInterval ) {
    lastSmsSent = currentMillis;
    sendLocationSMS(gps_lat, gps_lon);
  }

    // Ilość satelitów i dokładność pomiaru
    int gps_satelite = gps.satellites.isValid() ? gps.satellites.value() : 0; // ilość satelitów
    float gps_hdop = gps.hdop.isValid() ? gps.hdop.value() : 0.0; // dokładność pomiaru

    // <1.0 – bardzo dokładna pozycja
    // 1.0 – 2.0 – dobra dokładność
    // 2.0 – 5.0 – przeciętna dokładność
    // >5.0 – niska dokładność

    // GPS - data i godzina
    int GPS_godzina = gps.time.isValid() ? gps.time.hour()+1 : 0;
    int GPS_minuta = gps.time.isValid() ? gps.time.minute() : 0;
    int GPS_sekunda = gps.time.isValid() ? gps.time.second() : 0;

    int GPS_dzien = gps.date.isValid() ? gps.date.day() : 0;
    int GPS_miesiac = gps.date.isValid() ? gps.date.month() : 0;
    int GPS_rok = gps.date.isValid() ? gps.date.year() : 0;
    /*
  Serial.print(GPS_godzina);    
  Serial.print("/");            
  Serial.print(GPS_minuta);     
  Serial.print("/");            
  Serial.println(GPS_sekunda);  

  // Wyświetlanie daty GPS
  Serial.print(GPS_dzien);      
  Serial.print("/");            
  Serial.print(GPS_miesiac);    
  Serial.print("/");            
  Serial.println(GPS_rok);      
*/
   // Dane z MPU6050 GY-521
  sensor.read();
  float ax = sensor.getAccelX(); //Akcelerometr Przyśpieszenie we wszytskich trzech osiach XYZ
  float ay = sensor.getAccelY();
  float az = sensor.getAccelZ();
  float gx = sensor.getGyroX(); //Żyroskop prędkośc kątowa wokół osi XYZ
  float gy = sensor.getGyroY();
  float gz = sensor.getGyroZ();
  float temp_mpu = sensor.getTemperature();
  
  //Dane z BMP280
  float temp_bmp = bmp.readTemperature();
  float pressure_bmp = bmp.readPressure() / 100.0;
  float altitude_bmp = bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);
  
  //Dane AHT10
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);  // Pobierz pomiary
  
  float temp_AHT10 = temp.temperature;   // Odczyt temperatury
  float humidity_AHT10 = humidity.relative_humidity; // Odczyt wilgotności

  
  //Dane MQ135
  int mq2_analog = analogRead(MQ2_ANALOG_PIN);
  int mq2_digital = digitalRead(MQ2_DIGITAL_PIN);
  
  //Czas z RTC
  DateTime now = rtc.now();

  
  File dataFile = SD.open(currentFile.c_str(), FILE_APPEND); // FILE_APPEND dopisywanie danych, a nie podmiana
  if (dataFile) {
    
    dataFile.print(now.year()); dataFile.print(".");
    dataFile.print(now.month()); dataFile.print(".");
    dataFile.print(now.day()); dataFile.print("\t");

    dataFile.print(now.hour()); dataFile.print(".");
    dataFile.print(now.minute()); dataFile.print(".");
    dataFile.print(now.second()); dataFile.print("\t");

    dataFile.print(gps_lat, 6); dataFile.print("\t");
    dataFile.print(gps_lon, 6); dataFile.print("\t");
    dataFile.print(gps_altitude, 2); dataFile.print("\t");
    dataFile.print(gps_speed, 2); dataFile.print("\t");
    dataFile.print(gps_course, 2); dataFile.print("\t");
    dataFile.print(gps_satelite, 2); dataFile.print("\t");
    dataFile.print(gps_hdop, 2); dataFile.print("\t");

    dataFile.print(GPS_godzina); dataFile.print(".");
    dataFile.print(GPS_minuta); dataFile.print(".");
    dataFile.print(GPS_sekunda); dataFile.print("\t");

    dataFile.print(GPS_dzien); dataFile.print(".");
    dataFile.print(GPS_miesiac); dataFile.print(".");
    dataFile.print(GPS_rok); dataFile.print("\t");

   

    dataFile.print(ax, 3); dataFile.print("\t");
    dataFile.print(ay, 3); dataFile.print("\t");
    dataFile.print(az, 3); dataFile.print("\t");
    dataFile.print(gx, 3); dataFile.print("\t");
    dataFile.print(gy, 3); dataFile.print("\t");
    dataFile.print(gz, 3); dataFile.print("\t");
    dataFile.print(temp_mpu, 2); dataFile.print("\t");

    dataFile.print(temp_bmp, 2); dataFile.print("\t");
    dataFile.print(pressure_bmp, 2); dataFile.print("\t");
    dataFile.print(altitude_bmp, 2); dataFile.print("\t");

    dataFile.print(temp_AHT10, 2); dataFile.print("\t");
    dataFile.print(humidity_AHT10, 2); dataFile.print("\t");

    dataFile.print(mq2_analog); dataFile.print("\t");
    dataFile.println(mq2_digital == LOW ? "Wysoki poziom gazu" : "Normalny poziom gazu");

   
    dataFile.close(); // Zamykamy po każdym wpisie!
    Serial.println("Dane dopisane do pliku!");
  } 
  else {
    Serial.println("Błąd dopisywania danych!");
  }

  delay(5000); // Co 5 sekundę
}
