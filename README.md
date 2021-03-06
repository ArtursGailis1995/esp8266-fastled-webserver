FastLED + ESP8266 Web Server
=========

Control an addressable RGB LED strip with an ESP8266 via a WEB browser.

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

The WEB App is a single page app that uses [jQuery](https://jquery.com) and [Bootstrap](http://getbootstrap.com).  It has buttons for On/Off, a slider for brightness, a pattern selector, and a color picker (powered by [jQuery MiniColors](http://labs.abeautifulsite.net/jquery-minicolors)).  Event handlers for the controls are used, so you don't have to click a 'Send' button after making changes to the LED strip parameters.  The brightness slider and the color picker use a delayed event handler, to prevent from flooding the ESP8266 WEB server with too many requests. As web app is stored in SPIFFS (on-board flash memory).

* **Main page provides access to all features related to controlling RGB LED strip parameters and features**<br/>
<img src="/images/WEBApp_Main.jpg" alt="WEB App Main window" width="850"/>

* **Simple mode allows to control power state, change active mode or set static color of RGB LED strip**<br/>
<img src="/images/WEBApp_Simple.jpg" alt="WEB App Simple window" width="850"/>

* **Firmware update page allows to upload precompiled firmware to ESP8266 in order to update it without connecting to PC**<br/>
<img src="/images/WEBApp_FW.jpg" alt="WEB App Firmware update window" width="850"/>

* **Information page lists recent changes to this project and latest feature updates available to it**<br/>
<img src="/images/WEBApp_Info.jpg" alt="WEB App Information window" width="850"/>

Installing
-----------
The app is installed via the Arduino IDE which can be [downloaded here](https://www.arduino.cc/en/main/software). The ESP8266 boards will need to be added to the Arduino IDE which is achieved as follows:

- Click `File` > `Preferences`
- Copy and paste the URL `http://arduino.esp8266.com/stable/package_esp8266com_index.json` into the `Additional Boards Manager URLs` field
- Click `OK`
- Click `Tools` > `Boards: ...` > `Boards Manager`
- Find and click on `ESP8266` (using the Search function may make this proccess quicker)
- Click on `Install` (version 2.7.4 works fine)
- After installation, click on `Close`
- Select your `ESP8266` or `NodeMCU 1.0 (ESP12E)` board from the `Tools` > `Board: ...` menu according to your board specifications

The app depends on the following libraries, which must either be downloaded from GitHub and placed in the Arduino 'libraries' folder, or installed as [described here](https://www.arduino.cc/en/Guide/Libraries) by using the Arduino library manager (working versions of all required libraries are included in this projects `libraries` folder too):

- [FastLED](https://github.com/FastLED/FastLED)
- [Arduino WebSockets](https://github.com/Links2004/arduinoWebSockets)
- [WIFiManager Developement branch](https://github.com/tzapu/WiFiManager/tree/development)

Download the app code from GitHub using the green Clone or Download button from [the GitHub project main page](https://github.com/ArtursGailis1995/esp8266-fastled-webserver) and click Download ZIP. Decompress the ZIP file after downloading it.

The web app needs to be uploaded to the ESP8266's SPIFFS memory. You can do this within the Arduino IDE after installing the [Arduino ESP8266FS tool](https://github.com/esp8266/arduino-esp8266fs-plugin). On Windows, you may need to install Python from Microsoft Store or other sources for this tool to work. Or you can use older version of the tool (it is included in this projects `tools` folder).

With ESP8266FS installed, upload the WEB App using `ESP8266 Sketch Data Upload` command in the Arduino `Tools` menu.

Wi-Fi hotspot with name `ESP-****` will be started after code upload. You need to connect to it and set up your Wi-Fi network by providing your router's network name (SSID) and password, then click Connect. After connecting to your home Wi-Fi, the hotspot is disabled and you can see the assigned IP adress of the LED controller in serial output via USB or look in your router settings for DHCP lease assigned to `ESP-****`.

Set communication pin for your ESP8266 module in the sketch before uploading it. You can see the ESP8266 NodeMCU pin mappings in the image below. Look for the following code fragment in the sketch's `.ino` file and change data pin if needed. Example: D2 physically on board > D4 in code for NodeMCU module.

    #define DATA_PIN      D4    

<img src="/images/GPIO_Explained.png" alt="GPIO of ESP8266 explained" width="650"/>

REST Web services
-----------------

The firmware implements basic [RESTful web services](https://en.wikipedia.org/wiki/Representational_state_transfer) using the `ESP8266WebServer` library.  Current values are requested with `HTTP GETs`, and values are set with `HTTP POSTs` using query string parameters.