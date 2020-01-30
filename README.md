FastLED + ESP8266 Web Server
=========

Control an addressable RGB LED strip with an ESP8266 via a WEB browser or infrared remote control.

## Parts used for this project

* ESP8266 NodeMCU V3 developement board<br/>
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
* Directly edit SPIFFS file system contents (HTML, JS, CSS, fonts)
* Upload new firmware via OTA
* Many, many more!

Web App
--------

The WEB App is a single page app that uses [jQuery](https://jquery.com) and [Bootstrap](http://getbootstrap.com).  It has buttons for On/Off, a slider for brightness, a pattern selector, and a color picker (powered by [jQuery MiniColors](http://labs.abeautifulsite.net/jquery-minicolors)).  Event handlers for the controls are used, so you don't have to click a 'Send' button after making changes to the LED strip parameters.  The brightness slider and the color picker use a delayed event handler, to prevent from flooding the ESP8266 WEB server with too many requests. As web app is stored in SPIFFS (on-board flash memory), uploading the files can be extremely slow, requiring several minutes, regardless of how large the files are.

* **Main page provides access to all features related to controlling RGB LED strip parameters and features**<br/>
<img src="/images/WEBApp_Main.jpg" alt="WEB App Main window" width="850"/>

* **Simple mode allows to control power state, change active mode or set static color of RGB LED strip**<br/>
<img src="/images/WEBApp_Simple.jpg" alt="WEB App Simple window" width="850"/>

* **Firmware update page allows to upload precompiled firmware to ESP8266 in order to update it without connecting to PC**<br/>
<img src="/images/WEBApp_FW.jpg" alt="WEB App Firmware update window" width="850"/>

* **Wi-Fi settings page lets you set SSID and password of your home Wi-Fi network for connecting ESP8266 with your Wi-Fi router**<br/>
<img src="/images/WEBApp_WLAN.jpg" alt="WEB App Wi-Fi window" width="850"/>

* **Information page lists recent changes to this project and latest feature updates available to it**<br/>
<img src="/images/WEBApp_Info.jpg" alt="WEB App Information window" width="850"/>

Installing
-----------
The app is installed via the Arduino IDE which can be [downloaded here](https://www.arduino.cc/en/main/software). The ESP8266 boards will need to be added to the Arduino IDE which is achieved as follows:

- Click File > Preferences
- Copy and paste the URL "http://arduino.esp8266.com/stable/package_esp8266com_index.json" into the Additional Boards Manager URLs field
- Click OK
- Click Tools > Boards: ... > Boards Manager
- Find and click on ESP8266 (using the Search function may make this proccess quicker)
- Click on Install
- After installation, click on Close
- Select your ESP8266 board from the Tools > Board: ... menu

The app depends on the following libraries, which must either be downloaded from GitHub and placed in the Arduino 'libraries' folder, or installed as [described here](https://www.arduino.cc/en/Guide/Libraries) by using the Arduino library manager:

- [FastLED](https://github.com/FastLED/FastLED)
- [IRremoteESP8266](https://github.com/sebastienwarin/IRremoteESP8266)
- [Arduino WebSockets](https://github.com/Links2004/arduinoWebSockets)

Download the app code from GitHub using the green Clone or Download button from [the GitHub project main page](https://github.com/jasoncoon/esp8266-fastled-webserver) and click Download ZIP. Decompress the ZIP file in your Arduino sketch folder.

The web app needs to be uploaded to the ESP8266's SPIFFS.  You can do this within the Arduino IDE after installing the [Arduino ESP8266FS tool](http://esp8266.github.io/Arduino/versions/2.3.0/doc/filesystem.html#uploading-files-to-file-system).

With ESP8266FS installed upload the web app using `ESP8266 Sketch Data Upload` command in the Arduino Tools menu.

Then enter your wi-fi network SSID and password in the WiFi.h file, and upload the sketch using the Upload button.

Compression
-----------

The web app files can be gzip compressed before uploading to SPIFFS by running the following command:

`gzip -r data/`

The ESP8266WebServer will automatically serve any .gz file.  The file index.htm.gz will get served as index.htm, with the content-encoding header set to gzip, so the browser knows to decompress it.  The ESP8266WebServer doesn't seem to like the Glyphicon fonts gzipped, though, so I decompress them with this command:

`gunzip -r data/fonts/`

REST Web services
-----------------

The firmware implements basic [RESTful web services](https://en.wikipedia.org/wiki/Representational_state_transfer) using the ESP8266WebServer library.  Current values are requested with HTTP GETs, and values are set with POSTs using query string parameters.  It can run in connected or standalone access point modes.

Infrared Remote Control
-----------------------

Control via infrared remote control is also supported, via the [ESP8266 port of the IRremote library](https://github.com/sebastienwarin/IRremoteESP8266).

[Adafruit NeoPixel Ring]:https://www.adafruit.com/product/1586
[Adafruit HUZZAH ESP8266 Breakout]:https://www.adafruit.com/products/2471
