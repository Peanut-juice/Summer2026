/* Local Wi-Fi BarometerStation — no internet or ThingSpeak.
   BMP085/BH1750 SDA/SCL=21/22; DHT22 data=32; soil=34; TDS=35; Pump 1=26; Pump 2=33;
   WS2812B=23; fan=27; dehumidifier driver=25; humidifier driver=13; heater driver=19. Use a separate relay/MOSFET driver per actuator.
   Never power motors or the dehumidifier from an ESP32 pin; share GND with its low-voltage driver. */
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <BH1750.h>
#include <DHT.h>

constexpr uint8_t SOIL_PIN=34,TDS_PIN=35,PUMP1_PIN=26,PUMP2_PIN=33,PIXEL_PIN=23,FAN1_PIN=27,DEHUMIDIFIER_PIN=25,HUMIDIFIER_PIN=13,HEATER_PIN=19,DHT_PIN=32;
constexpr uint16_t PIXEL_COUNT=22;
constexpr int MOISTURE_DRY_RAW=3000,MOISTURE_WET_RAW=1300;
constexpr uint32_t SENSOR_INTERVAL_MS=2000,MAX_PUMP_MS=30000,PUMP2_MIN_INTERVAL_MS=600000;
const char* AP_SSID="BarometerSetup"; const char* AP_PASS=""; IPAddress AP_IP(192,168,4,1);
struct Settings { bool autoWater=false,autoLight=true,autoFan=true,autoPump2=false,pump2WhenHigh=false,autoDehumidifier=false,autoHumidifier=false,autoHeater=false; int moistureThreshold=35,lightThresholdLux=80,fanThresholdC=30,heaterThresholdC=18,heaterHysteresis=2,tdsThresholdPpm=800,humidityThreshold=70,humidityHysteresis=3,humidifierThreshold=40,humidifierHysteresis=3; uint32_t pumpMs=3000,pump2Ms=1000,led=0x008000; } settings;
Adafruit_BMP085 bmp; BH1750 lightMeter; DHT dht(DHT_PIN,DHT22); Adafruit_NeoPixel pixel(PIXEL_COUNT,PIXEL_PIN,NEO_GRB+NEO_KHZ800); WebServer server(80); DNSServer dnsServer; Preferences prefs;
bool bmpOK=false,lightOK=false,dhtOK=false,pump1On=false,pump2On=false,fanOn=false,ledOn=false,dehumidifierOn=false,humidifierOn=false,heaterOn=false;
float tempC=NAN,lightLux=NAN,tdsPpm=NAN,dhtTempC=NAN,humidity=NAN; int soilRaw=0,soilPct=0; uint32_t pump1StopAt=0,pump2StopAt=0,lastPump2DoseAt=0,lastSensorMs=0;

