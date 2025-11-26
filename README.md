# 🌦️ IoT Weather Station (ESP8266 + OLED)

![Platform](https://img.shields.io/badge/Platform-ESP8266-blue)
![License](https://img.shields.io/badge/License-Open%20Source-green)
![Status](https://img.shields.io/badge/Status-Stable-brightgreen)

**Smart Weather Monitor** berbasis ESP8266 yang menampilkan data cuaca real-time, suhu, dan waktu akurat pada layar OLED. Proyek ini mengutamakan keamanan data dan kemudahan penggunaan dengan antarmuka konfigurasi berbasis web yang modern.

## ✨ Fitur Unggulan

* **🔒 Secure by Default**: Tidak ada *hardcode* credensial. SSID, Password, dan API Key dikonfigurasi melalui portal, bukan di dalam kodingan.
* **🎨 Modern Dark Mode Web UI**: Antarmuka konfigurasi (`192.168.4.1`) kini hadir dengan desain *Dark Matte*, responsif, dan elegan untuk pengaturan yang nyaman via HP.
* **💾 Data Persistence**: Pengaturan disimpan secara permanen menggunakan **LittleFS**, sehingga data tidak hilang saat alat dimatikan.
* **🖥️ OLED Dashboard**: Menampilkan informasi lengkap:
    * Nama Kota & Hari (Bergantian otomatis).
    * Jam Digital (Auto-sync NTP Server).
    * Suhu, Kelembaban, Angin, & Tekanan Udara.
    * Indikator Sinyal WiFi & Ikon Cuaca Grafis.
* **🔄 Auto-Recovery**: Otomatis kembali ke mode Access Point (AP) jika koneksi WiFi terputus atau konfigurasi salah.

## 🛠️ Kebutuhan Hardware

| Komponen | Keterangan |
| :--- | :--- |
| **Microcontroller** | NodeMCU v3 / Wemos D1 Mini (ESP8266) |
| **Display** | OLED SSD1306 0.96 inch (Resolusi 128x64) |
| **Kabel** | 4x Kabel Jumper (Female-to-Female) |

### 🔌 Wiring Diagram (I2C)

Hubungkan pin OLED ke Board ESP8266 sesuai tabel berikut:

| Pin OLED | NodeMCU (ESP8266) | Fungsi |
| :--- | :--- | :--- |
| **VCC** | 3.3V (3V3) | Power Supply |
| **GND** | GND | Ground |
| **SCL** | D1 (GPIO 5) | Clock Line |
| **SDA** | D2 (GPIO 4) | Data Line |

> **Catatan:** Tombol Reset menggunakan tombol **FLASH** bawaan pada board (GPIO 0), jadi tidak perlu merakit tombol tambahan.

## 📥 Instalasi Software

Pastikan Anda telah menginstal **Arduino IDE** dan library berikut melalui *Library Manager*:

1.  `ESP8266WiFi` & `ESP8266HTTPClient` (Bawaan Board)
2.  `Adafruit GFX Library`
3.  `Adafruit SSD1306`
4.  `ArduinoJson` (Gunakan versi 6.x)
5.  `WiFiManager` (by tzapu)
6.  `LittleFS` (Pastikan memilih board setting dengan FS, misal: *Flash Size: 4MB (FS:2MB)*).



## 🚀 Cara Penggunaan (First Run)

Karena kode ini kosong secara default (untuk keamanan), ikuti langkah ini saat pertama kali menyalakan alat:

1.  **Nyalakan Alat**: Layar OLED akan menampilkan **"BOOTING"** lalu **"SETUP MODE"**.
2.  **Koneksi WiFi**:
    * Cari WiFi bernama **`Weather_AP`** di HP/Laptop Anda.
    * Sambungkan (Tanpa password).
3.  **Buka Portal Konfigurasi**:
    * Buka browser dan akses **`192.168.4.1`**.
    * Anda akan melihat tampilan Web UI baru yang modern (Dark Mode).
4.  **Isi Data**:
    * Pilih SSID WiFi rumah Anda & masukkan Password.
    * **City**: Isi nama kota (contoh: `Jakarta` atau `Surabaya`).
    * **API Key**: Masukkan API Key dari [OpenWeatherMap](https://openweathermap.org/) (Gratis).
5.  **Save**: Klik tombol Save. Alat akan restart dan menampilkan data cuaca.

## ⚠️ Fitur Factory Reset

Jika Anda ingin mengganti WiFi atau memperbaiki kesalahan input:

1.  Tekan dan **tahan tombol FLASH** (tombol kecil dekat USB di NodeMCU) selama 3-5 detik saat alat menyala.
2.  Layar akan menampilkan **"SYSTEM ALERT: HOLD TO RESET"**.
3.  Terus tahan hingga muncul loading bar **"SYSTEM RESET"**.
4.  Alat akan menghapus konfigurasi dan restart kembali ke Mode AP.

## 📂 Struktur Folder

```text
IoT-Code/
├── WeatherStation/
│   ├── WeatherStation.ino   # Source Code Utama
│   └── (File project lainnya)
└── README.md                # Dokumentasi ini
