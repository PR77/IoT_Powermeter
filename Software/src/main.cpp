#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <LittleFS.h>

#include <OLEDDisplay.h>
#include <OLEDDisplayFonts.h>
#include <OLEDDisplayUi.h>
#include <Wire.h>
#include <SSD1306.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <DHTesp.h>

#include <clickButton.h>
#include <beeperControl.h>
#include <batteryHistogram.h>
#include <impulseCapture.h>

#include "uiGlobal.h"
#include "uiOverlay.h"
#include "uiFrameStatus.h"
#include "uiFrameSensor.h"
#include "uiFrameBattery.h"

//=============================================================================
// Types
//=============================================================================

//=============================================================================
// Hostname, SSID and Password for Wifi router
//=============================================================================
String staSSID(32);
String staPassword(32);

IPAddress apIPAddress(192,168,1,2);
IPAddress apGatwayAddress(192,168,1,1);
IPAddress apNetmask(255,255,255,0);

//=============================================================================
// Application specific defines and globals
//=============================================================================
const uint32_t blankUXAfterBootUp = 10L * (1000L);  // 10 seconds
const uint32_t blankUXAfterMessage = 10L;           // 10 seconds

static uint32_t lastDhtUpdateTime = 0;
static uint32_t lastLogUpdateTime = 0;
static uint32_t lastLogUpdateUiTime = 0;

unsigned long reconnectionTime;
bool logUpdate = false;

//=============================================================================
// Global constants for NTP Client
//=============================================================================
const uint32_t utcOffsetInSeconds = 3600;

//=============================================================================
// Global constants for Impulse Sensor interface, buttons, beeper interface, DHT11
//=============================================================================
#define BUILD_TIME                  ("__DATE__, __TIME__ ")

#define ACCESSORY_NAME              ("IoTPowerMeter")
#define ACCESSORY_SETUP_NAME        ("PowerMeterSetup")
#define ACCESSORY_SN                ("SN_0001")
#define ACCESSORY_MODEL             ("ESP8266")

#define RECONNECT_INTERVAL          5000
#define LOG_INTERVAL                10000   // 10 seconds in milli-seconds
#define LOG_UI_DISPLAY_TIME         500

const uint8_t SensorPin = 2;
const uint8_t MenuPin = 14;
const uint8_t EnterPin = 15;
const uint8_t BeeperPin = 13;
const uint8_t dhtSensorPin = 0;
const uint8_t DeepSleepPin = 16;

//=============================================================================
// Global objects for Wifi and ESP specifics
//=============================================================================
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiEventHandler onConnectedHandler;
WiFiEventHandler onGotIpHandler;
WiFiEventHandler onAccessPointConnectedHandler;
File fsUploadFile;

//=============================================================================
// OLED global object (https://github.com/ThingPulse/esp8266-oled-ssd1306)
//=============================================================================
SSD1306 display(0x3C, 4, 5, GEOMETRY_128_32);
OLEDDisplayUi ui(&display);

OverlayCallback overlays[] = {uiOverlay};
FrameCallback frames[] = {uiFrameStatus, uiFrameBattery, uiFrameSensor};
int overlaysCount = 1;
int frameCount = 3;

//=============================================================================
// Declare DHTxx object (DHT sensor library for ESPx by Bernd Giesecke)
//=============================================================================
DHTesp dht;
TempAndHumidity dhtTempAndHumidity;

//=============================================================================
// Global objects for NTP Client
//=============================================================================
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//=============================================================================
// Global objects for ClickButton object
//=============================================================================
ClickButton enterButton(EnterPin, HIGH);
ClickButton menuButton(MenuPin, LOW, CLICKBTN_PULLUP);

//=============================================================================
// Global objects for beeper object
//=============================================================================
BeeperControl beeper(BeeperPin);

//=============================================================================
// Global objects for battery object
//=============================================================================
BatteryHistogram battery(true);

//=============================================================================
// Global objects for impulse object
//=============================================================================
ImpulseCapture impulse(SensorPin);

//=============================================================================
// Global objects for UX
//=============================================================================
uiGlobalObject_s uiGlobalObject = {&dhtTempAndHumidity, &battery, &impulse, &logUpdate};

//=============================================================================
// Function prototypes
//=============================================================================
bool loadFromSpiffs(String path);
void handleRoot(void);
void handleFileList(void);
void handleFileUpload(void);
void handleFileDelete(void);
void handleWebRequests(void);
void handleBeeper(void);

