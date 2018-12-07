


// Copyright 2018 Martin Ondrak
// This Example code was developed for https://github.com/dragon-engineer/esp32_fmdevkit
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Please note that FM broadcasting is only legal with a valid license or a very low power.
// The author is not responsible for any damages or penalties.

// You must use the correct Arduino Board!! ESP32 FM DevKit uses specific I2C and I2S pins 

#include <stdio.h>
#include <Arduino.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <Ticker.h>
#include <Preferences.h>
#include <QN8027.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include "QN8027.h"
#include "symbols.h"


#ifdef __cplusplus
extern "C"
{
#endif
	
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "nvs.h"
	#include "nvs_flash.h"
	#include "esp_system.h"
	#include "esp_log.h"


	#include "esp_bt.h"
	#include "bt_app_core.h"
	#include "bt_app_av.h"
	#include "esp_bt_main.h"
	#include "esp_bt_device.h"
	#include "esp_gap_bt_api.h"
	#include "esp_a2dp_api.h"
	#include "esp_avrc_api.h"
	#include "driver/i2s.h"

#ifdef __cplusplus
}
#endif

Preferences preferences;

QN8027 fm;
Adafruit_SSD1306 oledDisplay = Adafruit_SSD1306(); // 128x64

Ticker oneSecTick;
Ticker rdsTick;
Ticker rdsRefreshTick;

float frequency = 87.5f;
float maybeFrequency = frequency; // When User changes the frequency with Switch/Button we use this temp variable
boolean frequencyChanged;

volatile boolean rdsTickFlag;
volatile boolean oledRefreshFlag = true;
volatile boolean rdsRefreshFlag;
volatile boolean oneSecFlag;
volatile boolean sw1Flag, sw2Flag, sw3Flag, sw4Flag;


esp_a2d_cb_param_t btConnectionParams;
btMetadata_t btMetadata;
uint64_t btMetadataMillis;

int btAudioSamplingFreq = 44100; // Default value that is updated as soon as there is BT A2DP connection

const char btName[] = "ESP32-FM_DevKit";
const char rdsName[] = "ESP32_FM";

uint8_t rdsTextRolloverPageCount; // Helper variable for RDS text paging

uint64_t btBlinkMillis; // Blue LED is flashing when Bluetooth A2DP is connected
boolean btLedBlinkState;

//Oversampling ratio - valid values are 64, 96, 128, 144, 192, 256...
// 64 is the lowest valid value. It is recommended as the result frequency has the lowest clock jitter.
int mclkCoeff = 64; // Fs * mclkCoeff = I2S MCLK clock
// We must use PWM to generate the I2S MCLK clock which is far from the best solution (We encounter a phase noise here).
// The ESP32 only has 1 Audio PLL so we can't really use it for our purpose - we'd need 2 (tested)
// Maybe someone knows a better hack to generate precise clock in ESP32

// ( The more precise MCLK the better sound quality )

//Bluetooth C methods use this function so it must be also Extern C
#ifdef __cplusplus
extern "C"
{
#endif
	// Change I2S MCLK clock - it assumes the PWM timer is already assigned to pin (We use channel 0)
	void adjustMclkRate(void)
	{
		ledcSetup(0, btAudioSamplingFreq * mclkCoeff, 1);
		ledcWrite(0, 1);
	}
#ifdef __cplusplus
}
#endif

void ICACHE_RAM_ATTR oneSec_callback(void) // One Second Ticker callback
{
	oneSecFlag = true;
}

void ICACHE_RAM_ATTR rdsTick_callback(void) // RDS Tick - we need this for paging the RDS Text if longer than 8 characters
{
	rdsTickFlag = true;
	// Frequency of this Ticker Event can be changed in symbols.h
}

void IRAM_ATTR rdsRefresh_callback(void) // RDS Refresh - We use this for streaming RDS info (every RDS page needs to be refreshed)
{
	rdsRefreshFlag = true;
	// Frequency of this Ticker Event can be changed in symbols.h
}

