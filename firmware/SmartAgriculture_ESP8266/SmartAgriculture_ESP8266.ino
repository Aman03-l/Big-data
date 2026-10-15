#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <time.h>
#include <DHT.h>
#include <BH1750.h>

struct WifiCred { const char* ssid; const char* pass; };
static const WifiCred WIFI_LIST[] = {
  { "Aaa", "" },
};
static const int WIFI_LIST_COUNT = sizeof(WIFI_LIST) / sizeof(WIFI_LIST[0]);

#define WIFI_TIMEOUT_MS  15000

#define INFLUX_HOST      "Salman"
#define INFLUX_FALLBACK  "10.109.10.188"
#define INFLUX_PORT      8086
#define INFLUX_ORG       "smart-agri"
#define INFLUX_BUCKET    "agriculture"
#define INFLUX_TOKEN     "smart-agri-dev-token"

#define DEVICE_ID        "esp8266-garden-01"
#define LOCATION_TAG     "greenhouse-1"

#define PIN_SOIL_ADC     A0
#define PIN_DHT          14
#define PIN_I2C_SDA      4
#define PIN_I2C_SCL      5

#define DHT_TYPE         DHT11
#define BH1750_ADDR      0x23
#define SOIL_ADC_SAMPLES 16

#define SOIL_ADC_DRY     1022
#define SOIL_ADC_WET     473
#define SOIL_ADC_MAX     1023

#define SOIL_MOISTURE_MIN   30.0f
#define SOIL_MOISTURE_MAX   80.0f
#define SOIL_CRITICAL       20.0f
#define AIR_TEMP_HIGH       35.0f
#define LIGHT_LOW_LUX       5000.0f
#define LIGHT_HIGH_LUX      60000.0f

#define SAMPLE_INTERVAL_MS   60000UL
#define SERIAL_BAUD          115200
#define NTP_SERVER           "pool.ntp.org"
#define NTP_GMT_OFFSET_SEC   (7 * 3600)

DHT    dht(PIN_DHT, DHT_TYPE);
BH1750 bh1750;
bool   bhReady = false;

struct SensorData {
  float soilPercent;
  int   soilRaw;
  float soilVoltage;
  float airTemp;
  float airHumidity;
  float lightLux;
  bool  dhtOk;
  bool  bh1750Ok;
};

struct Prediction {
  float slope;
  float hoursToDry;
  float healthScore;
  bool  irrigationNeeded;
  char  status[48];
};

#define HIST_SIZE 12
static float    histSoil[HIST_SIZE];
static uint32_t histTime[HIST_SIZE];
static int      histCount = 0;
static int      histHead  = 0;

static uint32_t gLastSample = 0;
static bool     gTimeSynced = false;

void sensorsBegin() {

  dht.begin();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  bhReady = bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR, &Wire);
  if (bhReady) Serial.println(F("[OK] BH1750 terdeteksi."));
  else         Serial.println(F("[WARN] BH1750 TIDAK terdeteksi (cek SDA/SCL)."));
}

float soilRawToPercent(int raw) {
  float span = (float)(SOIL_ADC_DRY - SOIL_ADC_WET);
  if (span <= 0) span = 1.0f;
  float pct = ((float)SOIL_ADC_DRY - (float)raw) / span * 100.0f;
  if (pct < 0.0f)   pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  return pct;
}

static int readSoilRaw() {
  uint32_t acc = 0;
  for (int i = 0; i < SOIL_ADC_SAMPLES; i++) {
    acc += analogRead(PIN_SOIL_ADC);
    delay(5);
  }
  return (int)(acc / SOIL_ADC_SAMPLES);
}

void readSensors(SensorData &out) {
  out.soilRaw     = readSoilRaw();
  out.soilPercent = soilRawToPercent(out.soilRaw);
  out.soilVoltage = out.soilRaw / (float)SOIL_ADC_MAX * 3.3f;

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    out.dhtOk = false;
    out.airTemp = out.airHumidity = NAN;
    Serial.println(F("[WARN] DHT22 gagal dibaca."));
  } else {
    out.dhtOk = true; out.airTemp = t; out.airHumidity = h;
  }

  if (bhReady) {
    float lux = bh1750.readLightLevel();
    if (isnan(lux) || lux < 0) { out.bh1750Ok = false; out.lightLux = NAN; }
    else                       { out.bh1750Ok = true;  out.lightLux = lux; }
  } else {
    out.bh1750Ok = false; out.lightLux = NAN;
  }
}