//=============================================================================
// Helper function
//=============================================================================
bool loadFromSpiffs(String path) {
    String dataType = "text/plain";
    bool fileTransferStatus = false;

    // If a folder is requested, send the default index.html
    if (path.endsWith("/"))
        path += "index.htm";
    else if (path.endsWith(".html")) dataType = "text/html";
    else if (path.endsWith(".htm")) dataType = "text/html";
    else if (path.endsWith(".css")) dataType = "text/css";
    else if (path.endsWith(".js")) dataType = "application/javascript";
    else if (path.endsWith(".png")) dataType = "image/png";
    else if (path.endsWith(".gif")) dataType = "image/gif";
    else if (path.endsWith(".jpg")) dataType = "image/jpeg";
    else if (path.endsWith(".ico")) dataType = "image/x-icon";
    else if (path.endsWith(".xml")) dataType = "text/xml";
    else if (path.endsWith(".pdf")) dataType = "application/pdf";
    else if (path.endsWith(".zip")) dataType = "application/zip";

    File dataFile = LittleFS.open(path.c_str(), "r");

    if (!dataFile) {
        return fileTransferStatus;
    }
    else {
        if (httpServer.streamFile(dataFile, dataType) == dataFile.size()) {
            fileTransferStatus = true;
        }
    }

    dataFile.close();
    return fileTransferStatus;
}

//=============================================================================
// Webserver event handlers
//=============================================================================
void handleRoot(void) {
    httpServer.sendHeader("Location", "/index.html", true);

    // Redirect to our index.html web page
    httpServer.send(302, "text/plain", "");
}

void handleFileList(void) {
    // curl -X GET ACCESSORY_NAME.local/list

    String path = "/";
    String directoryList = "[";
    Dir directoryEntry = LittleFS.openDir(path);

    while (directoryEntry.next()) {
        File fileElement = directoryEntry.openFile("r");

        if (directoryList != "[")
            directoryList += ", ";

        directoryList += String(fileElement.name()).substring(0);
        fileElement.close();
    }

    directoryList += "]";
    httpServer.send(200, "text/plain", directoryList);
}

void handleFileUpload(void) {
    // curl -X POST -F "file=@SomeFile.EXT" ACCESSORY_NAME.local/upload

    HTTPUpload& upload = httpServer.upload();

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;

        if (!filename.startsWith("/"))
            filename = "/" + filename;

        fsUploadFile = LittleFS.open(filename, "w");

    } else if (upload.status == UPLOAD_FILE_WRITE) {

        if(fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);

    } else if (upload.status == UPLOAD_FILE_END) {

        if(fsUploadFile)
            fsUploadFile.close();
    }
}

void handleFileDelete(void) {

    String fileDeleteResponse = String();
    String fileName = httpServer.arg(0);

    if (LittleFS.exists(fileName)) {
        if (LittleFS.remove(fileName)) {
            fileDeleteResponse = "{\"success\":1}";
        }
        else {
            fileDeleteResponse = "{\"file does not exist\":1}";
        }
    }
    else {
        fileDeleteResponse = "{\"unable to open file\":1}";
    }

    httpServer.send(200, "text/plain", fileDeleteResponse);
}

void handleWebRequests(void) {
    String message = "File Not Detected\n\n";

    if (loadFromSpiffs(httpServer.uri()))
        return;

    message += "URI: ";
    message += httpServer.uri();
    message += "\nMethod: ";
    message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += httpServer.args();
    message += "\n";

    for (uint8_t i = 0; i < httpServer.args(); i++) {
        message += " NAME:" + httpServer.argName(i);
        message += "\n VALUE:" + httpServer.arg(i) + "\n";
    }

    httpServer.send(404, "text/plain", message);
}

void handleBeeper() {
    // curl -X POST ACCESSORY_NAME.local/beeper?count={BEEPS}"

    String beeperRequestResponse = String();

    if (beeper.GetBeeperState() == beepHandlerIdle) {
        if (httpServer.hasArg("count")) {
            beeper.RequestBeeper(httpServer.arg("count").toInt());

            beeperRequestResponse = "{\"success\":1}";
        } else {
            beeperRequestResponse = "{\"no count specified\":1}";
        }
    }
    else {
        beeperRequestResponse = "{\"busy\":1}";
    }

    httpServer.send(200, "text/plain", beeperRequestResponse);
}

