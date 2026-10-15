// =====================================================================
//  CEK KONEKSI WIFI ESP8266 - "Aaa" (TANPA PASSWORD)
//  ------------------------------------------------------------------
//  Sketch sederhana untuk menguji apakah ESP8266 bisa connect ke
//  WiFi "Aaa" yang terbuka (open / tanpa password).
//
//  CARA PAKAI:
//    1. Buka file ini di Arduino IDE
//    2. Tools > Board > "NodeMCU 1.0 (ESP-12E Module)"
//    3. Tools > Port > COM... (pilih yang muncul saat modul dicolok)
//    4. Klik Upload
//    5. Buka Serial Monitor (115200 baud)
// =====================================================================

#include <ESP8266WiFi.h>

// ======================= KONFIGURASI =======================
#define WIFI_SSID      "Aaa"
// WiFi terbuka -> password KOSONG. Jangan diisi!
// ===========================================================

#define WIFI_TIMEOUT_MS  20000   // timeout koneksi (20 detik)

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("=============================================="));
  Serial.println(F("   CEK KONEKSI WIFI - ESP8266"));
  Serial.println(F("=============================================="));
  Serial.printf("  Target SSID : \"%s\"\n", WIFI_SSID);
  Serial.println(F("  Password    : (TIDAK ADA / terbuka)"));
  Serial.println(F("----------------------------------------------"));

  // Info modul
  Serial.printf("  Chip ID     : %08X\n", ESP.getChipId());
  Serial.printf("  Core        : %s\n", ESP.getCoreVersion().c_str());
  Serial.printf("  Flash       : %d KB\n", ESP.getFlashChipRealSize() / 1024);
  Serial.printf("  Free heap   : %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  MAC address : %s\n", WiFi.macAddress().c_str());
  Serial.println(F("----------------------------------------------"));

  // --- Mode station ---
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(500);

  // --- SCAN dulu: apakah "Aaa" terlihat? ---
  Serial.println(F("[1] Scan jaringan sekitar..."));
  int n = WiFi.scanNetworks(false, true);
  Serial.printf("    Hasil scan: %d jaringan\n", n);

  bool found = false;
  int  targetRSSI = 0;
  if (n <= 0) {
    Serial.println(F("    !! Tidak ada jaringan / scan gagal."));
    Serial.println(F("    -> Cek: modul & PC di WiFi 2.4GHz, daya USB cukup."));
  } else {
    for (int i = 0; i < n; i++) {
      bool t = (WiFi.SSID(i) == String(WIFI_SSID));
      Serial.printf("    %2d) %-22s RSSI=%4d dBm ch=%2d %s\n",
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                    WiFi.channel(i), t ? "<<< TARGET" : "");
      if (t) { found = true; targetRSSI = WiFi.RSSI(i); }
    }
  }
  WiFi.scanDelete();

  if (found) {
    Serial.printf("[2] \"%s\" DITEMUKAN (sinyal %d dBm).\n", WIFI_SSID, targetRSSI);
  } else {
    Serial.printf("[2] \"%s\" TIDAK terlihat di scan!\n", WIFI_SSID);
    Serial.println(F("    -> Cek ejaan SSID, atau jaringan terlalu jauh."));
  }

  // --- CONNECT (tanpa password) ---
  Serial.println(F("----------------------------------------------"));
  Serial.printf("[3] Menyambung ke \"%s\" (terbuka) ...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID);        // <== TANPA password

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();

  // --- HASIL ---
  Serial.println(F("----------------------------------------------"));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("  BERHASIL TERHUBUNG!"));
    Serial.println(F("=============================================="));
    Serial.printf("  SSID     : %s\n", WiFi.SSID().c_str());
    Serial.printf("  IP       : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  Gateway  : %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("  Subnet   : %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("  DNS      : %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("  RSSI     : %d dBm\n", WiFi.RSSI());
    Serial.printf("  Channel  : %d\n", WiFi.channel());
    Serial.printf("  BSSID    : %s\n", WiFi.BSSIDstr().c_str());
    Serial.printf("  MAC      : %s\n", WiFi.macAddress().c_str());
    Serial.println(F("=============================================="));

    // --- TES KONEKSI ke server InfluxDB di PC ---
    Serial.println(F("[4] Tes koneksi ke server (InfluxDB PC)..."));
    WiFiClient client;
    // IP PC: 10.4.75.52, port InfluxDB 8086
    if (client.connect("10.4.75.52", 8086)) {
      Serial.println(F("    OK - Server terjangkau (10.4.75.52:8086)"));
      Serial.println(F("    -> ESP8266 siap kirim data ke InfluxDB!"));
      client.stop();
    } else {
      Serial.println(F("    GAGAL - Server TIDAK terjangkau"));
      Serial.println(F("    -> Cek IP PC (ipconfig) & InfluxDB jalan (docker ps)"));
    }
    Serial.println(F("=============================================="));
    Serial.println(F("  SELESAI. WiFi OK! Tekan RESET untuk uji ulang."));
  } else {
    Serial.println(F("  GAGAL TERHUBUNG"));
    Serial.println(F("=============================================="));
    Serial.printf("  Status akhir : %d\n", (int)WiFi.status());
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println(F("  -> SSID tidak ditemukan. Cek nama, atau 5GHz."));
        break;
      case WL_CONNECT_FAILED:
        Serial.println(F("  -> Koneksi gagal. Pastikan WiFi benar-benar terbuka."));
        break;
      case WL_WRONG_PASSWORD:
        Serial.println(F("  -> Password salah. WiFi mungkin TIDAK terbuka."));
        break;
      case WL_DISCONNECTED:
        Serial.println(F("  -> Terputus. Sinyal lemah / router menolak."));
        break;
      default:
        Serial.println(F("  -> Status tidak dikenal."));
    }
    Serial.println(F("  Tekan RESET untuk uji ulang."));
  }
}

void loop() {
  // Pantau koneksi tiap 10 detik
  delay(10000);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[loop] Masih terhubung. IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.println(F("[loop] Terputus! Mencoba reconnect..."));
    WiFi.reconnect();
  }
}