int medianRead(uint8_t pin) {
  int values[15];
  for (int &value : values) {
    value = analogRead(pin);
    delay(2);
  }
  for (int i = 0; i < 14; i++) {
    for (int j = i + 1; j < 15; j++) {
      if (values[j] < values[i]) {
        int temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }
  return values[7];
}
int toPercent(int raw) {
  return constrain(map(raw, MOISTURE_DRY_RAW, MOISTURE_WET_RAW, 0, 100), 0, 100);
}
void showLed(bool on) {
  uint32_t color = on ? pixel.Color((settings.led >> 16) & 255,
                                    (settings.led >> 8) & 255,
                                    settings.led & 255)
                      : 0;
  for (uint16_t i = 0; i < PIXEL_COUNT; i++) pixel.setPixelColor(i, color);
  pixel.show();
  ledOn = on;
}
void setFan(bool on) {
  digitalWrite(FAN1_PIN, on ? HIGH : LOW);
  fanOn = on;
}
void setDehumidifier(bool on) {
  digitalWrite(DEHUMIDIFIER_PIN, on ? HIGH : LOW);
  dehumidifierOn = on;
}
void setHumidifier(bool on) {
  digitalWrite(HUMIDIFIER_PIN, on ? HIGH : LOW);
  humidifierOn = on;
}
void setHeater(bool on){digitalWrite(HEATER_PIN,on?HIGH:LOW);heaterOn=on;}
void stopPump1() {
  digitalWrite(PUMP1_PIN, LOW);
  pump1On = false;
  pump1StopAt = 0;
}
void startPump1(uint32_t ms) {
  digitalWrite(PUMP1_PIN, HIGH);
  pump1On = true;
  pump1StopAt = millis() + constrain(ms, 1UL, MAX_PUMP_MS);
}
void stopPump2() {
  digitalWrite(PUMP2_PIN, LOW);
  pump2On = false;
  pump2StopAt = 0;
}
void startPump2(uint32_t ms) {
  digitalWrite(PUMP2_PIN, HIGH);
  pump2On = true;
  pump2StopAt = millis() + constrain(ms, 1UL, MAX_PUMP_MS);
}
void readSensors() {
  if (bmpOK) tempC = bmp.readTemperature();
  if (lightOK) lightLux = lightMeter.readLightLevel();
  float h = dht.readHumidity();
  float dt = dht.readTemperature();
  dhtOK = !isnan(h) && !isnan(dt);
  if (dhtOK) {
    humidity = h;
    dhtTempC = dt;
  }
  soilRaw = medianRead(SOIL_PIN);
  soilPct = toPercent(soilRaw);
  float volts = medianRead(TDS_PIN) * 3.3f / 4095.0f;
  float adjusted = volts / (1 + 0.02f * ((isnan(tempC) ? 25.0f : tempC) - 25));
  tdsPpm = (133.42f * adjusted * adjusted * adjusted -
            255.86f * adjusted * adjusted + 857.39f * adjusted) * 0.5f;
}
void loadSettings(){prefs.begin("station",true);settings.autoWater=prefs.getBool("water",false);settings.autoLight=prefs.getBool("light",true);settings.autoFan=prefs.getBool("fan",true);settings.autoPump2=prefs.getBool("autoP2",false);settings.pump2WhenHigh=prefs.getBool("p2High",false);settings.autoDehumidifier=prefs.getBool("autoDehum",false);settings.autoHumidifier=prefs.getBool("autoHumid",false);settings.autoHeater=prefs.getBool("autoHeat",false);settings.moistureThreshold=prefs.getInt("moisture",35);settings.lightThresholdLux=prefs.getInt("lux",80);settings.fanThresholdC=prefs.getInt("temp",30);settings.heaterThresholdC=prefs.getInt("heatTemp",18);settings.heaterHysteresis=prefs.getInt("heatHyst",2);settings.tdsThresholdPpm=prefs.getInt("tds",800);settings.humidityThreshold=prefs.getInt("humidity",70);settings.humidityHysteresis=prefs.getInt("hyst",3);settings.humidifierThreshold=prefs.getInt("humid",40);settings.humidifierHysteresis=prefs.getInt("humidHyst",3);settings.pumpMs=prefs.getUInt("pump",3000);settings.pump2Ms=prefs.getUInt("pump2",1000);settings.led=prefs.getUInt("led",0x008000);prefs.end();}
void saveSettings(){prefs.begin("station",false);prefs.putBool("water",settings.autoWater);prefs.putBool("light",settings.autoLight);prefs.putBool("fan",settings.autoFan);prefs.putBool("autoP2",settings.autoPump2);prefs.putBool("p2High",settings.pump2WhenHigh);prefs.putBool("autoDehum",settings.autoDehumidifier);prefs.putBool("autoHumid",settings.autoHumidifier);prefs.putBool("autoHeat",settings.autoHeater);prefs.putInt("moisture",settings.moistureThreshold);prefs.putInt("lux",settings.lightThresholdLux);prefs.putInt("temp",settings.fanThresholdC);prefs.putInt("heatTemp",settings.heaterThresholdC);prefs.putInt("heatHyst",settings.heaterHysteresis);prefs.putInt("tds",settings.tdsThresholdPpm);prefs.putInt("humidity",settings.humidityThreshold);prefs.putInt("hyst",settings.humidityHysteresis);prefs.putInt("humid",settings.humidifierThreshold);prefs.putInt("humidHyst",settings.humidifierHysteresis);prefs.putUInt("pump",settings.pumpMs);prefs.putUInt("pump2",settings.pump2Ms);prefs.putUInt("led",settings.led);prefs.end();}
String stateJson(){char o[1200];float safeTemp=isnan(tempC)?0.0f:tempC,safeDhtTemp=isnan(dhtTempC)?0.0f:dhtTempC,safeHumidity=isnan(humidity)?0.0f:humidity,safeLux=isnan(lightLux)?0.0f:lightLux,safeTds=isnan(tdsPpm)?0.0f:tdsPpm;snprintf(o,sizeof(o),"{\"bmp_ok\":%s,\"light_ok\":%s,\"dht_ok\":%s,\"temp\":%.1f,\"dht_temp\":%.1f,\"humidity\":%.1f,\"lux\":%.0f,\"soil\":%d,\"soil_raw\":%d,\"tds\":%.0f,\"pump1\":%s,\"pump2\":%s,\"fan\":%s,\"led_on\":%s,\"dehumidifier\":%s,\"humidifier\":%s,\"heater\":%s,\"auto_water\":%s,\"auto_light\":%s,\"auto_fan\":%s,\"auto_pump2\":%s,\"pump2_high\":%s,\"auto_dehumidifier\":%s,\"auto_humidifier\":%s,\"auto_heater\":%s,\"moisture\":%d,\"light_lux\":%d,\"fan_temp\":%d,\"heater_temp\":%d,\"heater_hysteresis\":%d,\"tds_threshold\":%d,\"pump2_ms\":%lu,\"humidity_threshold\":%d,\"humidity_hysteresis\":%d,\"humidifier_threshold\":%d,\"humidifier_hysteresis\":%d,\"pump_ms\":%lu,\"led\":%lu}",bmpOK?"true":"false",lightOK?"true":"false",dhtOK?"true":"false",safeTemp,safeDhtTemp,safeHumidity,safeLux,soilPct,soilRaw,safeTds,pump1On?"true":"false",pump2On?"true":"false",fanOn?"true":"false",ledOn?"true":"false",dehumidifierOn?"true":"false",humidifierOn?"true":"false",heaterOn?"true":"false",settings.autoWater?"true":"false",settings.autoLight?"true":"false",settings.autoFan?"true":"false",settings.autoPump2?"true":"false",settings.pump2WhenHigh?"true":"false",settings.autoDehumidifier?"true":"false",settings.autoHumidifier?"true":"false",settings.autoHeater?"true":"false",settings.moistureThreshold,settings.lightThresholdLux,settings.fanThresholdC,settings.heaterThresholdC,settings.heaterHysteresis,settings.tdsThresholdPpm,(unsigned long)settings.pump2Ms,settings.humidityThreshold,settings.humidityHysteresis,settings.humidifierThreshold,settings.humidifierHysteresis,(unsigned long)settings.pumpMs,(unsigned long)settings.led);return String(o);}

const char DASH_HTML[] PROGMEM=R"HTML(<!doctype html><meta name=viewport content="width=device-width,initial-scale=1"><title>Barometer Station</title><style>body{font:16px system-ui;max-width:46rem;margin:2rem auto;padding:0 1rem;background:#0f172a;color:#e2e8f0}.card,.w{background:#1e293b;padding:1rem;border-radius:.7rem;margin:.8rem 0}.g{display:grid;grid-template-columns:repeat(auto-fit,minmax(9rem,1fr));gap:.7rem}.w{margin:0;border-left:4px solid #38bdf8}.l{color:#94a3b8;font-size:.75rem;text-transform:uppercase}.v{font-size:1.55rem;font-weight:700}.d{color:#cbd5e1;font-size:.82rem}.yes{color:#86efac}.no{color:#fca5a5}input,button,a,select{font:inherit;margin:.25rem;padding:.35rem}label{display:block;margin:.4rem 0}a{display:inline-block;text-decoration:none;background:#334155;color:#fff}.go{background:#166534}.stop{background:#991b1b}</style><h1>Barometer Station</h1><p>Local dashboard — no internet required.</p><section class=g id=state>Loading…</section><div class=card><h2>Automatic controls</h2><form action=/settings><label>Water Pump 1 below % <input name=moisture type=number min=0 max=100></label><label>Pump 1 time (ms) <input name=pumpms type=number min=1 max=30000></label><label><input name=water type=checkbox> Auto Pump 1</label><label>Pump 2 TDS threshold (ppm) <input name=tds type=number min=0 max=5000></label><label>Pump 2 dose time (ms) <input name=pump2ms type=number min=1 max=30000></label><label>Pump 2 when TDS <select name=pump2mode><option value=low>below</option><option value=high>above</option></select></label><label><input name=pump2auto type=checkbox> Auto Pump 2 (10 min cooldown)</label><label>LED below lux <input name=lux type=number min=0 max=100000></label><label>LED color <input name=led type=color></label><label><input name=light type=checkbox> Auto LED</label><label>Fan on at °C <input name=temp type=number min=0 max=80></label><label><input name=fan type=checkbox> Auto Fan 1</label><label>Heater below °C <input name=heattemp type=number min=0 max=80></label><label>Heater gap °C <input name=heathyst type=number min=1 max=20></label><label><input name=heaterauto type=checkbox> Auto heater</label><label>Dehumidifier at % RH <input name=humidity type=number min=0 max=100></label><label>Dehumidifier gap % <input name=hyst type=number min=1 max=20></label><label>Humidifier on below % RH <input name=humidifier type=number min=0 max=100></label><label>Humidifier turn-off gap % <input name=humidifierhyst type=number min=1 max=20></label><label><input name=humidifierauto type=checkbox> Auto humidifier</label><label><input name=dehumidifierauto type=checkbox> Auto dehumidifier</label><button>Save automatic settings</button></form></div><div class=card><h2>Manual actuator control</h2><p>Manual ON/OFF disables that actuator’s automatic rule.</p><p>Pump 1: <a class=go href=/pump1?state=on>ON</a><a class=stop href=/pump1?state=off>OFF</a> Pump 2: <a class=go href=/pump2?state=on>ON</a><a class=stop href=/pump2?state=off>OFF</a></p><p>Fan 1: <a class=go href=/fan?state=on>ON</a><a class=stop href=/fan?state=off>OFF</a> LED: <a class=go href=/led?state=on>ON</a><a class=stop href=/led?state=off>OFF</a></p><p>Dehumidifier: <a class=go href=/dehumidifier?state=on>ON</a><a class=stop href=/dehumidifier?state=off>OFF</a> Humidifier: <a class=go href=/humidifier?state=on>ON</a><a class=stop href=/humidifier?state=off>OFF</a> Heater: <a class=go href=/heater?state=on>ON</a><a class=stop href=/heater?state=off>OFF</a></p></div><script>let box=document.getElementById('state');function w(n,v,x){return `<article class=w><div class=l>${n}</div><div class=v>${v}</div><div class=d>${x}</div></article>`}function s(n,on,a){return w(n,`<span class=${on?'yes':'no'}>${on?'ON':'OFF'}</span>`,a?'automatic':'manual')}async function refresh(){try{let r=await fetch('/data'),d=await r.json();box.innerHTML=w('Air temperature',`${d.dht_temp} °C`,d.dht_ok?'DHT22 online':'DHT22 not detected')+w('Humidity',`${d.humidity}% RH`,d.auto_dehumidifier?`dehumidifier at ${d.humidity_threshold}%`:'dehumidifier manual')+w('Light',`${d.lux} lx`,d.light_ok?'BH1750 online':'BH1750 not detected')+w('Soil moisture',`${d.soil}%`,`raw ${d.soil_raw} · target ${d.moisture}%`)+w('TDS',`${d.tds} ppm`,`${d.pump2_high?'above':'below'} ${d.tds_threshold} ppm`)+w('BMP085',`${d.temp} °C`,d.bmp_ok?'online':'not detected')+s('Pump 1',d.pump1,d.auto_water)+s('Pump 2',d.pump2,d.auto_pump2)+s('Fan 1',d.fan,d.auto_fan)+s('LED',d.led_on,d.auto_light)+s('Dehumidifier',d.dehumidifier,d.auto_dehumidifier)+s('Humidifier',d.humidifier,d.auto_humidifier)+s('Heater',d.heater,d.auto_heater);let m={moisture:'moisture',pumpms:'pump_ms',tds:'tds_threshold',pump2ms:'pump2_ms',lux:'light_lux',temp:'fan_temp',heattemp:'heater_temp',heathyst:'heater_hysteresis',humidity:'humidity_threshold',hyst:'humidity_hysteresis',humidifier:'humidifier_threshold',humidifierhyst:'humidifier_hysteresis'};for(let n in m)document.querySelector(`[name=${n}]`).value=d[m[n]];for(let n of ['water','light','fan'])document.querySelector(`[name=${n}]`).checked=d['auto_'+n];document.querySelector('[name=pump2auto]').checked=d.auto_pump2;document.querySelector('[name=dehumidifierauto]').checked=d.auto_dehumidifier;document.querySelector('[name=humidifierauto]').checked=d.auto_humidifier;document.querySelector('[name=heaterauto]').checked=d.auto_heater;document.querySelector('[name=pump2mode]').value=d.pump2_high?'high':'low';document.querySelector('[name=led]').value='#'+Number(d.led).toString(16).padStart(6,'0')}catch(e){box.innerHTML='<div class=w>Unable to read ESP32 data. Refresh the page.</div>'}}refresh();setInterval(refresh,2000)</script>)HTML";
void handleSettings() {
  if (server.hasArg("moisture")) settings.moistureThreshold = constrain(server.arg("moisture").toInt(), 0, 100);
  if (server.hasArg("pumpms")) settings.pumpMs = constrain((uint32_t)server.arg("pumpms").toInt(), 1UL, MAX_PUMP_MS);
  if (server.hasArg("tds")) settings.tdsThresholdPpm = constrain(server.arg("tds").toInt(), 0, 5000);
  if (server.hasArg("pump2ms")) settings.pump2Ms = constrain((uint32_t)server.arg("pump2ms").toInt(), 1UL, MAX_PUMP_MS);
  if (server.hasArg("lux")) settings.lightThresholdLux = constrain(server.arg("lux").toInt(), 0, 100000);
  if (server.hasArg("temp")) settings.fanThresholdC = constrain(server.arg("temp").toInt(), 0, 80);
  if (server.hasArg("heattemp")) settings.heaterThresholdC = constrain(server.arg("heattemp").toInt(), 0, 80);
  if (server.hasArg("heathyst")) settings.heaterHysteresis = constrain(server.arg("heathyst").toInt(), 1, 20);
  if (server.hasArg("humidity")) settings.humidityThreshold = constrain(server.arg("humidity").toInt(), 0, 100);
  if (server.hasArg("hyst")) settings.humidityHysteresis = constrain(server.arg("hyst").toInt(), 1, 20);
  if (server.hasArg("led")) {
    String color = server.arg("led");
    color.replace("#", "");
    settings.led = strtoul(color.c_str(), nullptr, 16);
  }
  settings.autoWater = server.hasArg("water");
  settings.autoLight = server.hasArg("light");
  settings.autoFan = server.hasArg("fan");
  settings.autoPump2 = server.hasArg("pump2auto");
  settings.pump2WhenHigh = server.arg("pump2mode") == "high";
  settings.autoDehumidifier = server.hasArg("dehumidifierauto");
  settings.autoHumidifier = server.hasArg("humidifierauto");
  settings.autoHeater = server.hasArg("heaterauto");
  saveSettings();
  server.sendHeader("Location", "/");
  server.send(303);
}
void redirectHome(){server.sendHeader("Location","/");server.send(303);}
void registerRoutes(){server.on("/",HTTP_GET,[]{server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html",DASH_HTML);});server.on("/data",HTTP_GET,[]{server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",stateJson());});server.on("/generate_204",HTTP_GET,redirectHome);server.on("/hotspot-detect.html",HTTP_GET,redirectHome);server.on("/connecttest.txt",HTTP_GET,redirectHome);server.on("/ncsi.txt",HTTP_GET,redirectHome);server.on("/settings",HTTP_GET,handleSettings);server.on("/pump1",HTTP_GET,[]{settings.autoWater=false;server.arg("state")=="off"?stopPump1():startPump1(settings.pumpMs);saveSettings();redirectHome();});server.on("/pump2",HTTP_GET,[]{settings.autoPump2=false;server.arg("state")=="off"?stopPump2():startPump2(settings.pump2Ms);saveSettings();redirectHome();});server.on("/fan",HTTP_GET,[]{settings.autoFan=false;setFan(server.arg("state")!="off");saveSettings();redirectHome();});server.on("/led",HTTP_GET,[]{settings.autoLight=false;showLed(server.arg("state")!="off");saveSettings();redirectHome();});server.on("/dehumidifier",HTTP_GET,[]{settings.autoDehumidifier=false;setDehumidifier(server.arg("state")!="off");saveSettings();redirectHome();});server.on("/humidifier",HTTP_GET,[]{settings.autoHumidifier=false;setHumidifier(server.arg("state")!="off");saveSettings();redirectHome();});server.on("/heater",HTTP_GET,[]{settings.autoHeater=false;setHeater(server.arg("state")!="off");saveSettings();redirectHome();});server.onNotFound(redirectHome);}
void setup(){Serial.begin(115200);pinMode(PUMP1_PIN,OUTPUT);pinMode(PUMP2_PIN,OUTPUT);pinMode(FAN1_PIN,OUTPUT);pinMode(DEHUMIDIFIER_PIN,OUTPUT);pinMode(HUMIDIFIER_PIN,OUTPUT);pinMode(HEATER_PIN,OUTPUT);stopPump1();stopPump2();setFan(false);setDehumidifier(false);setHumidifier(false);setHeater(false);loadSettings();analogReadResolution(12);analogSetPinAttenuation(SOIL_PIN,ADC_11db);analogSetPinAttenuation(TDS_PIN,ADC_11db);Wire.begin(21,22);bmpOK=bmp.begin();lightOK=lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE,0x23,&Wire);dht.begin();pixel.begin();showLed(false);WiFi.mode(WIFI_AP);WiFi.softAPConfig(AP_IP,AP_IP,IPAddress(255,255,255,0));WiFi.softAP(AP_SSID,strlen(AP_PASS)?AP_PASS:NULL);dnsServer.start(53,"*",AP_IP);registerRoutes();server.begin();Serial.println("Wi-Fi: BarometerSetup  Website: http://192.168.4.1");}
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  uint32_t now = millis();

  if (pump1On && (int32_t)(now - pump1StopAt) >= 0) stopPump1();
  if (pump2On && (int32_t)(now - pump2StopAt) >= 0) stopPump2();
  if (now - lastSensorMs < SENSOR_INTERVAL_MS) return;
  lastSensorMs = now;
  if (!bmpOK) bmpOK = bmp.begin();

  readSensors();
  if (settings.autoLight && lightOK && !isnan(lightLux)) showLed(lightLux < settings.lightThresholdLux);
  if (settings.autoFan && !isnan(tempC)) setFan(tempC >= settings.fanThresholdC);
  if (settings.autoWater && !pump1On && soilPct < settings.moistureThreshold) startPump1(settings.pumpMs);

  bool tdsNeedsDose = !isnan(tdsPpm) &&
    ((settings.pump2WhenHigh && tdsPpm > settings.tdsThresholdPpm) ||
     (!settings.pump2WhenHigh && tdsPpm < settings.tdsThresholdPpm));
  if (settings.autoPump2 && !pump2On && tdsNeedsDose &&
      (lastPump2DoseAt == 0 || now - lastPump2DoseAt >= PUMP2_MIN_INTERVAL_MS)) {
    startPump2(settings.pump2Ms);
    lastPump2DoseAt = now;
  }

  if (settings.autoDehumidifier && dhtOK) {
    if (humidity >= settings.humidityThreshold) setDehumidifier(true);
    else if (humidity <= settings.humidityThreshold - settings.humidityHysteresis) setDehumidifier(false);
  }
  if (settings.autoHumidifier && dhtOK) {
    if (humidity <= settings.humidifierThreshold) setHumidifier(true);
    else if (humidity >= settings.humidifierThreshold + settings.humidifierHysteresis) setHumidifier(false);
  }
  if (settings.autoHeater && dhtOK) {
    if (dhtTempC <= settings.heaterThresholdC) setHeater(true);
    else if (dhtTempC >= settings.heaterThresholdC + settings.heaterHysteresis) setHeater(false);
  }
}