//==============================================================
// WiFi function
//==============================================================
void onAccessPointConnected(const WiFiEventSoftAPModeStationConnected& event) {
    // Start servers
    httpServer.begin();
    Serial.printf("Webserver running\n");
}

void onConnected(const WiFiEventStationModeConnected& event) {
    Serial.printf("Connected to: %s\n", event.ssid.c_str());

    // Start servers
    httpServer.begin();
    Serial.printf("Webserver running\n");

    httpUpdater.setup(&httpServer);
    Serial.printf("Update server running\n");
}

void onGotIp(const WiFiEventStationModeGotIP& event) {
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

    if (MDNS.begin(ACCESSORY_NAME)) {
        Serial.printf("mDNS service running as: %s\n", ACCESSORY_NAME);
    }
    else {
        Serial.printf("Could not start mDNS service\n");
    }
    
    // Attached to NTP Client
    timeClient.begin();
}

void connectWiFi(void) {
    if (WiFi.getMode() == WIFI_AP)
        return;
    
    if (WiFi.status() == WL_CONNECTED || millis() < reconnectionTime) {
        return;
    }
    else {
        reconnectionTime = millis() + RECONNECT_INTERVAL;
    }

    // Reconnect
    Serial.printf("Connecting WiFi\n");
    WiFi.begin(staSSID, staPassword);
}

//=============================================================================
// Setup function
//=============================================================================
void setup() {
    File fsWifiConfig;
    bool apMode = false;

    // Initialise serial object
    Serial.begin(115200);

    // Initialize deep sleep pin to allow for wakeup
    pinMode(DeepSleepPin, OUTPUT);
    digitalWrite(DeepSleepPin, HIGH);

    // Initialise battery charge state GetBatteryHistogram
    battery.Init();

    // Initialse impulse capturing
    impulse.Init();

    // Initialise OLED display driver
    display.init();
    display.resetDisplay();
    display.flipScreenVertically();

    // Initilase Serial Communication
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
    Serial.setDebugOutput(false);

    // Build environment
    printf("\n");
    printf("SketchSize: %d B\n", ESP.getSketchSize());
    printf("FreeSketchSpace: %d B\n", ESP.getFreeSketchSpace());
    printf("FlashChipSize: %d B\n", ESP.getFlashChipSize());
    printf("FlashChipRealSize: %d B\n", ESP.getFlashChipRealSize());
    printf("FlashChipSpeed: %d\n", ESP.getFlashChipSpeed());
    printf("SdkVersion: %s\n", ESP.getSdkVersion());
    printf("FullVersion: %s\n", ESP.getFullVersion().c_str());
    printf("CpuFreq: %dMHz\n", ESP.getCpuFreqMHz());
    printf("FreeHeap: %d B\n", ESP.getFreeHeap());
    printf("ResetInfo: %s\n", ESP.getResetInfo().c_str());
    printf("ResetReason: %s\n", ESP.getResetReason().c_str());
    printf("BuildDetails: %s, %s\n", __DATE__, __TIME__ );

    // Initialize File System.
    LittleFS.begin();

    // AP if no wifi.config file exists.
    if (!LittleFS.exists("/wifi.conf")) {
        apMode = true;
    }

    if (apMode == true) {
        Serial.printf("WiFi AP mode active\n");

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIPAddress, apGatwayAddress, apNetmask);
        onAccessPointConnectedHandler = WiFi.onSoftAPModeStationConnected(onAccessPointConnected);
        WiFi.softAP(ACCESSORY_SETUP_NAME);
    } else {
        fsWifiConfig = LittleFS.open("/wifi.conf", "r");

        staSSID = fsWifiConfig.readStringUntil(',');
        staPassword = fsWifiConfig.readStringUntil(',');
        fsWifiConfig.close();

        onConnectedHandler = WiFi.onStationModeConnected(onConnected);
        onGotIpHandler = WiFi.onStationModeGotIP(onGotIp);
        WiFi.mode(WIFI_STA);
        WiFi.hostname(ACCESSORY_NAME);
    }

    // Setup UI
    ui.setTargetFPS(10);
    ui.disableAllIndicators();
    ui.disableAutoTransition();
    ui.setOverlays(overlays, overlaysCount);
    ui.setFrames(frames, frameCount);
    ui.getUiState()->userData = (void *)&uiGlobalObject;
    ui.init();
    display.flipScreenVertically();

    // Setup DHT11 interface
    dht.setup(dhtSensorPin, DHTesp::DHT11);

    // Assign server helper functions
    httpServer.on("/", handleRoot);
    httpServer.on("/list", HTTP_GET, handleFileList);
 
    httpServer.on("/upload", HTTP_POST, []() {
        httpServer.send(200, "text/plain", "{\"success\":1}");
    }, handleFileUpload);

    httpServer.on("/delete", HTTP_DELETE, handleFileDelete);

    httpServer.on("/format", HTTP_POST, []() {
        LittleFS.format();
        httpServer.send(200, "text/plain", "{\"success\":1}");
    });

    httpServer.on("/reset", HTTP_POST, []() {
        ESP.reset();
        httpServer.send(200, "text/plain", "{\"success\":1}");
    });

    httpServer.on("/info", HTTP_GET, []() {
        String spiffsInfo = String();
        FSInfo fsInfo;

        LittleFS.info(fsInfo);
        spiffsInfo = "{";
        spiffsInfo += "\"NVMSize\":" + String(fsInfo.totalBytes) + ",";
        spiffsInfo += "\"UsedBytes\":" + String(fsInfo.usedBytes) + ",";
        spiffsInfo += "\"FlashSize\":" + String(ESP.getFlashChipRealSize()) + ",";
        spiffsInfo += "\"CPUSpeed\":" + String(ESP.getCpuFreqMHz());
        spiffsInfo += "}";
        httpServer.send(200, "text/plain", spiffsInfo);
    });

    httpServer.on("/temperature", HTTP_GET, []() {
        String dht11TempData = String();

        dht11TempData = "{";
        dht11TempData += "\"temperature\":" + String(dhtTempAndHumidity.temperature);
        dht11TempData += "}";
        httpServer.send(200, "text/plain", dht11TempData.c_str());
    });

    httpServer.on("/humidity", HTTP_GET, []() {
        String dht11HumData = String();

        dht11HumData = "{";
        dht11HumData += "\"humidity\":" + String(dhtTempAndHumidity.humidity);
        dht11HumData += "}";
        httpServer.send(200, "text/plain", dht11HumData.c_str());
    });

    httpServer.on("/watts", HTTP_GET, []() {
        String wattsData = String();

        wattsData = "{";
        wattsData += "\"watts\":" + String(impulse.GetInstantWattUsgage());
        wattsData += "}";
        httpServer.send(200, "text/plain", wattsData.c_str());
    });

    httpServer.on("/beeper", HTTP_POST, handleBeeper);

    httpServer.onNotFound(handleWebRequests);

    lastDhtUpdateTime = millis();
}

