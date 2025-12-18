#define BLYNK_TEMPLATE_ID "TMPL6_E93vIuf"
#define BLYNK_TEMPLATE_NAME "Afet Sistemi"
#define BLYNK_AUTH_TOKEN "uVb_FSvif9LfXYwK0c1Onp_nOEmY2oFg"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <time.h>
#include <Firebase_ESP_Client.h>

// --- WIFI AYARLARI ---
char ssid[] = "";
char pass[] = "";

// --- ADAFRUIT MQTT AYARLARI ---
#define AIO_SERVER      ""
#define AIO_SERVERPORT  
#define AIO_USERNAME    ""
#define AIO_KEY         ""

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish kapiYayin = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/kapi-kontrol");

// --- PINLER ---
#define PIN_MQ2 D0
#define PIN_LM35 A0
#define PIN_BUZZER D8
#define PIN_RED D5
#define PIN_GREEN D6
#define PIN_BLUE D7

// --- FIREBASE AYARLARI ---
#define API_KEY ".."
#define DATABASE_URL "---.."

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

MPU6050 mpu(Wire); // MPU Nesnesi
BlynkTimer timer;

// --- DEPREM ALGORİTMASI ---
float st_algo_STA = 0.0;
float st_algo_LTA = 0.001; 
float STA_alpha = 0.20; 
float LTA_alpha = 0.02;
const float RATIO_THRESHOLD = 2.5;

bool alarmDurumu = false;
unsigned long sonAlarmZamani = 0;
const float YANGIN_SINIRI = 50.0; 

void MQTT_connect();

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_MQ2, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  
  renkAyarla(0,1,0); // Yeşil

  // I2C Başlatma
  Wire.begin(D2, D1); // SDA=D2, SCL=D1

  // MPU6050 Başlatma
  byte status = mpu.begin();
  Serial.print("MPU Durumu: "); Serial.println(status);
  
  if (status != 0) {
    Serial.println("MPU hatasi! Baglantilari kontrol et.");
    while (1) { delay(10); } // Hata varsa dur
  }

  Serial.println("MPU6050 Baglandi! Kalibrasyon yapiliyor, kipirdatma...");
  delay(1000);
  mpu.calcOffsets(); // Sensörü sıfırlar
  Serial.println("Kalibrasyon Tamam!");

  // Blynk Başlat
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = API_KEY;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  timer.setInterval(50L, sensorDongusu);

  Serial.print("Saat sunucusuna baglaniyor...");
  // Türkiye Saati (UTC+3) -> 3 * 3600 saniye
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); 

  // Saatin güncellenmesini bekle
  while (time(nullptr) < 1000) {
    delay(100); Serial.print(".");
  }
  Serial.println("\nSaat Guncellendi!"); 
}

void loop() {
  Blynk.run();
  timer.run();
  MQTT_connect();
}

// --- ZAMAN ALMA FONKSİYONU ---
String zamanDamgasiAl() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  
  char buffer[30];
  // Format: Gün.Ay.Yıl Saat:Dakika:Saniye
  sprintf(buffer, "%02d.%02d.%d %02d:%02d:%02d", 
          p_tm->tm_mday, p_tm->tm_mon + 1, p_tm->tm_year + 1900, 
          p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
          
  return String(buffer);
}

