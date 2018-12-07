/*
 * unused_tests.cpp
 *
 * Created: 18.11.2018 0:39:09
 *  Author: Matas
 */ 


 // Attempt to use the audio PLL to generate I2S MCLK
 /*
	
	i2s_config_t i2s_config2 = {
		.mode = mode,
		.sample_rate = 44100, //this value is in fact dynamically adjusted by the A2DP source (PC might have a different sample rate than a phone)
		.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
		.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
		.communication_format = comm_fmt,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL3, // high interrupt priority
		.dma_buf_count = 8,
		.dma_buf_len = 64,   //Interrupt level 1
		.use_apll = true, // Don't Use audio PLL - it is very bad in this (synchronous) case
		.fixed_mclk = 44100*64
	};

	// Pin definitions as in the boards manager
	i2s_pin_config_t pin_config2 = {
		.bck_io_num = I2S_MCLK,
		.ws_io_num = I2S_PIN_NO_CHANGE,
		.data_out_num = I2S_PIN_NO_CHANGE,
		.data_in_num = I2S_PIN_NO_CHANGE
	};

	i2s_driver_install(I2S_NUM_0, &i2s_config2, 0, NULL);
	i2s_set_pin(I2S_NUM_0, &pin_config2);
	uint16_t sample = 0;
	i2s_write_bytes(I2S_NUM_0, &sample, sizeof(sample), 10);
	
	*/