//=============================================================================
// Loop function
//=============================================================================
void loop() {

    static bool displayState = true;
    uint32_t currentTime = millis();
    uint16_t uiRemainingBudget = 0;

    connectWiFi();
    MDNS.update();
    httpServer.handleClient();

    enterButton.Update();
    menuButton.Update();

    if (menuButton.clicks > 0) {
        ui.nextFrame();
    } else if (menuButton.clicks < 0) {
        ui.previousFrame();
    }

    if (enterButton.clicks > 0) {
        if (displayState == true) {
            display.displayOff();
            displayState = false;
        } else {
            display.displayOn();
            displayState = true;
        }
    }
    
    if (enterButton.clicks < 0) {
        impulse.ClearInstantWattUsage();
        if (LittleFS.exists("log.csv")) {
            LittleFS.remove("log.csv");
        }
    }

    uiRemainingBudget = ui.update();

    if (uiRemainingBudget > 0) {
        
        beeper.Update();
        battery.Update();
        impulse.Update();
        
        if ((currentTime - lastLogUpdateTime) >=  LOG_INTERVAL) {
            timeClient.update();

            if (timeClient.isTimeSet() == true) {
                File fsLog =  LittleFS.open("/log.csv", "a");
                fsLog.printf("%s", String(timeClient.getEpochTime()).c_str());
                fsLog.print(',');
                fsLog.println(String(impulse.GetInstantWattUsgage()).c_str());
                fsLog.close();
            
                logUpdate = true;
                lastLogUpdateUiTime = currentTime;
            }

            lastLogUpdateTime = currentTime;
        }

        if ((logUpdate == true) && ((currentTime - lastLogUpdateUiTime) > LOG_UI_DISPLAY_TIME)) {
            logUpdate = false;
        }

        if ((currentTime - lastDhtUpdateTime) >=  2000) {
            dhtTempAndHumidity = dht.getTempAndHumidity();

            lastDhtUpdateTime = currentTime;
        }
    }
}
