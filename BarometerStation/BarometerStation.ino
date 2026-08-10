/*

  BarometerStation — ESP32 + BME280, WiFi Captive Setup Portal, Web Dashboard

  What this does:
    - Reads temperature / humidity from a BME280 over I2C.
    - On first boot (or if saved WiFi fails), opens an AP named
      "BarometerSetup" with a captive portal so you can enter your WiFi
      credentials from a phone/laptop, same idea as the original
      Precipitate sketch's setup portal (no lat/lon needed here, since
      the sensor is local, not an internet weather lookup).
    - Once connected to your WiFi, serves a live dashboard web page on
      your local network showing current readings + trend, auto-
      refreshing via JS fetch (no OLED, no LED — browser only).
    - A "Forget WiFi" button on the dashboard clears saved credentials
      and reboots back into setup mode (headless equivalent of the
      hardware reset button in the original).

  Wiring (I2C):
    BME280   ESP32
    VIN  --  3V3
    GND  --  GND
    SCL  --  GPIO22 (default)
    SDA  --  GPIO21 (default)

  Libraries needed (Library Manager):
    - Adafruit BME280 Library (+ Adafruit Unified Sensor dependency)

  Usage:
    1. Flash, open Serial Monitor at 115200.
    2. Connect a phone/laptop to WiFi "BarometerSetup" (open network).
       A captive-portal prompt should pop up automatically; if not,
       browse to http://192.168.4.1
    3. Enter your WiFi SSID/password and submit.
    4. The device reboots onto your network. Serial prints the IP —
       open that IP in a browser to see the dashboard.

*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ThingSpeak.h>

// ---------------- Config ----------------

#define BME280_ADDR 0x76

const char*   AP_SSID    = "ARDUINO&ESP32";
const char*   AP_PASS    = "";              // open network
const uint8_t AP_CHANNEL = 6;
IPAddress     apIP(192, 168, 4, 1);

// How often to refresh the "live" numbers shown on the dashboard
const uint32_t LIVE_READ_INTERVAL_MS = 2000;

const uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

// ---------------- Globals ----------------

Adafruit_BME280 bme;
bool g_sensorOK = false;

float g_tempC       = 0.0f;
float g_humidityPct = 0.0f;

uint32_t g_lastLiveMs   = 0;

Preferences prefs;
String saved_ssid, saved_pass;
bool   provisioned = false;

DNSServer dnsServer;
WebServer server(80);
bool inSetupPortal    = false;
bool pendingProvision = false;

float temperatureThreshold = 25.0;
float humidityThreshold = 40.0;

WiFiClient thingSpeakClient;

unsigned long thingSpeakChannel = 3445275;  // Replace with your Channel ID
const char* thingSpeakWriteKey = "RQ0JVY92LO15CJAK";

const uint32_t THINGSPEAK_INTERVAL_MS = 20000; // Send every 20 seconds
uint32_t g_lastThingSpeakMs = 0;

// ============================================================
// Sensor
// ============================================================

static void readLiveSensor(){
  if(!g_sensorOK) return;
  g_tempC       = bme.readTemperature();
  g_humidityPct = bme.readHumidity();
}

// ============================================================
// Prefs
// ============================================================

void loadPrefs(){
  prefs.begin("barometer", true);
  saved_ssid  = prefs.getString("ssid", "");
  saved_pass  = prefs.getString("pass", "");
  provisioned = prefs.getBool("prov", false);
  temperatureThreshold = prefs.getFloat("tLimit", 30.0f);
  humidityThreshold = prefs.getFloat("hLimit", 80.0f);
  prefs.end();
}

// ============================================================
// WiFi
// ============================================================

static bool connectToWiFi(const String &ssid, const String &pass){
  Serial.printf("[WiFi] Connecting to '%s' ...\n", ssid.c_str());
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t t0 = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_CONNECT_TIMEOUT_MS){
    delay(150);
    Serial.print(".");
  }
  Serial.println();

  if(WiFi.status() == WL_CONNECTED){
    Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println("[WiFi] Failed to connect.");
  return false;
}

// ============================================================
// HTML: setup portal
// ============================================================

const char setup_html[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Barometer Station Setup</title>
<style>
  :root{ --card-bg:rgba(255,255,255,0.75); --text:#243040; --hint:#64748b; --accent:#3b82f6; }
  body{ margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center; color:var(--text);
        font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
        background:linear-gradient(120deg,#dbeafe,#e0f2fe,#ede9fe); }
  .card{ background:var(--card-bg); border-radius:18px; padding:28px 26px; width:min(92vw,380px);
         box-shadow:0 10px 30px rgba(0,0,0,0.12); backdrop-filter:blur(6px); }
  h1{ font-size:1.25rem; margin:0 0 4px; }
  p.hint{ margin:0 0 20px; color:var(--hint); font-size:0.9rem; }
  label{ display:block; font-size:0.85rem; margin:14px 0 6px; }
  input{ width:100%; box-sizing:border-box; padding:10px 12px; border-radius:10px;
         border:1px solid #cbd5e1; font-size:1rem; }
  button{ margin-top:20px; width:100%; padding:12px; border:none; border-radius:10px;
          background:var(--accent); color:white; font-size:1rem; cursor:pointer; }
  button:active{ opacity:0.85; }
</style>
</head>
<body>
  <div class="card">
    <h1>Barometer Station</h1>
    <p class="hint">Enter your WiFi details so the station can join your network and serve its dashboard.</p>
    <form action="/save" method="POST">
      <label for="ssid">WiFi Network (SSID)</label>
      <input id="ssid" name="ssid" maxlength="32" required>
      <label for="pass">Password</label>
      <input id="pass" name="pass" type="password" maxlength="63" required>
      <button type="submit">Connect</button>
    </form>
  </div>
</body>
</html>
)HTML";

const char saved_html[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Saved</title>
<style>body{font-family:-apple-system,sans-serif;display:flex;align-items:center;justify-content:center;
min-height:100vh;margin:0;background:#ede9fe;color:#243040;text-align:center}
.card{background:white;padding:28px;border-radius:16px;box-shadow:0 10px 30px rgba(0,0,0,0.1)}</style>
</head><body><div class="card"><h2>Saved</h2>
<p>The station is connecting to your network now.<br>Check its new IP in Serial Monitor,<br>
or look for it on your router.</p></div></body></html>
)HTML";

// ============================================================
// HTML: dashboard (served once WiFi-connected, not in portal mode)
// ============================================================

const char dashboard_html[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Barometer Station</title>
<style>
  :root{ --bg1:#0f172a; --bg2:#1e293b; --card:#111827cc; --text:#e2e8f0; --hint:#94a3b8;
         --rise:#4ade80; --fall:#f87171; --steady:#60a5fa; --unknown:#a1a1aa; }
  body{ margin:0; min-height:100vh; font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
        color:var(--text); background:linear-gradient(160deg,var(--bg1),var(--bg2));
        display:flex; align-items:center; justify-content:center; padding:20px; box-sizing:border-box; }
  .card{ width:min(92vw,420px); background:var(--card); border-radius:20px; padding:26px 24px;
         box-shadow:0 20px 40px rgba(0,0,0,0.35); backdrop-filter:blur(8px); }
  h1{ font-size:1.1rem; margin:0 0 18px; color:var(--hint); font-weight:600; letter-spacing:0.02em; }
  .unit{ font-size:1.1rem; color:var(--hint); margin-left:6px; }
  .divider{ height:1px; background:#334155; margin:18px 0; }
  .stat-row{ display:flex; justify-content:space-between; font-size:1rem; margin:8px 0; }
  .stat-row .label{ color:var(--hint); }
  .footer{ margin-top:20px; display:flex; justify-content:space-between; align-items:center; }
  .updated{ font-size:0.75rem; color:var(--hint); }
  button{ background:none; border:1px solid #475569; color:var(--hint); border-radius:8px;
          padding:6px 10px; font-size:0.75rem; cursor:pointer; }
  button:active{ opacity:0.8; }
  .offline{ color:var(--fall); font-size:0.85rem; margin-top:10px; }
</style>
</head>
<body>
  <div class="card">
    <h1>BAROMETER STATION</h1>
    <div class="stat-row"><span class="label">Temperature</span><span id="temp">--</span></div>
    <div class="stat-row"><span class="label">Humidity</span><span id="humidity">--</span></div>
    <div id="offlineMsg" class="offline" style="display:none">Sensor not detected. Check wiring.</div>
    <div class="divider"></div>

    <label>High-temperature alert (°C)</label>
    <input id="temperatureLimit" type="number" value="30">

    <label>High-humidity alert (%)</label>
    <input id="humidityLimit" type="number" value="80">
    <button onclick="saveLimits()">Save alert limits</button>
    <div class="footer">
      <span class="updated" id="updated">--</span>
      <button onclick="forgetWifi()">Forget WiFi</button>
    </div>
  </div>
<script>
function saveLimits() {
  const q = new URLSearchParams({
    temperature: temperatureLimit.value,
    humidity: humidityLimit.value
  });
  fetch("/settings?" + q).then(() => alert("Limits saved"));
}
function refresh(){
  document.getElementById("offlineMsg").style.display = d.ok ? "none" : "block";
  if(!d.ok) return;
  document.getElementById("temp").textContent = d.temp.toFixed(1) + " °C";
  document.getElementById("humidity").textContent = Math.round(d.humidity) + " %";
  document.getElementById("updated").textContent =
  "Updated " + new Date().toLocaleTimeString();
}
function forgetWifi(){
  if(confirm("Forget saved WiFi and restart into setup mode?")){
    fetch("/reset-wifi").then(() => {
      document.body.innerHTML = "<div style='color:white;text-align:center;margin-top:40vh;font-family:sans-serif'>Restarting into setup mode...</div>";
    });
  }
}
refresh();
setInterval(refresh, 3000);
</script>
</body>
</html>
)HTML";

// ============================================================
// Web handlers
// ============================================================

static bool isIp(const String &str){
  for(size_t i = 0; i < str.length(); i++){
    char c = str[i];
    if(c != '.' && (c < '0' || c > '9')) return false;
  }
  return true;
}

void handleRoot(){
  if(inSetupPortal){
    server.send_P(200, "text/html", setup_html);
  } else {
    server.send_P(200, "text/html", dashboard_html);
  }
}

void handleSave(){
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if(ssid.length() == 0 || ssid.length() > 32){
    server.send(400, "text/plain", "SSID must be 1-32 characters"); return;
  }
  if(pass.length() == 0 || pass.length() > 63){
    server.send(400, "text/plain", "WiFi password must be 1-63 characters"); return;
  }

  prefs.begin("barometer", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putBool("prov", true);
  prefs.end();

  saved_ssid  = ssid;
  saved_pass  = pass;
  provisioned = true;

  server.send_P(200, "text/html", saved_html);
  pendingProvision = true;
}

void handleData(){
  if(!g_sensorOK){
    server.send(200, "application/json", "{\"ok\":false}");
    return;
  }

  char buf[120];
  snprintf(buf, sizeof(buf),
    "{\"ok\":true,\"temp\":%.1f,\"humidity\":%.0f}",
    g_tempC, g_humidityPct);

  server.send(200, "application/json", buf);
}

void handleResetWifi(){
  prefs.begin("barometer", false);
  prefs.clear();
  prefs.end();
  server.send(200, "text/plain", "WiFi forgotten. Rebooting into setup mode...");
  Serial.println("[Reset] WiFi credentials cleared via dashboard.");
  Serial.flush();
  delay(200);
  ESP.restart();
}

void handleNotFound(){
  if(inSetupPortal && !isIp(server.hostHeader())){
    // Captive-portal redirect for unknown hostnames
    server.sendHeader("Location", String("http://") + apIP.toString(), true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void handleSettings() {
  temperatureThreshold = server.arg("temperature").toFloat();
  humidityThreshold = server.arg("humidity").toFloat();

  prefs.begin("barometer", false);
  prefs.putFloat("tLimit", temperatureThreshold);
  prefs.putFloat("hLimit", humidityThreshold);
  prefs.end();

  server.send(200, "text/plain", "Saved");
}


void registerRoutes(){
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/data", HTTP_GET, handleData);
  server.on("/reset-wifi", HTTP_GET, handleResetWifi);
  server.on("/settings", HTTP_GET, handleSettings);
  server.onNotFound(handleNotFound);
}

// ============================================================
// Portal start/stop
// ============================================================

void startSetupPortal(){
  inSetupPortal = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if(strlen(AP_PASS) == 0){
    WiFi.softAP(AP_SSID, NULL, AP_CHANNEL);
  } else {
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL);
  }
  delay(200);
  dnsServer.start(53, "*", apIP);
  server.begin();
  Serial.println("[SETUP] AP started: BarometerSetup (open)");
  Serial.println("[SETUP] Open http://192.168.4.1 to configure.");
}

void stopSetupPortal(){
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  inSetupPortal = false;
  Serial.println("[SETUP] Portal stopped.");
}

void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;

  ThingSpeak.setField(1, g_tempC);
  ThingSpeak.setField(2, g_humidityPct);

  int response = ThingSpeak.writeFields(
    thingSpeakChannel,
    thingSpeakWriteKey
  );

  if (response == 200) {
    Serial.println("[ThingSpeak] Data sent.");
  } else {
    Serial.println("[ThingSpeak] Error: " + String(response));
  }
}

// ============================================================
// Setup & Loop
// ============================================================

void setup(){
  Serial.begin(115200);
  delay(150);

  Wire.begin();
  g_sensorOK = bme.begin(BME280_ADDR);
  if(g_sensorOK){
    Serial.println("[Info] BME280 initialized.");
    readLiveSensor();
  } else {
    Serial.println("[Error] BME280 not found. Check wiring/address (0x76/0x77).");
  }

  loadPrefs();
  registerRoutes();
  ThingSpeak.begin(thingSpeakClient);

  if(!provisioned){
    startSetupPortal();
  } else {
    if(connectToWiFi(saved_ssid, saved_pass)){
      server.begin();
      Serial.println("[Web] Dashboard ready at http://" + WiFi.localIP().toString());
    } else {
      startSetupPortal();
    }
  }

  g_lastLiveMs   = millis();
}

void loop(){
  if(inSetupPortal){
    dnsServer.processNextRequest();
    server.handleClient();

    if(pendingProvision){
      delay(150);
      stopSetupPortal();
      loadPrefs();
      if(connectToWiFi(saved_ssid, saved_pass)){
        pendingProvision = false;
        server.begin();
        Serial.println("[Web] Dashboard ready at http://" + WiFi.localIP().toString());
      } else {
        Serial.println("[WiFi] Connect failed. Returning to setup portal.");
        pendingProvision = false;
        startSetupPortal();
      }
    }
    delay(5);
    return;
  }

  server.handleClient();

  // Try to recover a sensor that wasn't found at boot
  if(!g_sensorOK){
    static uint32_t lastRetry = 0;
    uint32_t now = millis();
    if(now - lastRetry > 5000){
      lastRetry = now;
      g_sensorOK = bme.begin(BME280_ADDR);
      if(g_sensorOK) Serial.println("[Info] BME280 detected.");
    }
    return;
  }

  uint32_t now = millis();

  if(now - g_lastLiveMs >= LIVE_READ_INTERVAL_MS){
    readLiveSensor();
    g_lastLiveMs = now;
  }
  if (now - g_lastThingSpeakMs >= THINGSPEAK_INTERVAL_MS) {
    sendToThingSpeak();
    g_lastThingSpeakMs = now;
  }
}
