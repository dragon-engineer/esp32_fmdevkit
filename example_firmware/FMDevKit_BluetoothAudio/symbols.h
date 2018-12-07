/*
 * symbols.h
 *
 * Created: 14.11.2018 17:33:07
 *  Author: Matas
 */ 


#ifndef SYMBOLS_H_
#define SYMBOLS_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include <Arduino.h>

#define RDS_PAGING_INTERVAL		2000 // Time to rotate pages: Note that we need to send two 4-byte chunks (therefore doubles this time as a result = 4 seconds to see one 8-characters screen)
#define RDS_REFRESH_INTERVAL	200	 // Time to refresh pages in RDS feed (internal setting, user should not notice this)

#define RDS_ENABLED	false	// Sorry but the RDS is a little tricky: I got it to work with RTL-SDR where I could see
							// RDS data with no problem but with my portable receiver I had no luck (Only got gibberish chars or nothing)
							// If you want to help with the RDS implementation you should start in the QN8027.cpp file in _rdsWrite() method. Thank you
							// Also I noticed some strange audible pitch when transmitting with RDS - either the QN8027 was foulty or the design is somehow messed up (worse scenario)


// We use this structure to get the Audio Metadata from the Bluetooth A2DP Connection. It is available in the main sketch.							
typedef struct 
{
	char metadata[4][60];
	uint64_t millisUpdated;
} btMetadata_t;
/*
Example output:
---Metadata Received----------
[0] Hope (feat. Ty Dolla $ign)
[1] Wiz Khalifa
[2] Blacc Hollywood (Deluxe Version)
[3] Hip-Hop/Rap
------------------------------
Source: Android Google Music App
*/


#define LOG_HEAP Serial.printf("Free Heap: %u\r\n", ESP.getFreeHeap());




#ifdef __cplusplus
}
#endif

#endif /* SYMBOLS_H_ */