//event for handler "bt_av_hdl_stack_up
enum {
	BT_APP_EVT_STACK_UP = 0
};

//handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *param);

// Fired from the main loop - refresh the OLED
void oledRefresh(void)
{
	
	oledDisplay.fillRect(0, 0, 128, 64, BLACK);

	oledDisplay.setCursor(3, 3);
	oledDisplay.setTextSize(1);
	char rds[9];
	fm.getCurrentRds(rds); //Watch out, actually returns 9 bytes (8 chars + 1B termination)
	oledDisplay.printf("RDS: %c%c%c%c%c%c%c%c", rds[0], rds[1], rds[2], rds[3], rds[4], rds[5], rds[6], rds[7]);
	
	if (frequencyChanged)
	{
		oledDisplay.setCursor(120, 3);
		oledDisplay.printf("!");
	}
	
	oledDisplay.setCursor(3, 26);
	oledDisplay.setTextSize(2);
	oledDisplay.printf("%3.02f MHz", maybeFrequency);
	
	oledDisplay.display();

}

// Switch/Button Callbacks - ESC Button
void IRAM_ATTR sw1_callback(void)
{
	sw1Flag = true;
}

// Switch/Button Callbacks - OK Button
void IRAM_ATTR sw2_callback(void)
{
	sw2Flag = true;
}

// Switch/Button Callbacks - DOWN Button
void IRAM_ATTR sw3_callback(void)
{
	sw3Flag = true;
}

// Switch/Button Callbacks - UP Button
void IRAM_ATTR sw4_callback(void)
{
	sw4Flag = true;
}

void setup(void)
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);

	Serial.printf("\r\nESP32 Chip Revision: %u\r\n\r\n", ESP.getChipRevision());
	// Revision 0 had a bug in audio PLL design but all the chips now should be Rev 1
	
	preferences.begin("fmdevkit"); // Store user frequency in FLASH
	frequency = preferences.getFloat("frequency", FREQ_DEFAULT);
	maybeFrequency = frequency;

	pinMode(LED_BUILTIN, OUTPUT); // Blue built-in LED
	digitalWrite(LED_BUILTIN, HIGH);
	
	pinMode(SW1, INPUT);
	pinMode(SW2, INPUT);
	pinMode(SW3, INPUT);
	pinMode(SW4, INPUT);

	attachInterrupt((SW1), sw1_callback, FALLING); // Register all input callback
	attachInterrupt((SW2), sw2_callback, FALLING);
	attachInterrupt((SW3), sw3_callback, FALLING);
	attachInterrupt((SW4), sw4_callback, FALLING);

	i2s_comm_format_t comm_fmt = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
	i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);

	i2s_config_t i2s_config = {
		.mode = mode,
		.sample_rate = 44100, //this value is in fact dynamically adjusted by the A2DP source (PC might have a different sample rate than a phone)
		.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
		.communication_format = comm_fmt,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
		.dma_buf_count = 16,
		.dma_buf_len = 64,   //Interrupt level 1
		.use_apll = true,
		.fixed_mclk = 44100*64,
	};

	// Pin definitions as in the boards manager
	i2s_pin_config_t pin_config = {
		.bck_io_num = I2S_SCLK,
		.ws_io_num = I2S_LRCLK,
		.data_out_num = I2S_DOUT,
		.data_in_num = I2S_PIN_NO_CHANGE
	};
	
	i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
	i2s_set_pin(I2S_NUM_1, &pin_config);
	
	ledcAttachPin(I2S_MCLK, 0); //Init I2S MCLK
	adjustMclkRate(); // Calculate and write to the PWM timer so we start I2S MCLK

	if (!btStart()) {
		ESP_LOGE(BT_AV_TAG, "%s initialize controller failed\n", __func__);
		return;
	}

	if (esp_bluedroid_init() != ESP_OK) 
	{
		ESP_LOGE(BT_AV_TAG, "%s initialize bluedroid failed\n", __func__);
		return;
	}

	if (esp_bluedroid_enable() != ESP_OK) 
	{
		ESP_LOGE(BT_AV_TAG, "%s enable bluedroid failed\n", __func__);
		return;
	}

	/* create application task */
	bt_app_task_start_up();

	/* Bluetooth device name, connection mode and profile set up */
	bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

	// QN8027 Init
	fm.begin(SDA, SCL, 400000, frequency);

	fm.rdsEnable(RDS_ENABLED); // Change this in the symbols.h config file

	//Fill the internal library buffer. Later on we need to fm.rdsTick() and fm.rdsRefresh() it. See the Tickers for details
	fm.rdsBufSet(rdsName, sizeof(rdsName));
	
	//Demo screen
	oledDisplay.begin();
	oledDisplay.setTextColor(WHITE);

	oledDisplay.clearDisplay();
	oledDisplay.display();

	oledDisplay.setTextSize(1);
	oledDisplay.setCursor(0, 0);
	oledDisplay.printf("FM DevKit");

	oledDisplay.setTextSize(3);
	oledDisplay.setCursor(0, 16);
	oledDisplay.printf("ESP32");
	
	oledDisplay.setTextSize(1);
	oledDisplay.setCursor(0, 40);
	oledDisplay.printf("GitHub.com/dragon-engineer/esp32_fmdevkit");

	oledDisplay.display();

	delay(4000);

	oneSecTick.attach_ms(1000, oneSec_callback);
	rdsTick.attach_ms(RDS_PAGING_INTERVAL, rdsTick_callback);
	rdsRefreshTick.attach_ms(RDS_REFRESH_INTERVAL, rdsRefresh_callback);

}

