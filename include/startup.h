#include "kioskControl.h"
#include "config.h"

// #include <WiFiManager.h>
#include <WiFiMulti.h>
#include <FastLED.h>
#include <EasyNextionLibrary.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <nvs_flash.h>
#include "timer.h"
#include <time.h>


//Below 4 libs required by ESP32Fota
#include <WiFiClientSecure.h>
// #include <HTTPClient.h>
#include <FS.h>
#include <Update.h>
//-----------------------------------

#include "alvato.h"


#ifdef RFID_SPI
#include <SPI.h>
#include <MFRC522.h>
#endif

#ifdef RFID_i2C
#include <MFRC522.h>
#endif

#ifdef EASY_NEXTION
#include <EasyNextionLibrary.h>
#endif
