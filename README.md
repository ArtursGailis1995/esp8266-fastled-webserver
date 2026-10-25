FastLED + ESP8266 Web Server
=========

Control an addressable RGB LED strip with an ESP8266 via a web browser.

Tested with **ESP8266 Arduino core 3.1.2**, **FastLED 3.10.8** and **Arduino IDE 2** on a NodeMCU 1.0 (ESP-12E) board.

## Parts used for this project

* ESP8266 NodeMCU V3 development board<br/>
<img src="/images/NodeMCU_ESP8266.jpg" alt="ESP8266" width="350"/>

* WS2812b or other addressable RGB LED strip (personally using 5 meters with 300 LEDs total)<br/>
<img src="/images/WS2812b_LED.jpg" alt="WS2812b" width="350"/>

Features
--------
* Turn LED strip on or off
* Adjust the brightness
* Enable automatic pattern switching
* Adjust pattern speed and intensity
* Sweep built-in gradients
* Set autoplay duration
* Change the pattern
* Adjust the color
* Use full or simple control mode
* Discover and control several controllers on the network at the same time
* Directly edit the LittleFS file system contents (HTML, JS, CSS)
* Upload new firmware and file system over the network, protected with a password
* Many, many more!

Web App
--------

The web app uses [Bootstrap 5](https://getbootstrap.com) with the [Bootswatch Flatly](https://bootswatch.com/flatly/) theme, [jQuery](https://jquery.com) and [jQuery MiniColors](https://github.com/claviska/jquery-minicolors) for the color picker. It uses system fonts and inline icons, so it works without internet access (only the file system editor loads the [Ace editor](https://ace.c9.io) from a CDN).

Event handlers for the controls are used, so you don't have to click a 'Send' button after making changes to the LED strip parameters. Sliders and the color picker use a delayed event handler, to prevent flooding the ESP8266 web server with too many requests. Changes are pushed to all open browser windows over a WebSocket. The web app is stored in the LittleFS file system (on-board flash memory).

Screenshots below show the previous version of the interface, the layout is the same.

* **Main page provides access to all features related to controlling RGB LED strip parameters and features**<br/>
<img src="/images/WEBApp_Main.jpg" alt="WEB App Main window" width="850"/>

* **Simple mode allows to control power state, change active mode or set static color of RGB LED strip**<br/>
<img src="/images/WEBApp_Simple.jpg" alt="WEB App Simple window" width="850"/>

* **Firmware update page allows to upload precompiled firmware to ESP8266 in order to update it without connecting to PC**<br/>
<img src="/images/WEBApp_FW.jpg" alt="WEB App Firmware update window" width="850"/>

* **Information page shows Wi-Fi status and lists recent changes to this project**<br/>
<img src="/images/WEBApp_Info.jpg" alt="WEB App Information window" width="850"/>

Installing
-----------
The app is installed via the Arduino IDE which can be [downloaded here](https://www.arduino.cc/en/software). The ESP8266 boards need to be added to the Arduino IDE:

- Click `File` > `Preferences`
- Copy and paste the URL `https://arduino.esp8266.com/stable/package_esp8266com_index.json` into the `Additional boards manager URLs` field
- Click `OK`
- Open `Tools` > `Board` > `Boards Manager`
- Find `esp8266` by ESP8266 Community and click `Install` (version 3.1.2 is tested)
- Select `NodeMCU 1.0 (ESP-12E Module)` (or your board) in `Tools` > `Board` > `esp8266`
- Keep the default `Tools` > `Flash Size` > `4MB (FS:2MB OTA:~1019KB)`

Install the following libraries with the Arduino Library Manager (`Tools` > `Manage Libraries...`):

| Library | Tested version |
|---|---|
| [FastLED](https://github.com/FastLED/FastLED) | 3.10.8 |
| [WebSockets](https://github.com/Links2004/arduinoWebSockets) by Markus Sattler | 2.7.2 |
| [WiFiManager](https://github.com/tzapu/WiFiManager) by tzapu | 2.0.17 |

Download the app code from GitHub using the green `Code` button on [the GitHub project main page](https://github.com/ArtursGailis1995/esp8266-fastled-webserver) and click `Download ZIP`, or clone the repository.

### Configuration

1. **Admin password.** Copy `secrets.example.h` to `secrets.h` (in the same folder as the `.ino` file) and set your own `ADMIN_USER` and `ADMIN_PASSWORD`. They protect the firmware update page, the file system editor, Wi-Fi reset and Arduino OTA uploads. `secrets.h` is listed in `.gitignore`, so your password is never committed. Without `secrets.h` the sketch compiles with the default `admin` / `changeme` and prints a warning.
2. **LED strip.** Set these values in `esp8266-fastled-webserver.ino`:

        #define DATA_PIN      D2      // pin labelled D2 on the NodeMCU board
        #define LED_TYPE      WS2812B
        #define COLOR_ORDER   GRB
        #define NUM_LEDS      300
        #define MILLI_AMPS    18000   // max current of your power supply in mA

    With FastLED 3.10 and newer, `D2` in the code is the pin labelled `D2` on the board (older FastLED versions needed a different number, ignore old instructions saying otherwise). The pin mapping is shown below.

<img src="/images/GPIO_Explained.png" alt="GPIO of ESP8266 explained" width="650"/>

### Uploading the firmware

Connect the board over USB, select its port in `Tools` > `Port` and click `Upload`.

Later updates can be done over Wi-Fi: select the controller under `Network ports` in `Tools` > `Port` (the IDE asks for `ADMIN_PASSWORD`), or export a `.bin` file with `Sketch` > `Export Compiled Binary` and upload it on the **Firmware & Wi-Fi** page. The exported file is saved in the sketch folder as `build/esp8266.esp8266.nodemcuv2/esp8266-fastled-webserver.ino.bin`.

### Uploading the web app (LittleFS)

The web app in the `data` folder must be uploaded to the LittleFS file system once, and again after every change in `data`. Choose one of the options:

* **USB:** install the [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload) plugin for Arduino IDE 2, close the Serial Monitor, press `Ctrl` + `Shift` + `P` and run `Upload LittleFS to Pico/ESP8266/ESP32`.
* **Network:** build an image with the `mklittlefs` tool, which is installed with the ESP8266 board package (on Windows in `%LOCALAPPDATA%\Arduino15\packages\esp8266\tools\mklittlefs`). Run it in the sketch folder, the values are for the default `4MB (FS:2MB)` layout:

        mklittlefs -c data -p 256 -b 8192 -s 2072576 littlefs-4M2M.bin

    Then upload `littlefs-4M2M.bin` as `File system` on the **Firmware & Wi-Fi** page.

To keep the file system when uploading firmware over USB, keep `Tools` > `Erase Flash` > `Only Sketch` selected.

The web server also serves gzip compressed files: if only `file.js.gz` exists, it is sent when the browser asks for `file.js`. Files in this repository are kept uncompressed for readability, you can gzip large libraries (for example `data/js/jquery.min.js`) before building the image to save flash space and load pages faster.

### Upgrading from an older version

Older versions stored the web app in SPIFFS. After uploading the new firmware the old file system cannot be read and the web interface is empty. Open `http://<controller IP>/update`, log in with your admin credentials and upload a LittleFS image of the `data` folder (or use the USB upload). Refresh the pages with `Ctrl` + `F5` afterwards.

### First start

A Wi-Fi hotspot named `ESP8266-XXXX` (the end of the controller MAC address) is started after the first upload. Connect to it, open `http://192.168.4.1` if it does not open automatically, choose your Wi-Fi network and enter its password, then click `Save`. After connecting to your home Wi-Fi the hotspot is disabled. The IP address of the controller is printed in the serial output (115200 baud) and is also visible in your router as `ESP8266-XXXX`. You can also open `http://esp8266-xxxx.local` on devices that support mDNS.

REST Web services
-----------------

The firmware implements basic [RESTful web services](https://en.wikipedia.org/wiki/Representational_state_transfer) using the `ESP8266WebServer` library. Current values are requested with `HTTP GET`, and values are set with `HTTP POST` using query string or form parameters. Values outside the allowed range are limited to the nearest valid value.

| Endpoint | Method | Parameters |
|---|---|---|
| `/all` | GET | Returns all fields and values as JSON |
| `/power` | POST | `value` 0 or 1 |
| `/brightness` | POST | `value` 1-255 |
| `/pattern`, `/patternName` | POST | `value` pattern index or name |
| `/palette`, `/paletteName` | POST | `value` palette index or name |
| `/gradientPalette`, `/gradientPaletteName` | POST | `value` gradient index or name |
| `/solidColor` | POST | `r`, `g`, `b` 0-255 |
| `/speed` | POST | `value` 1-255 |
| `/autoplay` | POST | `value` 0 or 1 |
| `/autoplayDuration` | POST | `value` 3-250 seconds |
| `/cooling`, `/sparking` | POST | `value` 1-255 (Fire and Water effects) |
| `/twinkleSpeed` | POST | `value` 2-7 |
| `/twinkleDensity` | POST | `value` 1-8 |
| `/fadeInSpeed`, `/fadeOutSpeed` | POST | `value` 16-128 |
| `/wifi` | GET | Wi-Fi status as JSON (the Wi-Fi password is not included) |

The firmware update (`/update`), file system editor (`/edit`, `/list`) and Wi-Fi reset (`/disconnectNetwork`) endpoints require the admin credentials (HTTP basic authentication).

Settings are saved to EEPROM 3 seconds after the last change, so dragging a slider does not wear out the flash memory.