static void bt_av_hdl_a2d_evt(uint16_t event, void *btConnectionParams)
{
	Serial.printf("%s evt %d\r\n", __func__, event);
}

// Bluetooth Audio Stack Init Event
static void bt_av_hdl_stack_evt(uint16_t event, void *param)
{
	switch (event) {
		case BT_APP_EVT_STACK_UP : {
			/* set up device name */
			esp_bt_dev_set_device_name(btName);

			/* initialize A2DP sink */
			esp_a2d_register_callback(&bt_app_a2d_cb);
			esp_a2d_sink_register_data_callback(&bt_app_a2d_data_cb);
			
			esp_a2d_sink_init();

			/* initialize AVRCP controller */
			esp_avrc_ct_init();
			esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
			
			/* set discoverable and connectable mode, wait to be connected */
			esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
			break;
		}
		default:
		Serial.printf("%s unhandled evt %d\r\n", __func__, event);
		break;
	}
}

// We can get info on Artist, Title, Album and Genre. The lower level handler is in file bt_app_av.c
void fetchNewMetadata(void)
{
	
	if (btMetadata.metadata[0][0] == '\0')
	{
		btMetadataMillis = 0;
		btMetadata.millisUpdated = 0;
		rdsTextRolloverPageCount = 0; // Blank / Invalid metadata (This happens when you close Music App for example)
		// So we just display station name
		
		return;
	}

	//Debug info
	Serial.println("\r\n---Metadata Received----------");
	Serial.printf("[0] %s\r\n", btMetadata.metadata[0]);
	Serial.printf("[1] %s\r\n", btMetadata.metadata[1]);
	Serial.printf("[2] %s\r\n", btMetadata.metadata[2]);
	Serial.printf("[3] %s\r\n", btMetadata.metadata[3]);
	Serial.println("------------------------------\r\n");

	char tmpbuf[120];
	strcpy(tmpbuf, btMetadata.metadata[0]);

	// Put the second string to a new RDS text page - fill this with spaces as necessary
	for (uint8_t i = 0; i < (8 - (strlen(btMetadata.metadata[0]) % 8)); i++)
	{
		strcat(tmpbuf, " ");
	}

	strcat(tmpbuf, btMetadata.metadata[1]);
	uint8_t length = strlen(tmpbuf) + 1;
	
	// Show item [0] and [1] twice in RDS Text then switch to station name rdsName
	rdsTextRolloverPageCount = (uint16_t)(((length >> 3) + 1) * 3); // Show 3 times

	rdsTick.detach();
	rdsTick.attach_ms(RDS_PAGING_INTERVAL, rdsTick_callback);
	fm.rdsBufSet(tmpbuf, length);

	oledRefreshFlag = true;

	btMetadataMillis = btMetadata.millisUpdated;
}

