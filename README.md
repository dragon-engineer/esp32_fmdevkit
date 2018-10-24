# ESP32 FM DevKit (esp32_fmdevkit)
*ESP32-based development board

#### Features
###### - USB to UART bridge with auto boot mode / reset
###### - Blue OLED 128x64 (top 16pixels yellow for a status bar) 
###### - 24-bit Stereo I2S DAC
###### - Single-chip FM transmitter with RDS support
###### - 3.5mm Headphone Jack output
###### - 4 User buttons
###### - 1 Status LED (Red) and 1 User LED (Blue)
###### - RF SMA Output
###### - Expansion Pin Headers

*Note: The ESP32 FM DevKit is available in two variants: **Standard** is based on ESP32 with on-board PCB antenna trace and **Extended** is based on ESP32 with external IPEX antenna connector (antenna not included)*

*The FM transmitter works in full range, i.e. 76 - 108 MHz. Check your local FM frequencies first.*

##### Build your own FM Radio station based on this kit! Use WiFi or Bluetooth as an audio source (default ESP32 example firmware uses Bluetooth A2DP sink). 

![Top](https://raw.githubusercontent.com/dragon-engineer/esp32_fmdevkit/master/Hardware/Top.png "Top")

![Bottom](https://raw.githubusercontent.com/dragon-engineer/esp32_fmdevkit/master/Hardware/Bottom.png "Bottom")

###### Pinout Information:

|  Board Label | Arduino Pin Definition | ESP32 Pin Number|  Description |  
| ------------ | ------------ |
| D0 | D0 | 34 | GPIO Pin |
| D1 | D1 | 35 | GPIO Pin |
| D2 | D2 | 32 | GPIO Pin |
| D3 | D3 | 33 | GPIO Pin |
| D4 | D4 | 27 | GPIO Pin |
| D5 | D5 | 14 | GPIO Pin |
| D6 | D6 | 12 | GPIO Pin |
| D7 | D7 | 13 | GPIO Pin |
| D8 | D8 | 15 | GPIO Pin |
| D9 | D9 | 23 | GPIO Pin |
| D10 | D10 | 0 | GPIO Pin |
| SW1 | SW1 | 4 | Button Switch 1 (ESC) Input |
| SW2 | SW2 | 18 | Button Switch 2 (OK) Input |
| SW3 | SW3 | 19 | Button Switch 3 (Down) Input |
| SW4 | SW4 | 21 | Button Switch 4 (Up) Input |
| TX | TX | 1 | UART TX (Connected to USB-UART bridge)
| RX | RX | 3 | UART RX (Connected to USB-UART bridge) |
| SCL | SCL | 17 | I2C SCL (2k2 pull-up), OLED and FM TX present |
| SDA | SDA | 16 | I2C SDA (2k2 pull-up), OLED and FM TX present |
| LED2 | LED_BUILTIN | 5 | Blue built-in LED |
| I2SL | I2S_LRCLK | 25 | I2S Left/Right CLK |
| I2SB | I2S_SCLK | 26 | I2S Clock (Sampling Frequency) |
| I2SD | I2S_DOUT | 22 | I2S Data Out |
| 5V | - | - | USB Vcc |
| 3V3 | - | - | LDO Regulator Output (500mA total) |


Note that this is just a pre-release of an unofficial ESP32 development board mostly distributed on Ebay. You can however contact the Dev team at this [E-Mail](mailto:kfeksa2@gmail.com "E-Mail") if you are interested in this board distribution.