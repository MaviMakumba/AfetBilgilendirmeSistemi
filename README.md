# 🏢 IoT Tabanlı Akıllı Bina Yönetimi ve Otonom Afet Güvenlik Sistemi

![Status](https://img.shields.io/badge/Status-Completed-success)
![Platform](https://img.shields.io/badge/Hardware-NodeMCU%20ESP8266-blue)
![Backend](https://img.shields.io/badge/Backend-.NET%20MVC%20%26%20Firebase-orange)
![Protocol](https://img.shields.io/badge/M2M-MQTT%20(Adafruit)-yellow)

> **Sakarya Üniversitesi - Nesnelerin İnterneti (IoT) Dersi Projesi**

Bu proje, geleneksel bina güvenliğini (NFC/RFID) akıllı afet yönetimiyle birleştiren hibrit bir IoT sistemidir. Sistem, **iki ayrı NodeMCU modülü** üzerinden çalışarak; giriş-çıkışları kontrol eder, deprem/yangın anında otonom karar vererek kilitleri açar ve tüm verileri **Firebase** tabanlı Web Dashboard ve Mobil Uygulama üzerinden sunar.

---

## 🚀 Projenin Amacı ve Özgün Değeri
Mevcut akıllı ev sistemleri genellikle sadece konfora odaklanır. Bu projenin temel farkı **hayat kurtarmaya** odaklanmasıdır.
* **Otonom Tahliye (M2M):** Deprem algılandığında Sensör Modülü, Kapı Modülü ile **MQTT** üzerinden haberleşerek kilitli kapıları otomatik açar.
* **Gelişmiş Deprem Algılama:** Basit titreşim sensörü yerine, sismolojide kullanılan **STA/LTA (Short Term Average / Long Term Average)** algoritması ile MPU6050 verileri işlenerek hatalı alarmlar önlenir.

---

## 🛠 Donanım Mimarisi
Sistem iki ana düğümden oluşur:

### 1. Kapı ve Erişim Kontrol Ünitesi (NodeMCU-1)
* **Görev:** Personel takibi ve kapı kilidi kontrolü.
* **Bileşenler:**
    * NodeMCU V3 (ESP8266)
    * RC522 NFC/RFID Okuyucu
    * SG90 Servo Motor (Kilit Mekanizması)
    * Buzzer & LED (Durum Bildirimi)

### 2. Çevresel İzleme ve Afet İstasyonu (NodeMCU-2)
* **Görev:** Binayı sürekli dinleyen "Duyu Organı".
* **Bileşenler:**
    * NodeMCU V3 (ESP8266)
    * **MPU6050:** 6 Eksenli İvme ve Jiroskop (Deprem Tespiti)
    * **MQ-2:** Yanıcı Gaz ve Duman Sensörü
    * **LM35:** Analog Sıcaklık Sensörü
    * Alarm Sistemi (Buzzer & RGB LED)
    * gorseller/devre.jpg

---

## 💻 Yazılım ve Teknoloji Yığını

| Alan | Teknoloji / Platform | Kullanım Amacı |
| :--- | :--- | :--- |
| **Gömülü Yazılım** | Arduino IDE (C++) | ESP8266 kodlaması ve STA/LTA algoritması. |
| **Veritabanı** | Firebase Realtime DB | Kullanıcı verileri, loglar ve anlık durum senkronizasyonu. |
| **Haberleşme** | Adafruit IO (MQTT) | İki NodeMCU arasındaki M2M (Makineden Makineye) iletişim. |
| **Web Panel** | .NET MVC (C#) | Yönetici paneli, grafiksel raporlama ve geçmiş veri analizi. |
| **Mobil Panel** | Blynk IoT | Cepten anlık izleme ve "Push Notification" bildirimleri. |
| **Kart Yönetimi** | MIT App Inventor | NFC kartlara kişi tanımlamak için geliştirilen mobil araç. |

---

## 📊 Sistem Görselleri ve Arayüzler

### Web Dashboard (.NET MVC)
Sistemin genel durumunun, anlık gaz/sıcaklık verilerinin ve giriş-çıkış loglarının takip edildiği yönetim paneli.
gorseller/ana.jpg
gorseller/kartlar.jpg
gorseller/girisler.jpg
gorseller/afetler.jpg


### Mobil Uygulamalar
* **Blynk:** Ev sahibinin alarm durumunu gördüğü ekran.
* **Kart Yöneticisi:** Yöneticinin yeni kiracı/personel kartı tanımladığı arayüz.
gorseller/appinventor.jpg

---

## ⚙️ Kurulum ve Algoritma Detayları

### STA/LTA Deprem Algoritması
Sistem, sadece "sallantı var" demez; sarsıntının karakteristiğini analiz eder.
* **STA (Kısa Dönem):** Son 0.5 saniyedeki ivme ortalaması.
* **LTA (Uzun Dönem):** Son 30 saniyedeki zemin gürültüsü ortalaması.
* **Tetiklenme:** `Oran = STA / LTA`. Eğer Oran > 2.5 ise sistem **DEPREM** alarmı verir ve kapıları açar.

### Nasıl Çalıştırılır?
1.  `Arduino/` klasöründeki kodları NodeMCU kartlarına yükleyin.
2.  `WebPanel/` klasöründeki .NET projesini Visual Studio ile açıp Firebase bilgilerinizi girin.
3.  Blynk uygulamasından QR kodu taratarak mobil arayüzü klonlayın.

---

## 👥 Proje Ekibi
* **Eren Kartal** - B231210065
* **Umut Arda Vural** - B231210081
* **Özgür Demir** - B221210017

---
*Sakarya Üniversitesi, Bilgisayar ve Bilişim Bilimleri Fakültesi*
