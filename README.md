# IoT_PowerMeter
IoT Powermeter for simple electricity usage measurement.

# Warning
This design has not been compliance tested and will not be. It is published purely for inspiration to others. I take no responsibility for damages caused directly or indirectly. I accept no responsibility for any damage to any equipment that results from the use of this design and its components. IT IS ENTIRELY AT YOUR OWN RISK!

# Overview
My IoT_Powermeter can be used to monitor electricity usage measured by a consumer unit with an IR LED generating an Impulse.

## Basic Concept
My IoT Power Meter is essentially my IoT_Messenger but re-purposed. Interfacing with the device is via the Web using the simple HTTP Server class. The IoT Powermeter sends out a webpage where the instantenous energy usage can be plotted. Data is locally save to the LittleFS. 

Here is a nice 3d model of the PCB;

### Appearance
Nice 3D model:
![3DModel](/Images/IoTMessenger.png)

### Basic Interfaces
My IoT Powermeter needs to be portable therefore battery power and a basic set of controls are essential. I use a 600 mA LiPo cell which is charged via USB. USB provides power only. No intelligent power management has been implemented yet, so when running only via battery it last 4 - 5 hours.

There are 4 buttons for user input which can also be used for additional functions. For this application only 2 buttons are used (MENU and ENTER).

Power usage measurement is via a widely available "Flame Sensor" which I use to measure IR impulses from my consumer unit.

### Power Supply
I use the MCP73831 LiPo battery charger with a P-Channel load balancing FET. This is to ensure the LiPo charger only charges the battery when connected to a USB power source. The necessary 3.3 volts is obtained via a basic LM1117-3.3.

### Sensors And Indicators
I wanted to have the PCB somewhat universal - I guess like an evaluation or child friendly development board - so I added a nice RGB LED (PL9823) and an environmental sensor (DHT11). In addition to these, as the device is inspired by pagers, it has a OLED display (128x32) and Piezo Beeper. For this application, the RGB LED is not used, rather the GPIO is connected to the Flame Sensor digital (pulse) output.

As mentioned, flexibility was important so with the OLED, it is essentially dual-footprinted where a module with necessary passives could just be solder on, or the raw OLED panel and necessary passives be soldered on the IoT Messenger board. For my prototype I used the latter so get some experience with this. Lastly, I added a SAO port, well for - Add Ons.

Nice fully soldered IoT Power Meter:
![FullySoldered](/Images/FullySoldered.jpg)

### Server
All of the interfacing with the hardware features is via HTTP Server GET and POST requests. The following services have been implemented currently;

|Request      |Type|Parameters|Comments                              |
|-------------|----|----------|--------------------------------------|
|/list        |GET |none      |Lists files currently saved to SPIFFS |
|/upload      |POST|filename  |Uploads a file to SPIFFS              |
|/format      |POST|none      |Formats SPIFFS                        |
|/info        |GET |none      |Get SPIFFS and CPU MHz info           |
|/temperature |GET |none      |Read environmental sensor data (DHT11)|
|/humidity    |GET |none      |Read environmental sensor data (DHT11)|
|/ledwatts    |GET |none      |Instandenous watts                    |
|/beeper      |POST|count     |Beep piezo beeper                     |


## Open Sources Used
PlatformIO is the main development environment. In addition to the Arduino framework for ESP8266, I used the following (either important as libraries into PIO or seperate);

1. OLED library (https://github.com/ThingPulse/esp8266-oled-ssd1306)
2. ClickButton library (https://github.com/pkourany/clickButton)
3. Environmental sensor library (https://github.com/beegee-tokyo/DHTesp)