void sensorDongusu() {
  // 1. MPU6050 GÜNCELLEME VE OKUMA
  mpu.update();

  // G cinsinden ivme değerleri (1.0 = Yerçekimi)
  float accX = mpu.getAccX();
  float accY = mpu.getAccY();
  float accZ = mpu.getAccZ() - 1.0; // Yerçekimini çıkar (9.81 değil 1.0)
  
  // Titreşim Büyüklüğü
  float anlikIvme = sqrt(accX*accX + accY*accY + accZ*accZ);

  // STA/LTA Hesaplama
  st_algo_STA = (st_algo_STA * (1.0 - STA_alpha)) + (anlikIvme * STA_alpha);
  st_algo_LTA = (st_algo_LTA * (1.0 - LTA_alpha)) + (anlikIvme * LTA_alpha);
  
  float oran = 0.0;
  if (st_algo_LTA > 0) oran = st_algo_STA / st_algo_LTA;

  // 2. GAZ OKUMA
  int gazDurumu = digitalRead(PIN_MQ2); 
  bool gazTehlikesi = (gazDurumu == LOW);

  // 3. SICAKLIK OKUMA
  float toplamOkuma = 0;
  
  // 1. Dalgalanmayı önlemek için 10 kez oku ve topla
  for (int i = 0; i < 10; i++) {
    toplamOkuma += analogRead(PIN_LM35);
    delay(5); 
  }
  
  // 2. Ortalamayı al
  float ortalamaHamDeger = toplamOkuma / 10.0;

  // 3. Hesaplama (Düşük voltaj sorununu çözmek için SONUNA * 10 EKLENDİ)
  // Normal formül: (HamDeger * 3300) / 10240
  float sicaklik = ((ortalamaHamDeger * 3300.0) / 10240.0) * 10.0;

  // 4. İnce Ayar
  bool yanginTehlikesi = (sicaklik > YANGIN_SINIRI);

  // --- ALARM KONTROLÜ ---
  if (!alarmDurumu) {
    // Eşik değer kontrolü
    if (oran > RATIO_THRESHOLD && st_algo_STA > 0.05) { 
      alarmBaslat("DEPREM");
    } 
    else if (gazTehlikesi) {
      alarmBaslat("GAZ");
    }
    else if (yanginTehlikesi) {
      alarmBaslat("YANGIN");
    }
  } 
  else 
  {
    if (millis() - sonAlarmZamani > 5000) 
    {
       if (oran < RATIO_THRESHOLD && !gazTehlikesi && !yanginTehlikesi)
       {
         alarmDurdur();
       }
    }
  }

  // --- BLYNK VERİ GÖNDERME ---
  static unsigned long sonGonderim = 0;
  if (millis() - sonGonderim > 1000) {
    Blynk.virtualWrite(V0, sicaklik); 
    Blynk.virtualWrite(V2, oran);    
    sonGonderim = millis();
  }
}

void alarmBaslat(String tur) {
  if (millis() - sonAlarmZamani < 2000) return; 
  
  alarmDurumu = true;
  sonAlarmZamani = millis();
  
  Serial.println("ALARM: " + tur);
  renkAyarla(1, 0, 0); // Kırmızı
  
  if (tur == "DEPREM") tone(PIN_BUZZER, 1000);
  else if (tur == "GAZ") tone(PIN_BUZZER, 500);
  else if (tur == "YANGIN") tone(PIN_BUZZER, 1500);

  if (tur == "DEPREM") Blynk.logEvent("deprem_alarm", "DIKKAT! Sarsinti Algilandi!");
  else if (tur == "GAZ") Blynk.logEvent("gaz_alarm", "DIKKAT! Gaz Kacagi!");
  else if (tur == "YANGIN") Blynk.logEvent("yangin_alarm", "DIKKAT! Yuksek Sicaklik/Yangin!");

  // İşlemciye nefes aldır
  yield();

  // --- LOGLAMA KISMI ---
  String tarihSaat = zamanDamgasiAl();
  float deger = 0.0;
  
  if (tur == "DEPREM") deger = st_algo_STA; // Deprem büyüklüğü (STA)
  else if (tur == "GAZ") deger = 1.0;       // Gaz var (1)
  else if (tur == "YANGIN") deger = (analogRead(PIN_LM35) * 330.0) / 1024.0; // Sıcaklık
  
  FirebaseJson json;
  json.set("tur", tur);
  json.set("deger", deger);
  json.set("tarih", tarihSaat);

  // İşlemciye nefes aldır
  yield();
  
  // "/Afet_Gecmisi" altına yeni kayıt ekle
  Firebase.RTDB.pushJSON(&fbdo, "/Afet_Gecmisi", &json);
  // ----------------------------


  kapiYayin.publish(1);
}

void alarmDurdur() {
  alarmDurumu = false;
  renkAyarla(0, 1, 0); 
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
}

void renkAyarla(int r, int g, int b) {
  digitalWrite(PIN_RED, r); digitalWrite(PIN_GREEN, g); digitalWrite(PIN_BLUE, b);
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { 
       mqtt.disconnect();
       delay(2000);  
       retries--;
       if (retries == 0) break;
  }
}