#include <ESP8266WiFi.h>

#define WIFI_SSID      "Aaa"
#define WIFI_PASSWORD  ""

const char* statusStr(wl_status_t s) {
  switch (s) {
    case WL_IDLE_STATUS:     return "IDLE";
    case WL_NO_SSID_AVAIL:   return "SSID tidak ditemukan";
    case WL_SCAN_COMPLETED:  return "Scan selesai";
    case WL_CONNECTED:       return "TERHUBUNG";
    case WL_CONNECT_FAILED:  return "GAGAL (password?)";
    case WL_WRONG_PASSWORD:  return "PASSWORD salah";
    case WL_CONNECTION_LOST: return "Koneksi hilang";
    case WL_DISCONNECTED:    return "Terputus";
    default:                 return "?";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n\n========================================"));
  Serial.println(F("  DIAGNOSA WIFI ESP8266"));
  Serial.println(F("========================================"));

  Serial.printf("Chip ID      : %08X\n", ESP.getChipId());
  Serial.printf("Core version : %s\n", ESP.getCoreVersion().c_str());
  Serial.printf("Flash size   : %d KB\n", ESP.getFlashChipRealSize() / 1024);
  Serial.printf("Free heap    : %d bytes\n", ESP.getFreeHeap());
  Serial.printf("CPU freq     : %d MHz\n", ESP.getCpuFreqMHz());
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(500);

  Serial.println(F(">>> SCAN WiFi..."));
  int n = WiFi.scanNetworks(false, true);
  Serial.printf(">>> Hasil scan: %d\n", n);

  if (n < 0) {
    Serial.println(F("!!! SCAN GAGAL !!!"));
    Serial.println(F("    Penyebab: DAYA kurang / antena. Pakai kabel USB pendek & port kuat."));
  } else if (n == 0) {
    Serial.println(F("!!! Tidak ada WiFi terdeteksi !!!"));
    Serial.println(F("    -> ESP8266 hanya 2.4GHz. Cek jaringan Anda."));
  } else {
    Serial.printf("Ditemukan %d jaringan:\n", n);
    for (int i = 0; i < n; i++) {
      bool target = (WiFi.SSID(i) == String(WIFI_SSID));
      Serial.printf("  %2d) %-24s RSSI=%4d dBm  ch=%2d  enc=%d  %s\n",
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                    WiFi.channel(i), (int)WiFi.encryptionType(i),
                    target ? "<<< TARGET" : "");
    }
  }
  WiFi.scanDelete();

  Serial.println();
  Serial.printf(">>> Coba connect ke \"%s\" ...\n", WIFI_SSID);
  if (strlen(WIFI_PASSWORD) == 0) {
    Serial.println(F("    (WiFi terbuka / tanpa password)"));
    WiFi.begin(WIFI_SSID);
  } else {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  for (int i = 0; i < 40; i++) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) break;
    Serial.printf("  [%2ds] status=%d (%s)\n", i / 2, (int)st, statusStr(st));
    delay(500);
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("========================================"));
    Serial.println(F("  BERHASIL TERHUBUNG!"));
    Serial.println(F("========================================"));
    Serial.printf("  IP      : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  Gateway : %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("  Subnet  : %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("  RSSI    : %d dBm\n", WiFi.RSSI());
    Serial.printf("  Channel : %d\n", WiFi.channel());
    Serial.printf("  BSSID   : %s\n", WiFi.BSSIDstr().c_str());
  } else {
    Serial.println(F("========================================"));
    Serial.println(F("  GAGAL TERHUBUNG"));
    Serial.println(F("========================================"));
    Serial.printf("  Status akhir: %d (%s)\n", (int)WiFi.status(), statusStr(WiFi.status()));
    Serial.println();
    Serial.println(F("  LANGKAH PERBAIKAN:"));
    Serial.println(F("  1. Scan GAGAL -> masalah DAYA: kabel USB pendek & port USB kuat"));
    Serial.println(F("  2. SSID tidak ada -> jaringan 5GHz? ESP8266 hanya 2.4GHz"));
    Serial.println(F("  3. SSID ada tapi gagal -> cek PASSWORD"));
  }
  Serial.println();
  Serial.println(F("Tekan tombol RESET untuk uji ulang."));
}

void loop() {
  delay(10000);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[loop] Masih terputus, mencoba reconnect..."));
    WiFi.reconnect();
  }
}
