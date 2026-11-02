# Skymio ☁️

## What is Skymio?

Skymio is a small weather display that shows the current temperature, whether it's raining, and the weather forecast for the next three days. It uses an ESP32-C3 and a 2.4-inch LCD screen to show the weather in a simple and easy-to-read way.

## Why I Made It

I wanted to make something that I could actually use every day. Usually, whenever I want to check the weather, I have to open my phone or search it up. So I thought it would be cool to have a small display sitting on my desk that shows the weather automatically.

I also wanted to get more experience designing PCBs and 3D-printed enclosures. I decided to keep the design pretty simple because I wanted it to look clean and not take up too much space.

## How It Works

Skymio uses a Seeed Studio XIAO ESP32-C3, which connects to Wi-Fi and gets weather information from Open-Meteo, a free weather API.

The ESP32 then sends the information to the LCD screen. The display shows the current temperature in Fahrenheit, whether it's raining, and the high and low temperatures for the next three days.

The weather is set to update automatically every 15 minutes, so I don't have to do anything once it's connected.

## Project Images

### PCB Layout

![Skymio PCB](images/pcb.png)

### Schematic

![Skymio Schematic](images/schematic.png)

## Components

| Component | Quantity | Purpose |
|---|---:|---|
| [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html) | 1 | Controls everything and connects to Wi-Fi |
| [Waveshare 2.4-inch LCD](https://www.waveshare.com/product/2.4inch-lcd-module.htm) | 1 | Displays the weather |
| 10 kΩ resistor | 1 | Pull-up resistor for GPIO8 |
| 10 µF capacitor | 1 | Helps stabilize power |
| 100 nF capacitor | 1 | Helps filter power noise |
| 8-pin male header | 1 | Connects the LCD cable |
| Custom PCB | 1 | Connects all the components |
| USB-C cable | 1 | Powers the ESP32 |
| 3D-printed enclosure | 1 | Holds everything together |
| M2.5 screws and nuts | 2 | Secures the PCB |

## Wiring

| LCD Pin | ESP32-C3 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| DIN | D10 |
| CLK | D8 |
| CS | D3 |
| DC | D4 |
| RST | D5 |
| BL | 3.3V |

## Firmware

I used Arduino to program the ESP32-C3. The code connects to Wi-Fi, requests weather information from Open-Meteo, and displays it on the screen.

The main libraries used are Adafruit GFX, Adafruit ILI9341, and ArduinoJson. I also used the ESP32 WiFi and HTTPClient libraries to get weather information.

To use the code, open `skymio.ino` in Arduino IDE, install the required libraries, and enter your Wi-Fi information and location coordinates. After uploading it to the ESP32-C3, the display should start showing the weather once it connects to Wi-Fi.

## PCB and Enclosure

I designed the schematic and PCB in KiCad. Since the project only needs an ESP32-C3 and an LCD, the circuit is pretty simple. I added two capacitors for power filtering and a resistor for one of the ESP32's pins.

For the enclosure, I used Fusion 360. I wanted the screen to sit flush with the front so the whole device would look neat and minimalistic.

The LCD is mounted separately from the main PCB and connects using an 8-pin cable. The PCB sits behind the screen and is held in place using screws. I also placed the USB-C port on the side so the device can be powered without opening the case.

## Challenges

One of the things I had to figure out was how to connect the LCD to my custom PCB. At first, I planned to use a JST connector, but I realized I could use an 8-pin male header with the female jumper wires instead.

I also spent time figuring out how to position everything inside the enclosure. I wanted the screen to sit flush while keeping the edges as small as possible. This meant I had to account for the screen's dimensions, PCB size, mounting holes, and enough space for the wires.

## Resources

Here are some of the resources I used or referenced while working on Skymio.

- [Seeed XIAO ESP32-C3 Documentation](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [Waveshare LCD Documentation](https://www.waveshare.com/wiki/2.4inch_LCD_Module)
- [Adafruit ILI9341 Library](https://github.com/adafruit/Adafruit_ILI9341)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Open-Meteo Weather API](https://open-meteo.com/en/docs)
- [ArduinoJson Documentation](https://arduinojson.org/)