void predictorBegin() { histCount = 0; histHead = 0; }

static void pushHistory(float soilPct, uint32_t epochSec) {
  histSoil[histHead] = soilPct;
  histTime[histHead] = epochSec;
  histHead = (histHead + 1) % HIST_SIZE;
  if (histCount < HIST_SIZE) histCount++;
}

static float linearSlope() {
  if (histCount < 3) return 0.0f;
  double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
  int n = histCount;
  uint32_t t0 = histTime[(histHead - histCount + HIST_SIZE) % HIST_SIZE];
  for (int i = 0; i < n; i++) {
    int idx = (histHead - histCount + i + HIST_SIZE * 2) % HIST_SIZE;
    double x = (double)(histTime[idx] - t0) / 3600.0;
    double y = histSoil[idx];
    sumX += x; sumY += y; sumXY += x * y; sumXX += x * x;
  }
  double denom = (n * sumXX - sumX * sumX);
  if (fabs(denom) < 1e-9) return 0.0f;
  return (float)((n * sumXY - sumX * sumY) / denom);
}

Prediction computePrediction(const SensorData &d) {
  Prediction p;
  time_t nowT = time(nullptr);
  uint32_t epoch = (uint32_t)nowT;
  if (epoch < 1600000000UL) epoch = (uint32_t)(millis() / 1000);

  pushHistory(d.soilPercent, epoch);
  p.slope = linearSlope();

  if (p.slope < -0.05f && d.soilPercent > SOIL_MOISTURE_MIN)
    p.hoursToDry = (d.soilPercent - SOIL_MOISTURE_MIN) / (-p.slope);
  else
    p.hoursToDry = -1.0f;

  float score = 100.0f;
  if (d.soilPercent < SOIL_CRITICAL)          score -= 45.0f;
  else if (d.soilPercent < SOIL_MOISTURE_MIN) score -= 25.0f;
  else if (d.soilPercent > SOIL_MOISTURE_MAX) score -= 15.0f;
  if (!isnan(d.airTemp)) {
    if (d.airTemp > AIR_TEMP_HIGH) score -= 15.0f;
    else if (d.airTemp < 10.0f)    score -= 10.0f;
  }
  if (!isnan(d.lightLux)) {
    if (d.lightLux < LIGHT_LOW_LUX)       score -= 10.0f;
    else if (d.lightLux > LIGHT_HIGH_LUX) score -= 5.0f;
  }
  if (score < 0) score = 0;
  if (score > 100) score = 100;
  p.healthScore = score;

  p.irrigationNeeded = (d.soilPercent < SOIL_MOISTURE_MIN) ||
                       (p.hoursToDry >= 0 && p.hoursToDry < 6.0f);

  if (d.soilPercent < SOIL_CRITICAL)          strcpy(p.status, "KRITIS - segera siram!");
  else if (p.irrigationNeeded)                snprintf(p.status, sizeof(p.status), "Perlu irigasi (~%.1f jam)", p.hoursToDry);
  else if (d.soilPercent > SOIL_MOISTURE_MAX) strcpy(p.status, "Tanah terlalu basah");
  else                                        strcpy(p.status, "Optimal");

  return p;
}

static String escapeTag(const char *s) {
  String out;
  for (const char *p = s; *p; ++p) {
    if (*p == ' ' || *p == ',' || *p == '=') out += '\\';
    out += *p;
  }
  return out;
}

static String fnum(float v, int dec = 2) {
  if (isnan(v) || isinf(v)) v = 0.0f;
  return String(v, dec);
}

static String resolveServerHost() {
  static String cached;
  if (cached.length() > 0) return cached;

  IPAddress ip;
  String name = String(INFLUX_HOST) + ".local";
  if (WiFi.hostByName(name.c_str(), ip)) {
    cached = ip.toString();
    Serial.printf("[INFLUX] resolve %s -> %s\n", name.c_str(), cached.c_str());
    return cached;
  }

  Serial.printf("[INFLUX] resolve gagal, pakai IP: %s\n", INFLUX_FALLBACK);
  cached = INFLUX_FALLBACK;
  return cached;
}