void loop(void)
{

	if (oneSecFlag)
	{
		// User can use this quite precise 1 second ticker to their own events


		oneSecFlag = false;
	}

	
	if (rdsRefreshFlag)
	{
		fm.rdsRefresh();

		rdsRefreshFlag= false;
	}

	if (rdsTickFlag)
	{
		
		fm.rdsTick();

		if (rdsTextRolloverPageCount)
		{
			rdsTextRolloverPageCount--;

			if (!rdsTextRolloverPageCount)
			{
				fm.rdsBufSet(rdsName, sizeof(rdsName));
			}
		}
		
		oledRefreshFlag = true;

		rdsTickFlag = false;
	}

	// Switch/Buttons handlers
	if (sw1Flag)
	{
		//ESC
		Serial.println("ESC");

		if (frequencyChanged)
		{
			maybeFrequency = frequency;
			frequencyChanged = false;

			oledRefreshFlag = true;
		}

		sw1Flag = false;
	}
	else if (sw2Flag)
	{
		//OK
		Serial.println("OK");

		frequency = maybeFrequency;
		fm.setFrequency(frequency);
		preferences.putFloat("frequency", frequency);
		frequencyChanged = false;

		oledRefreshFlag = true;

		sw2Flag = false;
	}
	else if (sw3Flag)
	{
		//DOWN
		Serial.println("DOWN");

		if (maybeFrequency > 87.5f) maybeFrequency -= 0.1f;
		else maybeFrequency = 108.0f;

		frequencyChanged = true;

		oledRefreshFlag = true;

		sw3Flag = false;
		
		
	}
	else if (sw4Flag)
	{
		//UP
		Serial.println("UP");

		if (maybeFrequency < 108.0f) maybeFrequency = maybeFrequency + 0.1f;
		else maybeFrequency = 87.5f;
		
		frequencyChanged = true;

		Serial.printf("Freq: %3.2f\r\n", maybeFrequency);

		oledRefreshFlag = true;

		sw4Flag = false;
	}
	
	// We received metadata - Show these in Serial Debug output and pass the first two in the rotating message on OLED / RDS
	if (btMetadataMillis != btMetadata.millisUpdated)
	{
		
		fetchNewMetadata();
	}

	if (oledRefreshFlag)
	{
		
		oledRefresh();

		oledRefreshFlag = false;
	}

	// Asynchronous LED handler - it blinks when connected to A2DP source (phone, laptop... with playing music)
	uint64_t currentMillis = millis();
	if (btConnectionParams.conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
	{
		
		if (btLedBlinkState)
		{
			if (currentMillis - btBlinkMillis > 300)
			{
				btLedBlinkState = false;

				btBlinkMillis = currentMillis;
			}
		}
		else
		{
			if (currentMillis - btBlinkMillis > 150)
			{
				btLedBlinkState = true;

				btBlinkMillis = currentMillis;
			}
		}
		
	}
	else
	{
		memset(&btMetadata, 0, sizeof(btMetadata));
		btLedBlinkState = false;
		btMetadataMillis = 0;
		rdsTextRolloverPageCount = 0;

		fm.rdsBufSet(rdsName, sizeof(rdsName));
	}
	
	digitalWrite(LED_BUILTIN, btLedBlinkState);

}