bool influxWrite(const SensorData &d, const Prediction &p, time_t epochSec) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String line = "agriculture,device=" + escapeTag(DEVICE_ID) +
                ",location=" + escapeTag(LOCATION_TAG) + " ";
  line += "soil_moisture="        + fnum(d.soilPercent, 2);
  line += ",soil_raw="            + String(d.soilRaw) + "i";
  line += ",soil_voltage="        + fnum(d.soilVoltage, 3);
  line += ",air_temperature="     + fnum(d.airTemp, 2);
  line += ",air_humidity="        + fnum(d.airHumidity, 2);
  line += ",light_lux="           + fnum(d.lightLux, 1);
  line += ",predict_slope="       + fnum(p.slope, 3);
  line += ",predict_hours_to_dry=" + fnum(p.hoursToDry, 2);
  line += ",health_score="        + fnum(p.healthScore, 1);
  line += ",irrigation_needed="   + String(p.irrigationNeeded ? "true" : "false");

  if (epochSec > 1600000000UL)
    line += " " + String((uint64_t)epochSec * 1000000000ULL);

  String url = "http://" + resolveServerHost() + ":" + String(INFLUX_PORT) +
               "/api/v2/write?org=" + INFLUX_ORG +
               "&bucket=" + INFLUX_BUCKET + "&precision=ns";

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Authorization", "Token " INFLUX_TOKEN);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  http.setTimeout(10000);

  int code = http.POST(line);
  bool ok = (code >= 200 && code < 300);
  if (ok) Serial.printf("[INFLUX] OK (%d)\n", code);
  else {
    Serial.printf("[INFLUX] GAGAL code=%d: %s\n", code, http.getString().c_str());
    Serial.printf("[INFLUX] payload: %s\n", line.c_str());
  }
  http.end();
  return ok;
}

static const char* wifiStatusStr(wl_status_t s) {
  switch (s) {
    case WL_IDLE_STATUS:     return "IDLE";
    case WL_NO_SSID_AVAIL:   return "SSID tidak ditemukan";
    case WL_SCAN_COMPLETED:  return "Scan selesai";
    case WL_CONNECTED:       return "TERHUBUNG";
    case WL_CONNECT_FAILED:  return "Koneksi GAGAL (password salah?)";
    case WL_WRONG_PASSWORD:  return "PASSWORD salah";
    case WL_CONNECTION_LOST: return "Koneksi terputus";
    case WL_DISCONNECTED:    return "Terputus";
    default:                 return "Tidak diketahui";
  }
}

static void scanWiFi() {
  Serial.println(F("[WiFi] Scan jaringan sekitar..."));
  int n = WiFi.scanNetworks();
  if (n == WIFI_SCAN_FAILED) {
    Serial.println(F("[WiFi] !!! SCAN GAGAL !!!"));
    Serial.println(F("       -> Masalah DAYA atau antena: pakai kabel USB pendek,"));
    Serial.println(F("          colok langsung ke port USB (bukan hub), adaptor 5V 2A."));
  } else if (n == 0) {
    Serial.println(F("[WiFi] TIDAK ADA jaringan terdeteksi!"));
    Serial.println(F("       -> Cek: WiFi 2.4GHz? ESP8266 hanya support 2.4GHz."));
  } else {
    Serial.printf("[WiFi] Ditemukan %d jaringan:\n", n);
    for (int i = 0; i < n; i++) {
      bool match = false;
      for (int j = 0; j < WIFI_LIST_COUNT; j++) {
        if (WiFi.SSID(i) == String(WIFI_LIST[j].ssid)) { match = true; break; }
      }
      Serial.printf("   %2d) %-20s RSSI=%d dBm ch=%d %s\n",
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                    WiFi.channel(i), match ? "<== TARGET" : "");
    }
  }
  WiFi.scanDelete();
}

static bool tryConnect(const char* ssid, const char* pass) {
  bool open = (pass == nullptr || pass[0] == '\0');
  Serial.printf("[WiFi] Mencoba \"%s\" %s ...\n", ssid,
                open ? "(terbuka/tanpa password)" : "");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  if (open) {
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, pass);
  }

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(500); Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] TERHUBUNG ke \"%s\". IP: %s  RSSI: %d dBm\n",
                  ssid, WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }
  Serial.printf("[WiFi] Gagal \"%s\": %s (code %d)\n",
                ssid, wifiStatusStr(WiFi.status()), (int)WiFi.status());
  return false;
}

void connectWiFi() {
  for (int i = 0; i < WIFI_LIST_COUNT; i++) {
    if (tryConnect(WIFI_LIST[i].ssid, WIFI_LIST[i].pass)) {

      if (MDNS.begin("esp8266-agri")) {
        Serial.println(F("[mDNS] Aktif (esp8266-agri.local)"));
      }
      return;
    }
  }
  Serial.println(F("[WiFi] SEMUA jaringan di daftar GAGAL."));
  scanWiFi();
  Serial.println(F("[WiFi] >> Lihat daftar di atas: apakah SSID target muncul?"));
  Serial.println(F("[WiFi] >> Tidak muncul -> sinyal lemah / 5GHz / SSID beda."));
  Serial.println(F("[WiFi] >> Muncul tapi gagal -> kemungkinan password salah."));
}

void syncTime() {
  configTime(NTP_GMT_OFFSET_SEC, 0, NTP_SERVER);
  Serial.print(F("[NTP] Sinkronisasi waktu"));
  time_t nowT = 0;
  struct tm ti;
  for (int i = 0; i < 20; i++) {
    time(&nowT);
    if (nowT > 1600000000UL && getLocalTime(&ti)) {
      gTimeSynced = true;
      Serial.printf(" OK -> %04d-%02d-%02d %02d:%02d:%02d\n",
                    ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
                    ti.tm_hour, ti.tm_min, ti.tm_sec);
      return;
    }
    Serial.print('.'); delay(500);
  }
  Serial.println(F(" GAGAL (lanjut tanpa timestamp presisi)."));
}

void doSample() {
  Serial.println(F("\n=================== SAMPLING ==================="));
  SensorData d;
  readSensors(d);
  Prediction p = computePrediction(d);

  Serial.printf("Soil      : %.1f %%  (raw=%d, %.3f V)\n", d.soilPercent, d.soilRaw, d.soilVoltage);

  if (d.soilRaw >= SOIL_ADC_MAX - 2) {
    Serial.println(F("  [WARN] soil_raw mentok maksimum! Cek sambungan A0/VCC/GND sensor soil,"));
    Serial.println(F("         atau kalibrasi SOIL_ADC_DRY (lihat docs/CALIBRATION.md)."));
  } else if (d.soilRaw <= 2) {
    Serial.println(F("  [WARN] soil_raw mentok minimum! Cek sambungan sensor soil (mungkin short)."));
  }
  if (d.dhtOk) Serial.printf("Udara     : %.1f C  %.1f %%RH\n", d.airTemp, d.airHumidity);
  else         Serial.println(F("Udara     : gagal baca DHT (cek wiring & tipe DHT_TYPE)"));
  if (d.bh1750Ok) Serial.printf("Cahaya    : %.1f lux\n", d.lightLux);
  else            Serial.println(F("Cahaya    : gagal baca BH1750"));
  Serial.printf("Prediksi  : slope=%.3f %%/jam, kering dlm %.1f jam\n", p.slope, p.hoursToDry);
  Serial.printf("Kesehatan : %.0f/100  | Irigasi: %s\n", p.healthScore, p.irrigationNeeded ? "YA" : "tidak");
  Serial.printf("Status    : %s\n", p.status);

  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() == WL_CONNECTED)
    influxWrite(d, p, gTimeSynced ? time(nullptr) : 0);
  else
    Serial.println(F("[INFLUX] Dilewati: WiFi tidak terhubung."));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  Serial.println(F("\n\n### Smart Agriculture & Predictive Farming (ESP8266) ###"));
  Serial.printf("Flash: %d KB  |  Free heap: %d bytes\n",
                ESP.getFlashChipRealSize() / 1024, ESP.getFreeHeap());

  sensorsBegin();
  predictorBegin();
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) syncTime();

  doSample();
  gLastSample = millis();
}

void loop() {
  if (millis() - gLastSample >= SAMPLE_INTERVAL_MS) {
    gLastSample = millis();
    doSample();
  }

  if (WiFi.status() != WL_CONNECTED) {
    static uint32_t lastRetry = 0;
    if (millis() - lastRetry > 15000) {
      lastRetry = millis();
      connectWiFi();
    }
  }
  delay(100);
}
