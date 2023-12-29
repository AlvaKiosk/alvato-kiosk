/*  Before use modify EasyNextionLibrary.h and EasyNextionLibrary.cpp following

    in file EashNextionLibrary.h   and this line after begin
     "void begin(unsigned long baud,SerialConfig sconfig,int rx,int tx);"

    in file EasyNextionLibrary.h  and followin line 

    void EasyNex::begin(unsigned long baud,SerialConfig sconfig,int rx,int tx){
      _serial->begin(baud,sconfig,rx,tx);

      _tmr1 = millis();
      while(_serial->available() > 0){     // Read the Serial until it is empty. This is used to clear Serial buffer
        if((millis() - _tmr1) > 400UL){    // Reading... Waiting... But not forever...... 
          break;                            
        }   
          _serial->read();                // Read and delete bytes
      }
    }

*/




#include <Arduino.h>
#include "startup.h"
#include <Insights.h>

const char insights_auth_key[] = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyIjoiOGI3OWRjNmMtZmZlZC00NjM4LTk4NWItZThmMzU4ODI0MGM4IiwiaXNzIjoiZTMyMmI1OWMtNjNjYy00ZTQwLThlYTItNGU3NzY2NTQ1Y2NhIiwic3ViIjoiNjE4ZWU1NWYtMjkyZi00NmIxLThiNDctMDk1OGM5Y2QzN2FiIiwiZXhwIjoyMDE2ODE4MzI1LCJpYXQiOjE3MDE0NTgzMjV9.aK0biAr_VsB7gy86gNOjrO2alt5NUSSIkt-m1doSvCs3hLDHZRhvQ7PtgfaATRyAvQZw2a0z5S0_DKJ6IlIdWZKzIWUii3y7X-uVdNYK5rEPqLCNIlK5do__Eb0JAAWZZWWtipD_YinS4hInoqjwcAbObA-30wqA8NbeEX82luCtXQPH-4Ws3n7YcTVgesd6YlBwvqQh5KxmtojdGs4MQYiwn69-hZgxXw2HhCii7AoKtSHU3DFYULlMLCrrqjAs3QW3fNR9GRE-TCvtbcdz6cCIUT_BcoUBVVCwfKUDXadS1vpXrlMPWPiaxBqLQ3OvuCDTY4sol4JHkTn3Py1FcA";
/*-----------------  Function declaration in main file -------------------*/
String getdeviceid(void);
void coincount(void);
void IRAM_ATTR gpio_isr_handler(void* arg);
void gpio_task(void *arg);
void init_interrupt(void);

void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);

unsigned long hex2dec(byte *buffer, byte bufferSize);
void readRFID(void);

uint32_t getTagInfo(void);


void mqConnection(void);
void mqCallback(char* topic, byte* payload, unsigned int length);

void alvatoOtaFinishedCb(int partition, bool restart_after);
/*-----------------  Finish Function declaration in main file -------------------*/




Preferences cfgRAM;
CONFIG cfgInfo;
USERINFO userinfo;

int coinValue = 0;
int coinCount = 0;
int billValue = 0;
int billCount = 0;
int topupValue = 0;
bool creditUpdateFlag = false;

// int pricePerCoin = 1;
// int pricePerBill = 10;

//For RFID Page
uint32_t tagId = 0;
int credit = 0;
int newCredit = 0;
// int maxTopup = 300;
int topupPrice = 0;
int manualTopup = 0;

// String branchCode = "";
// String assetCode = "";
String qrGenId = "";


boolean waitForPay = true;
boolean showSummary = false;
boolean showPageFlag = true;
boolean slipCheckFailed = false;
int activePage = -1;

// int topupPrice[]={1,2,5,10,20};
size_t numBtn = 0;
boolean setBtnText = false;


#ifdef RGB_LED        // declare in kioskControl.h
  #define NUM_LEDS 1
  CRGB leds[NUM_LEDS];
#endif

#ifdef RFID_SPI
  MFRC522 rfid(CS2, RST);
  MFRC522::MIFARE_Key key; 
  int activetime = 0;

  // Init array that will store new NUID 
  // byte nuidPICC[4] = {0, 0, 0, 0};
#endif

#ifdef QRREADER
  String qrPrice="";
  String qrSlip="";
  String shopName="";
#endif

#ifdef EASY_NEXTION
  EasyNex myNex(Serial1);
  String topupValueStr;
#endif


//Communication Parameter
WiFiMulti wifiMulti;
WiFiClient esp1client,wiclient;

//MQTT Parameter
PubSubClient mqtt0(esp1client), mqtt1(wiclient);

String pubTopic =""; // Tx to server
String subTopic = "";  //Rx from server
String jsonmsg;
DynamicJsonDocument doc(512);

//Time Parameter
time_t tnow;
struct tm timeNow;
char timeNowFormat [30];


//Asset
String localIP;

//ActionState
uint8_t actionState;


//Alvato Parameter
alvato backend;
String response ="";


//OTA intial parameter
esp32FOTA FOTA(CODENAME, FIRMWARE, false, true); 
//End OTA initial parameter


/*-------------------------------------  Setup Area -------------------------------------*/

void setup() {
  //Setup serial console port.
  Serial.begin(115200);

  //OTA callback function
  FOTA.setUpdateFinishedCb(alvatoOtaFinishedCb);

  //Setup Nextion port for EASY NEXTION Library
  #ifdef EASY_NEXTION  // For initial
    Serial.println("Setup -> EASY NEXTION Library.");
    myNex.begin(9600,SERIAL_8N1,RXU1,TXU1);
    myNex.writeStr("page 0");
  #endif

  //Setup GPIO and Interrupt
  Serial.println("");
  Serial.println("Setup -> Initial GPIO and interrupt.");
  initGPIO(INPUT_SET,OUTPUT_SET); 
  init_interrupt();

  digitalWrite(ENCOIN,LOW);
  digitalWrite(UNLOCK,LOW);

  //Setup RGB LED indicator on PCB
  #ifdef RGBLED 
    Serial.println("Setup -> RGB LED indicator.");
    FastLED.addLeds<SK6812, RGB_LED, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    leds[0] = CRGB::Green;
    FastLED.show();
  #endif

  //Setup RFID SPI port
  #ifdef RFID_SPI
    Serial.println("Setup -> RFID SPI.");
    SPI.begin(SCLK, MISO, MOSI);
    rfid.PCD_Init();
    rfid.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
    // Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  #endif


  /*--------------- Initial Nextion page for booting  ---------------*/
  #ifdef EASY_NEXTION  //For command
    Serial.println("Setup -> Booting screen.");
    myNex.writeStr("page 0");
    myNex.writeStr("t0.pco=0");
    myNex.writeStr("vis t5,0");
    myNex.writeStr("msgTxt.txt","");
    myNex.writeStr("p0.pic=13");
    myNex.writeStr("vis title,0");
  #endif

  /*---------------------------- Starting ------------------------- */

  Serial.println("============================================================");
  Serial.println("=                          Starting                        =");
  Serial.println("============================================================");


  /*--------------------- Format NVRAM ------------------------*/
  Serial.printf("\n\nDelete NV-RAM data?  y/Y to delete or any to continue.\n");
  Serial.printf("Wait for key 15 secs ");

  int wtime = 0;
  while((Serial.available() < 1) && (wtime <30)){
    Serial.print("*");
    wtime++;
    delay(500);
  }
  //************ v1.05 wait for NV-RAM ************
  byte keyin = ' ';  // Accept key y or Y from serial port only
  while(Serial.available() > 0){
    keyin = Serial.read();
      //Serial.println(keyin);
  }

  if((keyin == 121) || (keyin == 89)){
    cfgRAM.begin("config",false);
    cfgRAM.clear();
    cfgRAM.end();
    Serial.println();
    Serial.println("#################  NV-RAM Deleted #################");
    Serial.println();
  }



  /* -------------------- Initialize Configuraiton  -------------------- */
  // Step1: Load default configuration from code.  default config is in initCFG function inside file config.cpp
  Serial.println("Setup -> Load initial configuation.");
  initCFG(cfgInfo);     


  // ---------- WiFi Configuration
  cfgRAM.begin("config",false);  //Open Preference

  Serial.println("Setup -> API Parameter");
  // ---------- assetRegister
  if(cfgRAM.isKey("assetregister")){
    cfgInfo.api.assetregister = cfgRAM.getString("assetregister");
    Serial.printf("  |- NVRAM -> assetRegister: %s\n",cfgInfo.api.assetregister.c_str());
  }else{
    Serial.printf("  |- Config.cpp -> assetRegister: %s\n",cfgInfo.api.assetregister.c_str());
  }

  // ---------- apihost
  if(cfgRAM.isKey("apihost")){
    cfgInfo.api.apihost = cfgRAM.getString("apihost");
    Serial.printf("  |- NVRAM -> apiHost: %s\n",cfgInfo.api.apihost.c_str());
  }else{
    Serial.printf("  |- Config.cpp -> apiHost: %s\n",cfgInfo.api.apihost.c_str());
  }

  // ---------- apihostQR
  if(cfgRAM.isKey("apihostqr")){
    cfgInfo.api.apihostqr = cfgRAM.getString("apihostqr");
    Serial.printf("  |- NVRAM -> apiHostQR: %s\n",cfgInfo.api.apihostqr.c_str());
  }else{
    Serial.printf("  |- Config.cpp -> apiHostQR: %s\n",cfgInfo.api.apihostqr.c_str());
  }  
  
  // ---------- apiKey
  if(cfgRAM.isKey("apikey")){
    cfgInfo.api.apikey = cfgRAM.getString("apikey");
    Serial.printf("  |- NVRAM -> apiKey: %s\n",cfgInfo.api.apikey.c_str());
  }else{
    Serial.printf("  |- Config.cpp -> apiKey: %s\n",cfgInfo.api.apikey.c_str());
  }

  // ---------- apiSecret
  if(cfgRAM.isKey("apisecret")){
    cfgInfo.api.apisecret = cfgRAM.getString("apisecret");
    Serial.printf("  |- NVRAM -> apiSecret: %s\n",cfgInfo.api.apisecret.c_str());
  }else{
    Serial.printf("  |- Config.cpp -> apiSecret: %s\n",cfgInfo.api.apisecret.c_str());
  }

  backend.setVar("apihost",cfgInfo.api.apihost.c_str());
  backend.setVar("apihostqr",cfgInfo.api.apihostqr.c_str());
  backend.setVar("apikey",cfgInfo.api.apikey.c_str());
  backend.setVar("apisecret",cfgInfo.api.apisecret.c_str());
  backend.setVar("assetregister",cfgInfo.api.assetregister.c_str());


  Serial.println();
  Serial.println("Setup -> WiFiMulti Connection");
  // WiFi-1 config then add to list for wifi.
  if(cfgRAM.isKey("wifi1.ssid")){
    cfgInfo.wifi[0].ssid = cfgRAM.getString("wifi1.ssid");
    cfgInfo.wifi[0].key = cfgRAM.getString("wifi1.key");
    wifiMulti.addAP(cfgInfo.wifi[0].ssid.c_str(),cfgInfo.wifi[0].key.c_str());
    Serial.print("  |- NVRAM -> WiFi-1 SSID: "); 
    Serial.print(cfgInfo.wifi[0].ssid);
    Serial.print(" ,Key: "); 
    Serial.println(cfgInfo.wifi[0].key);
  }

  // WiFi-2 config then add to list for wifi.
  if(cfgRAM.isKey("wifi2.ssid")){
    cfgInfo.wifi[1].ssid = cfgRAM.getString("wifi2.ssid");
    cfgInfo.wifi[1].key = cfgRAM.getString("wifi2.key");
    wifiMulti.addAP(cfgInfo.wifi[1].ssid.c_str(),cfgInfo.wifi[1].key.c_str());
    Serial.print("  |- NVRAM -> WiFi-2 SSID: "); 
    Serial.print(cfgInfo.wifi[1].ssid);
    Serial.print(" ,Key: "); 
    Serial.println(cfgInfo.wifi[1].key);
  }
  cfgRAM.end();  // Close Preference


  // ------- Default wifi is  myWiFi   add to List.
  Serial.println("  |- Config.cpp -> WiFi SSID: from default in code.");
  wifiMulti.addAP("myWiFi","1100110011");
  wifiMulti.addAP("Cashless","a1b2c3d4e5");
  wifiMulti.addAP("Casless-shop","a1b2c3d4e5");
  wifiMulti.addAP("Casless-shop-2.4G-ext","a1b2c3d4e5");
  wifiMulti.addAP("CashlessCashier","a1b2c3d4e5");

  // ------- Connecting WiFi
  int wifiLimit = 10; //WiFi retry limit
  Serial.printf("  |- WiFi Connecting with retrylimit %d times\n",wifiLimit);
  while ( (wifiMulti.run() != WL_CONNECTED) && (--wifiLimit >= 0) ) {
    if(wifiLimit >= 0){
      Serial.printf("     %d retries. -> ",10 - wifiLimit);
      delay(500);
    }else{
      Serial.println("");
      wifiLimit = 0;
      break;
    }
  }

  //WiFi Failed to connect any network.  Then asked for ESPTouch Smartconfig
  if((WiFi.status() != WL_CONNECTED) && (++wifiLimit == 0)){

    //Show wifi waiting on Nextion
    myNex.writeStr("msgTxt.y=320");
    myNex.writeStr("msgTxt.pco=63488");
    myNex.writeStr("msgTxt.txt","WiFi From ESPTouch");
    myNex.writeStr("vis msgTxt,1");

    wifiCFG(cfgRAM,cfgInfo);

    // Serial.printf("    Failed to connect WIFI ***********\n");
    // Serial.printf("\n\nWould you like to setup WiFi? y/Y to setup or any to continue.\n");
    // Serial.printf("Wait for key 15 secs ");

    // int wtime = 0;
    // while((Serial.available() < 1) && (wtime <30)){
    //   Serial.print("*");
    //   wtime++;
    //   delay(500);
    // }

    // Serial.println("");

    // //************  ************
    // byte keyin = ' ';  // Accept key y or Y from serial port only
    // while(Serial.available() > 0){
    //   keyin = Serial.read();
    //     //Serial.println(keyin);
    // }

    // //Confirm reconfig WiFi
    // if((keyin == 121) || (keyin == 89)){
    //   wifiCFG(cfgRAM,cfgInfo);  //Reconfig WiFi
    // }
  }


  //******************************************************************************
  //                        Double Check WIFI Connection
  //******************************************************************************

  //------------ Double Check WiFi connection --------
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("   WiFi -> WiFi Connected!");
    Serial.printf("  |- WiFi: %s\n", WiFi.SSID().c_str());
    localIP = WiFi.localIP().toString();
    Serial.print("  |- IP: ");
    Serial.println(localIP);

    myNex.writeStr("t5.txt","No WiFi Mode");
    myNex.writeStr("p1.pic=23");
  }else{
    Serial.println("");
    Serial.println("WiFi -> No WiFi available. Working in no WIFI Mode.");
    myNex.writeStr("t5.txt","No WiFi Mode");
    myNex.writeStr("p1.pic=27");
  }


  if(WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("   ####################################################");
    Serial.println("                         WIFI READY                    ");
    Serial.println("   ####################################################");


    // --------------------------- Prepare device information and Header  ------------------------------
    cfgRAM.begin("config",false);  //Open Preference
      Serial.println("");

      // ----------- Device ID
      Serial.println("Setup -> Getting DeviceID.");
      if(cfgRAM.isKey("deviceId")){
        cfgInfo.deviceId = cfgRAM.getString("deviceId");
        Serial.printf("  |- NVRAM -> %s\n",cfgInfo.deviceId.c_str());
      }else{
        cfgInfo.deviceId = ESP.getEfuseMac();
        Serial.printf("  |- Config.cpp -> %s\n",cfgInfo.deviceId.c_str());
      }

      // ----------- Mac Address
      Serial.println();
      Serial.println("Setup -> Getting Mac Address.");
      if(cfgRAM.isKey("fixedMac")){
        cfgInfo.asset.assetMac = cfgRAM.getString("fixedMac");
        Serial.printf("  |- NVRAM -> %s\n",cfgInfo.asset.assetMac.c_str());
      }else{
        cfgInfo.asset.assetMac = WiFi.macAddress();
        Serial.printf("  |- Config.cpp -> %s\n",cfgInfo.asset.assetMac.c_str());
      } 

      // ----------- assetCode
      Serial.println();
      Serial.println("Setup -> Getting Asset Infomation.");
      if(cfgRAM.isKey("assetCode")){
        cfgInfo.asset.assetCode = cfgRAM.getString("assetCode");
        Serial.printf("  |- NVRAM -> assetCode: %s\n",cfgInfo.asset.assetCode.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> assetCode: %s\n",cfgInfo.asset.assetCode.c_str());
      } 

      // ----------- assetName
      if(cfgRAM.isKey("assetName")){
        cfgInfo.asset.assetName = cfgRAM.getString("assetName");
        Serial.printf("  |- NVRAM -> assetName: %s\n",cfgInfo.asset.assetName.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> assetName: %s\n",cfgInfo.asset.assetName.c_str());
      }       

      // ----------- assetType
      if(cfgRAM.isKey("assetType")){
        cfgInfo.asset.assetType = cfgRAM.getString("assetType");
        Serial.printf("  |- NVRAM -> assetType: %s\n",cfgInfo.asset.assetType.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> assetType: %s\n",cfgInfo.asset.assetType.c_str());
      }     

      // ----------- firmware
      if(cfgRAM.isKey("firmware")){
        cfgInfo.asset.firmware = cfgRAM.getString("firmware");
        Serial.printf("  |- NVRAM -> Firmware: %s\n",cfgInfo.asset.firmware.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> Firmware: %s\n",cfgInfo.asset.firmware.c_str());
      }           

      // ----------  Device Header
      Serial.println();
      Serial.println("Setup -> Verify Device Header.");
      if(cfgRAM.isKey("header")){
        String header  =  cfgRAM.getString("header");
        if( header == "ALVATO"){
          cfgInfo.header = header;
          Serial.printf("  |- NVRAM -> %s\n",cfgInfo.header.c_str());
        }else{
          Serial.printf("  |- Config.cpp -> %s\n",cfgInfo.header.c_str());
        }
      }else{
        cfgInfo.header ="";
        Serial.printf("  |- Config.cpp -> Header not found\n");
        Serial.printf("  |- THIS DEVICE MUST REGISTERATION FIRST\n\n");

        // -----------------  Backend Registration Process ----------------
        if(cfgInfo.header.isEmpty()){
          Serial.println("Do you want to register this deveice ? Press y/Y to register. Or any key to skip.");
          Serial.printf("Wait for key 15 secs ");

          int wtime = 0;
          while((Serial.available() < 1) && (wtime <30)){
            Serial.print("*");
            wtime++;
            delay(500);
          }

          Serial.println("");

          //************ v1.05 wait for NV-RAM ************
          byte keyin = ' ';  // Accept key y or Y from serial port only
          while(Serial.available() > 0){
            keyin = Serial.read();
              //Serial.println(keyin);
          }

          //Confirm reconfig WiFi
          if((keyin == 121) || (keyin == 89)){
            //Call API for registration process
            Serial.println("Asset registration. Please wait");
            // String response;
            int rescode = backend.assetRegister(cfgInfo.asset.branchCode.c_str(),cfgInfo.asset.assetMac.c_str(),cfgInfo.asset.firmware.c_str(),response);
            if(rescode == 200){
              DynamicJsonDocument doc(1280);
              DeserializationError error = deserializeJson(doc, response);
              if (error) {
                  Serial.print(F("deserializeJson() failed: "));
                  Serial.println(error.f_str());
              }else{
                cfgInfo.header = doc["config"]["header"].as<String>();
                cfgInfo.asset.assetCode = doc["assetCode"].as<String>();
                cfgInfo.asset.assetName = doc["assetName"].as<String>();

                cfgRAM.putString("header",cfgInfo.header);
                cfgRAM.putString("assetCode",cfgInfo.asset.assetCode);
                cfgRAM.putString("assetName",cfgInfo.asset.assetName);
                

                // cfgInfo.asset.branchCode = doc["branchCode"].as<String>();
                // cfgInfo.asset.assetName = doc["assetName"].as<String>();

                // cfgRAM.putString("branchCode",cfgInfo.asset.branchCode);
                // cfgRAM.putString("assetName",cfgInfo.asset.assetName);

                Serial.printf("New AssetInfo -> header: %s, assetCode: %s \n",cfgInfo.header.c_str(),cfgInfo.asset.assetCode.c_str());
         
              }
            }else{//Request error
              Serial.println("Asset Registration error.");
              Serial.println(response);
              myNex.writeStr("popup.msgTxt.txt","Asset Registration error. Please try again later.");
              myNex.writeStr("page 5");
              delay(5000);
              ESP.restart();
            }
          }else{
            Serial.println("Asset not register, it will continue as demo Asset.");
            myNex.writeStr("popup.msgTxt.txt","Asset not register. Continue operate as demo asset.");
            myNex.writeStr("page 5");
            delay(5000);
          }
        }
      }
    cfgRAM.end(); //Close Preference


    //OTA intial parameter
      // FOTA("alvatokiosk",cfgInfo.asset.firmware.c_str() , false, true); 

    
    
    //End OTA initial parameter


    // ----------- End OTA parameters




  

    // ------------------------    ESP Insight ------------------------
    #ifdef ESPINSIGHT
      if(WiFi.status() == WL_CONNECTED){
        if(!Insights.begin(insights_auth_key)){
            return;
        }
        Serial.println("=========================================");
        Serial.printf("ESP Insights enabled Node ID %s\n", Insights.nodeID());
        Serial.println("=========================================");
      }else{
        Serial.println("");
        Serial.printf("\nSetup -> No WIFI skip ESP Insights ");
      }
    #endif

    // ------------------------    Time Server ------------------------
    Serial.printf("\nSetup -> TimeServer\n");
    // cfginfo.asset.ntpServer1 = "1.th.pool.ntp.org";   // Comment on 1 Nov 65
    // cfginfo.asset.ntpServer2 = "asia.pool.ntp.org";   // Comment on 1 Nov 65
    cfgRAM.begin("config",false);
    if(cfgRAM.isKey("ntp1")){
      cfgInfo.asset.ntp1 = cfgRAM.getString("ntp1");
      Serial.printf("  |- NVRAM -> NTP 1: %s\n",cfgInfo.asset.ntp1.c_str());
    }else{
      Serial.printf("  |- Config.cpp -> NTP 1: %s\n",cfgInfo.asset.ntp1.c_str());
    }

    if(cfgRAM.isKey("ntp2")){
      cfgInfo.asset.ntp2 = cfgRAM.getString("ntp2");
      Serial.printf("  |- NVRAM -> NTP 2: %s\n",cfgInfo.asset.ntp1.c_str());
    }else{
      Serial.printf("  |- Config.cpp -> NTP 2: %s\n",cfgInfo.asset.ntp2.c_str());
    }
    cfgRAM.end();  
    // configTime(7*3600,3600,"1.th.pool.ntp.org","asia.pool.ntp.org");
    configTime(7*3600,3600,cfgInfo.asset.ntp1.c_str(),cfgInfo.asset.ntp2.c_str());

    //Get Current time
    getLocalTime(&timeNow);
    getTimeWithFormat(timeNowFormat,&timeNow);
    Serial.printf("  |- Epoch Time: %lu\n",mktime(&timeNow));
    Serial.printf("  |- Time Now: %s\n",asctime(&timeNow));
    Serial.printf("  |- Time Now Formated: %s\n",timeNowFormat);

  
    //Save lastest boot time.
    cfgRAM.begin("lastBooting",false);
    cfgRAM.putULong("bootEpoch",mktime(&timeNow));
    cfgRAM.putString("bootTime",asctime(&timeNow));
    cfgRAM.end();


    // ------------------------    MQTT Server ------------------------     
    Serial.println("");
    Serial.println("Setup -> MQTT Server");
    cfgRAM.begin("config",false);
    //Load config from NVRAM
    if(cfgRAM.isKey("mqtt0.host")){
      cfgInfo.mqtt[0].enable = cfgRAM.getBool("mqtt0.enable");
      cfgInfo.mqtt[0].mqtthost = cfgRAM.getString("mqtt0.host");
      cfgInfo.mqtt[0].mqttport = cfgRAM.getString("mqtt0.port");
      cfgInfo.mqtt[0].mqttuser = cfgRAM.getString("mqtt0.user");
      cfgInfo.mqtt[0].mqttpass = cfgRAM.getString("mqtt0.pass");
      cfgInfo.mqtt[0].topicPub = cfgRAM.getString("mqtt0.topicPub");
      cfgInfo.mqtt[0].topicSub = cfgRAM.getString("mqtt0.topicSub");
      Serial.printf("   |- NVRAM -> MQTT-0 Config: %s\n",cfgInfo.mqtt[0].mqtthost.c_str());
    }else{
      Serial.printf("   |- Config.cpp -> MQTT-0 Config: %s\n",cfgInfo.mqtt[0].mqtthost.c_str());
    }

    if(cfgRAM.isKey("mqtt1.host")){
      cfgInfo.mqtt[1].enable = cfgRAM.getBool("mqtt1.enable");
      cfgInfo.mqtt[1].mqtthost = cfgRAM.getString("mqtt1.host");
      cfgInfo.mqtt[1].mqttport = cfgRAM.getString("mqtt1.port");
      cfgInfo.mqtt[1].mqttuser = cfgRAM.getString("mqtt1.user");
      cfgInfo.mqtt[1].mqttpass = cfgRAM.getString("mqtt1.pass");
      cfgInfo.mqtt[1].topicPub = cfgRAM.getString("mqtt1.topicPub");
      cfgInfo.mqtt[1].topicSub = cfgRAM.getString("mqtt1.topicSub");
      Serial.printf("   |- NVRAM MQTT-1 Config: %s\n",cfgInfo.mqtt[1].mqtthost.c_str());
    }else{
      Serial.printf("   |- Confog.cpp -> MQTT-1 Config: %s\n",cfgInfo.mqtt[1].mqtthost.c_str());
    }
    cfgRAM.end();
    // -------- Connecting to mqtt broker
    mqConnection();

    // ------------------------    Getting / Setting Price button  ------------------------ 
    cfgRAM.begin("config",false);
      Serial.println("");
      Serial.printf("Setup -> topupPrice \n");
      for(int i = 1; i<=5;i++){
        if(cfgRAM.isKey(String("topupPrice"+i).c_str())){
          cfgInfo.topupPrice[i-1] = cfgRAM.getInt(String("topupPrice"+i).c_str());
          Serial.printf("  |- NVRAM -> topupPrice%d: %d\n",i,cfgInfo.topupPrice[i-1]);
        }else{
          Serial.printf("  |- Config.cpp -> topupPrice%d: %d\n",i,cfgInfo.topupPrice[i-1]);
        }
        
      }
      numBtn = sizeof(cfgInfo.topupPrice) / sizeof(cfgInfo.topupPrice[0]);
      Serial.printf("  |- Total button: %d\n\n",numBtn);

      // ----------   Getting/Setting PricePerCoin, PricePerBill, MaxTopup  ----------- 
      // ---------- PricePerCoin
      Serial.println("");
      Serial.printf("Setup -> Other parameter \n");
      if(cfgRAM.isKey("PricePerCoin")){
        cfgInfo.pricePerCoin = cfgRAM.getInt("PricePerCoin");
        Serial.printf("  |- NVRAM -> pricePerCoin: %d\n",cfgInfo.pricePerCoin);
      }else{
        Serial.printf("  |- Config.cpp -> pricePerCoin: %d\n",cfgInfo.pricePerCoin);
      }

    // ---------- PricePerBill
      if(cfgRAM.isKey("PricePerBill")){
        cfgInfo.pricePerBill = cfgRAM.getInt("PricePerBill");
        Serial.printf("  |- NVRAM -> pricePerBill: %d\n",cfgInfo.pricePerBill);
      }else{
        Serial.printf("  |- Config.cpp -> pricePerBill: %d\n",cfgInfo.pricePerBill);
      }

    // ---------- maxTopup
      if(cfgRAM.isKey("maxTopup")){
        cfgInfo.maxTopup = cfgRAM.getInt("maxTopup");
        Serial.printf("  |- NVRAM -> maxTopup: %d\n",cfgInfo.maxTopup);
      }else{
        Serial.printf("  |- Config.cpp -> maxTopup: %d\n",cfgInfo.maxTopup);
      }
      myNex.writeNum("numPad.maxValue.val",cfgInfo.maxTopup);

    // ---------- cashEnable
      if(cfgRAM.isKey("cashEnable")){
        cfgInfo.cashEnable = cfgRAM.getBool("cashEnable");
        Serial.printf("  |- NVRAM -> cashEnable: %s\n",cfgInfo.cashEnable?"true":"false");
      }else{
        Serial.printf("  |- Config.cpp -> cashEnable: %s\n",cfgInfo.cashEnable?"true":"false");
      }

    // ---------- qrEnable
      if(cfgRAM.isKey("qrEnable")){
        cfgInfo.qrEnable = cfgRAM.getBool("qrEnable");
        Serial.printf("  |- NVRAM -> qrEnable: %s\n",cfgInfo.qrEnable?"true":"false");
      }else{
        Serial.printf("  |- Config.cpp -> qrEnable: %s\n",cfgInfo.qrEnable?"true":"false");
      }    

    cfgRAM.end();


      // ----------   Getting/Setting api service  ----------- 
    cfgRAM.begin("config",false);

      // ---------- getuserinfo
      if(cfgRAM.isKey("getuserinfo")){
        cfgInfo.api.getuserinfo = cfgRAM.getString("getuserinfo");
        Serial.printf("  |- NVRAM -> getuserinfo: %s\n",cfgInfo.api.getuserinfo.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> getuserinfo: %s\n",cfgInfo.api.getuserinfo.c_str());
      }

      // ---------- updatecredit
      if(cfgRAM.isKey("updatecredit")){
        cfgInfo.api.updatecredit = cfgRAM.getString("updatecredit");
        Serial.printf("  |- NVRAM -> updatecredit: %s\n",cfgInfo.api.updatecredit.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> updatecredit: %s\n",cfgInfo.api.updatecredit.c_str());
      }

      // ---------- heartbeat
      if(cfgRAM.isKey("heartbeat")){
        cfgInfo.api.heartbeat = cfgRAM.getString("heartbeat");
        Serial.printf("  |- NVRAM -> heartbeat: %s\n",cfgInfo.api.heartbeat.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> heartbeat: %s\n",cfgInfo.api.heartbeat.c_str());
      }
    
      // ---------- newcointrans
      if(cfgRAM.isKey("newcointrans")){
        cfgInfo.api.newcointrans = cfgRAM.getString("newcointrans");
        Serial.printf("  |- NVRAM -> newcointrans: %s\n",cfgInfo.api.newcointrans.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> newcointrans: %s\n",cfgInfo.api.newcointrans.c_str());
      }

      // ---------- updatepoint
      if(cfgRAM.isKey("updatepoint")){
        cfgInfo.api.updatepoint = cfgRAM.getString("updatepoint");
        Serial.printf("  |- NVRAM -> updatepoint: %s\n",cfgInfo.api.updatepoint.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> updatepoint: %s\n",cfgInfo.api.updatepoint.c_str());
      }

      // ---------- qrgen
      if(cfgRAM.isKey("qrgen")){
        cfgInfo.api.qrgen = cfgRAM.getString("qrgen");
        Serial.printf("  |- NVRAM -> qrgen: %s\n",cfgInfo.api.qrgen.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> qrgen: %s\n",cfgInfo.api.qrgen.c_str());
      }

      // ---------- slipcheck
      if(cfgRAM.isKey("slipcheck")){
        cfgInfo.api.slipcheck = cfgRAM.getString("slipcheck");
        Serial.printf("  |- NVRAM -> slipcheck: %s\n",cfgInfo.api.slipcheck.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> slipcheck: %s\n",cfgInfo.api.slipcheck.c_str());
      }

    cfgRAM.end();

    // ----------- Setting api service to backend class
    Serial.println();
    Serial.println("Setup Parameter to API Class");
    backend.setVar("getuserinfo",cfgInfo.api.getuserinfo.c_str());
    backend.setVar("updatecredit",cfgInfo.api.updatecredit.c_str());
    backend.setVar("updatepoint",cfgInfo.api.updatepoint.c_str());
    backend.setVar("heartbeat",cfgInfo.api.heartbeat.c_str());
    backend.setVar("qrgen",cfgInfo.api.qrgen.c_str());
    backend.setVar("slipcheck",cfgInfo.api.slipcheck.c_str());
    backend.setVar("newcointrans",cfgInfo.api.newcointrans.c_str());


    // ----------- OTA parameters
    cfgRAM.begin("config",false);
      if(cfgRAM.isKey("otaurl")){
        cfgInfo.manifest_url = cfgRAM.getString("otaurl");
        Serial.printf("  |- NVRAM -> OTA_url: %s\n",cfgInfo.manifest_url.c_str());
      }else{
        Serial.printf("  |- Config.cpp -> OTA_url: %s\n",cfgInfo.manifest_url.c_str());
      }
    cfgRAM.end();






    // ------------------------    Set Price button  ------------------------ 
    // ------------------------    Set Price button  ------------------------ 
    // ------------------------    Set Price button  ------------------------ 



  }else{
    Serial.println("");
    Serial.println("   ####################################################");
    Serial.println("                       WIFI NOT READY                  ");
    Serial.println("   ####################################################");

    
  }



  // -------------------- Finish Initialize Configuraiton --------------------
  

  Serial.println("");
  Serial.println("Booting done...");
  Serial.println("************************  AlvatoKiosk Ready ************************");
  Serial.printf("            Asset Code: %s      Asset Name: %s\n",cfgInfo.asset.assetCode, cfgInfo.asset.assetName);
  Serial.printf("            Firmware: %s             IP: %s\n",cfgInfo.asset.firmware,localIP);
  Serial.println("********************************************************************");
  Serial.println();

  #ifdef EASY_NEXTION  //For command
    //Move to cover page
    myNex.writeStr("page 0"); //Cover

    //Set Cover msgTxt
    myNex.writeStr("vis msgTxt,0");
    myNex.writeStr("cover.msgTxt.pco=65535");
    myNex.writeStr("cover.msgTxt.txt","Please touch to start");
    

    //Set 1st to show on cover page
    myNex.writeStr("p0.pic=10");
    myNex.writeStr("title.pic=32"); //"กรุณาสัมพัส"
    myNex.writeStr("vis title,1");

    //Set T5 to show AssetInfo
    myNex.writeStr("t5.txt",cfgInfo.asset.assetName + " IP: " +localIP);
    myNex.writeStr("vis t5,1");

    //Set RTC on Nextion.
    getLocalTime(&timeNow);
    myNex.writeStr("rtc0=" + String(1900 + timeNow.tm_year));
    myNex.writeStr("rtc1=" + String(1 + timeNow.tm_mon));
    myNex.writeStr("rtc2=" + String(timeNow.tm_mday));
    myNex.writeStr("rtc3=" + String(timeNow.tm_hour));
    myNex.writeStr("rtc4=" + String(timeNow.tm_min));
    myNex.writeStr("rtc5=" + String(timeNow.tm_sec));

    //set maxValue for numpad
    myNex.writeNum("numpad.maxVal.val",200);
  #endif


  // --------------------------- Check State from previous operation ------------------------------
  cfgRAM.begin("state",false);
    // ----------- Get Action state flag
    Serial.println("Setup -> Getting Action stat flag");
    if(cfgRAM.isKey("actionState")){
      actionState = cfgRAM.getUInt("actionState");
      cfgRAM.putUInt("actionState",0);
    }else{
      actionState = 0; // noting to do state.
    }
  cfgRAM.end();

  if(actionState != 0){ //
    doc.clear();

    switch(actionState){
      case 1: // rebooting state
        doc["response"] = "reboot";
        doc["status"] = "success";  
        break;
      case 2: // ota state
        doc["response"] = "ota";
        doc["status"] = "success";  
        doc["firmware"] = cfgInfo.asset.firmware;
        break;
      default:
        break;
    }

    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);

    serializeJson(doc,jsonmsg);
    Serial.println();
    Serial.print("pubTopic: "); Serial.println(pubTopic);
    Serial.print("Jsonmsg: ");Serial.println(jsonmsg);

    // Check connection before send 
    if( !mqtt0.connected() || !mqtt1.connected() ){
      mqConnection();
    }

    //Send response to mqtt server
    if(cfgInfo.mqtt[0].enable){
      mqtt0.publish(pubTopic.c_str(),jsonmsg.c_str());
    }

    if(cfgInfo.mqtt[1].enable){
      mqtt1.publish(pubTopic.c_str(),jsonmsg.c_str());
    }
    
  }// end if actionstate



  //update heartbeat 
  backend.heartbeat(cfgInfo.asset.assetName.c_str(),localIP.c_str(),response);


} // end setup()








/*-------------------------------------  Loop Area -------------------------------------*/
void loop() {

  mqConnection(); // Inside function include WiFi and MQTT connection check 

  // myNex.writeStr("Cover0.dtime0.txt","Hello");
  #ifdef RFID_SPI
    // readRFID_orig();
    // readRFID();
  #endif

  #ifdef CASH
    Serial.printf("CoinValue: %d\n", coinValue);

    if(count < 200){
      digitalWrite(ENCOIN,HIGH);
      waitForPay = true;
    }else{
      digitalWrite(ENCOIN,LOW);
      waitForPay = false;
    }
    if(count >= 400){
      count=0;
    }
    Serial.printf("Count: %d\n",count++);
    delay(100);
  #endif

  #ifdef NEXTION
    if(Serial1.available()){
      // digitalWrite(BUZZ,HIGH);
      // delay(100);
      // digitalWrite(BUZZ,LOW);
      Serial.print("Serial1: ");
      Serial.println(Serial1.readString());
    }
  #endif

  #ifdef EASY_NEXTION
    if(activePage != myNex.currentPageId ){
      activePage = myNex.currentPageId;
      Serial.printf("Active Page: %d\n",activePage);

      if(WiFi.status()){
        myNex.writeStr("p1.pic=23");
      }else{
        myNex.writeStr("p1.pic=26");
      }
    }


    switch(myNex.currentPageId){
      case 0: // Cover page
        //Show t5
        myNex.writeStr("t5.txt",cfgInfo.asset.assetName + ", Fw: " +cfgInfo.asset.firmware);
        myNex.writeStr("vis t5,1");


        //Set Parameter
        showPageFlag = true;
        showSummary = true;
        slipCheckFailed=false;

        tagId=0;
        topupValue = 0;
        userinfo.tagId="";
        userinfo.name="";
        userinfo.thName="";
        userinfo.credit=0;
        userinfo.point=0;
        myNex.writeStr("topup.spv0.txt","0");



        break;
      case 1: // Page1 (topup): Read RFID and get userInfo P
        //Show t5
        myNex.writeStr("t5.txt",cfgInfo.asset.assetName + ", Fw: " +cfgInfo.asset.firmware);
        myNex.writeStr("vis t5,1");
        myNex.writeStr("topup.msgTxt.txt","Maximum Topup is " + String(cfgInfo.maxTopup));
 
        if(tagId == 0){
          //Enable timer triggger   if do nothing for 1 min then move to page 0
          myNex.writeStr("tm1.en=1");

          // set button = Disable for btn 1-5 
          for(int i=1;i<=numBtn;i++){
            myNex.writeStr("b"+String(i)+".pco=27500");
            myNex.writeStr("b" + String(i) + ".txt", String(cfgInfo.topupPrice[i-1]));
            myNex.writeStr("b"+String(i)+".picc=1");   //Button color = gray
            myNex.writeStr("tsw b"+String(i)+",0");
          }

          // set diable for btn6
          // myNex.writeStr("b6.txt","");
          myNex.writeStr("b6.picc=1");
          myNex.writeStr("tsw t6,0"); 

          //Clear Nextion screen 
          myNex.writeStr("t1.txt","");
          myNex.writeStr("t2.txt","");
          myNex.writeStr("t3.txt","");

          // Enable RFID tag reading
          tagId = getTagInfo();

          // Reading tag success full.
          if(tagId != 0){
            myNex.writeStr("tm1.en=0");
            String tag = String(tagId); 
            Serial.printf("New Tag: %u\n",tagId);
            Serial.printf("New Tag in String: %s\n",tag);
      
            // String response;

            //Call getUserInfo
            // Serial.println(cfgInfo.api.getuserinfo);
            // backend.setVar("getUserInfo",cfgInfo.api.getuserinfo.c_str());


            int rescode = backend.getUserInfo(tag.c_str(),response);

            //int rescode = backend.checkUser(tag.c_str(),response);
            

            if(rescode == 200){
              StaticJsonDocument<512> doc;
              DeserializationError error = deserializeJson(doc, response);
              if (error) {
                  Serial.print(F("deserializeJson() failed: "));
                  Serial.println(error.f_str());
              }else{
                // String tempres = doc["status"].as<String>();
                // int resstatus = tempres.toInt();

               int resstatus = doc["status"].as<signed int>();

                Serial.print("res status: ");
                Serial.println(resstatus);

                if(resstatus == 200){
                  userinfo.tagId = tag;
                  userinfo.name = doc["name"].as<String>();
                  userinfo.credit = doc["credit"].as<int>();
                  userinfo.point = doc["point"].as<int>();

                  userinfo.thName = doc["thName"].as<String>();  // Only available when use getUserInfo API

                  Serial.printf("[page1]->getuserinfo: %s\n",userinfo.name);
                  
                  // myNex.writeStr("t1.txt",userinfo.name);
                  // myNex.writeStr("t1.txt",userinfo.tagId);
                  myNex.writeStr("t1.txt",userinfo.name);
                  myNex.writeStr("t2.txt",String(userinfo.credit));
                  myNex.writeStr("t3.txt",String(userinfo.point));

                  // set button = Disable for btn 1-5 
                  for(int i=1;i<=numBtn;i++){
                    myNex.writeStr("b"+String(i)+".pco=0");   // Text Color = black
                    myNex.writeStr("b" + String(i) + ".txt", String(cfgInfo.topupPrice[i-1]));
                    myNex.writeStr("b"+String(i)+".picc=0");  //Btn color = cyan
                    myNex.writeStr("tsw b"+String(i)+",1");   // Enable btn
                  }
                  // set enable for btn
                  // myNex.writeStr("b6.txt","");
                  myNex.writeStr("b6.picc=0");  
                  myNex.writeStr("tsw b6,1"); 
                }else{
                  myNex.writeStr("popup.msgTxt.txt","User Not Found.");
                  myNex.writeStr("page 5");
                }
              }
            }else{//Request error
              Serial.println("getUserInfo request failed");
              myNex.writeStr("popup.msgTxt.txt","getUserInfo request failed. Please try again.");
              myNex.writeStr("page 5");
              tagId=0;
            }
          }
        }else{
            
            if(showPageFlag){
              for(int i=1;i<=numBtn;i++){
                myNex.writeStr("b"+String(i)+".pco=0");   // Text Color = black
                myNex.writeStr("b" + String(i) + ".txt", String(cfgInfo.topupPrice[i-1]));
                myNex.writeStr("b"+String(i)+".picc=0");  //Btn color = cyan
                myNex.writeStr("tsw b"+String(i)+",1");   // Enable btn
              }
              // set enable for btn
              // myNex.writeStr("b6.txt","");
              myNex.writeStr("b6.picc=0");  
              myNex.writeStr("tsw b6,1"); 

              // set userInfo display
              Serial.printf("Old Tag: %u\n",tagId);
              // myNex.writeStr("t1.txt",userinfo.tagId);
              myNex.writeStr("t1.txt",userinfo.name);
              myNex.writeStr("t2.txt",String(userinfo.credit));
              myNex.writeStr("t3.txt",String(userinfo.point));
              showPageFlag = false;
            }
        }     



        // end readRFID();
        break;
      case 2: // Cash page
        //Show t5
        myNex.writeStr("t5.txt",cfgInfo.asset.assetName + ", Fw: " +cfgInfo.asset.firmware);
        myNex.writeStr("vis t5,1");

        if(cfgInfo.qrEnable){ // true
          if((coinValue > 0) || (billValue > 0)){
            myNex.writeStr("b3.picc=4");
            myNex.writeStr("tsw b3,0");
          }else{
            myNex.writeStr("b3.picc=3");
            myNex.writeStr("tsw b3,1");
          }
        }else{ // false
          myNex.writeStr("b3.picc=4");
          myNex.writeStr("tsw b3,0");
        }


        if(waitForPay == true){
          Serial.printf("TopupNow: %d\n",coinValue + billValue);
        }
        
        if((coinValue > 0) || (billValue > 0)){
          myNex.writeStr("t2.txt",String(coinValue + billValue)); 

          myNex.writeStr("tsw b1,0");
          myNex.writeStr("b1.picc=4");
          myNex.writeStr("tsw b2,0");
          myNex.writeStr("b2.picc=4");


          Serial.printf("TopupValue: %d\n",topupValue);
    
          if((coinValue + billValue) >= topupValue){
            myNex.writeStr("t2.txt",String(coinValue + billValue));

            if((coinValue + billValue) > topupValue){
              Serial.println("Topup Higher");
            }else if((coinValue + billValue)== topupValue){
              Serial.println("Topup equal");
              
            }

            waitForPay = false;
            digitalWrite(ENCOIN,LOW);
            Serial.println("Disable ENCOIN");

            coinCount = 0;
            coinValue = 0;
            billCount = 0;
            billValue = 0;

            delay(3000);
            showSummary = true;

            //Update Transaction and student database
            // String response;
            int rescode = backend.updateCredit(userinfo.tagId.c_str(),topupValue,cfgInfo.asset.assetName.c_str(),response);
            //int rescode = backend.updateCredit(userinfo.tagId.c_str(),userinfo.credit+topupValue,cfgInfo.asset.assetName.c_str(),response);
            
            //int rescode = backend.topupKiosk(userinfo.tagId.c_str(),userinfo.credit+topupValue,cfgInfo.asset.assetCode.c_str(),response);

            if(rescode == 200){
              // rescode = backend.newCoinTrans(cfgInfo.asset.branchCode.c_str(),cfgInfo.asset.assetCode.c_str(),userinfo.tagId.c_str(),topupValue,response);
              // if(rescode == 200){
              //   myNex.writeStr("page 4");
              //   // delay(1000);
              // }
              StaticJsonDocument<512> doc;
              DeserializationError error = deserializeJson(doc, response);
              if (error) {
                  Serial.print(F("deserializeJson() failed: "));
                  Serial.println(error.f_str());
              }else{
                myNex.writeStr("page 4");
              }
            }else{
              //Faile to update.
              myNex.writeStr("popup.msgTxt.txt","Failed to update credit. Please contack staff");
              myNex.writeStr("page 5");
            }
          }
        }        
        break;
      case 3: // QR Page
        //Show t5
        myNex.writeStr("t5.txt",cfgInfo.asset.assetName + ", Fw: " +cfgInfo.asset.firmware);
        myNex.writeStr("vis t5,1");

        #ifdef QRREADER
            if(qrSlip.isEmpty()){
              Serial.println("Reading Slip");

              //Reading miniQR from customer
              if(Serial2.available()){
                digitalWrite(BUZZ,HIGH);
                delay(100);
                digitalWrite(BUZZ,LOW);
                qrSlip = Serial2.readString();
                qrSlip.trim();
                Serial.print("Serial2: ");
                Serial.printf("qr2: %s\n",qrSlip.c_str());
              }
              
              
              if(!qrSlip.isEmpty()){
                //Call api for slipCheck
                // String response;
                int rescode = backend.slipCheck(cfgInfo.asset.branchCode.c_str(),cfgInfo.asset.assetCode.c_str(),
                userinfo.tagId.c_str(),
                qrSlip.c_str(),
                qrGenId.c_str(),
                response);

                Serial.printf("[Page3 slipCheck]->rescode: %d\n",rescode);

                //End serial2 port
                Serial2.end();

                if(rescode == 200){
                  //Display correct icon.
                  myNex.writeStr("p2.pic=16");
                  myNex.writeStr("vis p2,1");
          
                  //UpdateCredit here.  check by qrgenId
                  rescode = 0;
                  rescode = backend.updateCredit(userinfo.tagId.c_str(),topupValue,cfgInfo.asset.assetName.c_str(),response);
                  if(rescode == 200){
                    StaticJsonDocument<512> doc;
                    DeserializationError error = deserializeJson(doc, response);
                    if (error) {
                        Serial.print(F("deserializeJson() failed: "));
                        Serial.println(error.f_str());
                    }else{
                      myNex.writeStr("page 4");
                      // delay(1000);
                    }
                  }else{
                    //Faile to update.
                    myNex.writeStr("popup.msgTxt.txt","Failed to update credit. Please contack staff");
                    myNex.writeStr("page 5");
                  }
                }else{
                  slipCheckFailed=true;
                  myNex.writeStr("p2.pic=17");
                  myNex.writeStr("vis p2,1");
                  switch(rescode){
                    case 400: // Passing parameter not correct.
                      Serial.println("get 400");
                      myNex.writeStr("popup.msgTxt.txt","Input parameters not correct.");
                      break;
                    case 409: // Slip invalid. the reasons can be not company slip, duplicate slip, asset and slip not match.
                      Serial.println("get 409");
                      myNex.writeStr("popup.msgTxt.txt","Slip invalid (Duplicate, Fake slip, info not match)");
                      break;
                    case 500: // Server
                      Serial.println("get 500");
                      myNex.writeStr("popup.msgTxt.txt","Internal server error.");
                      break;
                  }
                  myNex.writeStr("page 5");
                }
              }
            }else{
              //1st check failed. retry.
              if(slipCheckFailed){
                slipCheckFailed = false;
                qrSlip = "";
                // Serial2.eventQueueReset();
                // Serial2.flush();
                Serial2.begin(9600,SERIAL_8N1, RXU2, TXU2);
      
                delay(5000);
                myNex.writeStr("page 3");
                myNex.writeStr("qr0.txt",qrPrice);
                myNex.writeStr("shopName.txt",shopName);
                myNex.writeStr("vis p2,0");
              }
            }
        #endif
        break;
      case 4: // Summary Page
        //Show t5
        myNex.writeStr("t5.txt",cfgInfo.asset.assetName + " IP: " +localIP);
        myNex.writeStr("vis t5,1");
        // myNex.writeNum("tm1.tim",4000);
        
        // call api for update 
        // Serial.printf("Active Page: %d\n",myNex.currentPageId);

        if(showSummary){
          digitalWrite(BUZZ,HIGH);
          delay(150);
          digitalWrite(BUZZ,LOW);
          
          digitalWrite(BUZZ,HIGH);
          delay(150);
          digitalWrite(BUZZ,LOW);
          
          showSummary = false;

          myNex.writeStr("t1.txt",String(userinfo.credit));
          myNex.writeStr("t2.txt",String(userinfo.credit + topupValue));

          //Reset all variable
          tagId=0;
          topupValue = 0;

          userinfo.tagId="";
          userinfo.name="";
          userinfo.thName="";
          userinfo.credit=0;
          userinfo.point=0;

          qrPrice = "";
          qrSlip = "";
          showPageFlag = true;
          delay(4000);
          myNex.writeStr("page 0");
        }
        break;
      case 6: // numPad Page
        // if(topupValue > 0){
        //   myNex.writeStr("show.txt",String(topupValue));
        // }
        break;
      case 7: // Login Page
        myNex.writeStr("t5.txt",cfgInfo.asset.assetName + ", Fw: " +cfgInfo.asset.firmware);
        myNex.writeStr("vis t5,1");

        myNex.writeStr("b1.picc=18");
        myNex.writeStr("b1.picc2=19");
        myNex.writeStr("b2.picc=18");
        myNex.writeStr("b2.picc2=19");
        break;
    }
    myNex.NextionListen();
  #endif

  mqtt0.loop();
  mqtt1.loop();

}// End Loop







/*-------------------------------------  Function Area -------------------------------------*/

String getdeviceid(void){
    char chipname[13];
    uint64_t chipid = ESP.getEfuseMac();

    snprintf(chipname, 13, "%04X%08X", (uint16_t)(chipid >> 32),(uint32_t)chipid);
    
    return chipname;
}

void trigger0(){     //login page
  Serial.println("Trigger0: Login page");
  // myNex.writeStr("page 7");

}

void trigger1(){ // Cancel on topupPage 
  Serial.println("Trigger1: Cancel on topup Page");

  tagId = 0;
  credit = 0;
  topupValue = 0;
  userinfo.tagId = "";
  userinfo.name = "";
  userinfo.thName = "";
  userinfo.credit = 0;
  userinfo.point = 0;
  myNex.writeStr("page 0");
}

void trigger2(){ //Restart Btn on topupPage
  Serial.println("Trigger2: Restart Btn on topupPage");

  tagId = 0;
  topupValue = 0;
  userinfo.tagId = "";
  userinfo.name = "";
  userinfo.thName = "";
  userinfo.credit = 0;
  userinfo.point = 0;

  myNex.writeStr("t1.txt","");
  myNex.writeStr("t2.txt","");
  myNex.writeStr("t3.txt","");
}

void trigger3(){ // Price Select btn on topupPage
  Serial.println("Trigger3: Price selection btn1-5 on topupPage");

  //Get price from Nextion page Topup   topup.spv0
  topupValue = myNex.readStr("topup.spv0.txt").toInt();
  Serial.printf("[Trigger4]->TopupValue: %d\n",topupValue);

  //Check over maxTopup
  if(topupValue < cfgInfo.maxTopup){
    digitalWrite(ENCOIN,HIGH);
    waitForPay = true;
  }else{
    myNex.writeStr("Topup value more than " + String(cfgInfo.maxTopup));
    myNex.writeStr("page 5");    
  }

  //Set page 2
  myNex.writeStr("page 2");
  myNex.writeStr("t2.txt","0");
}

void trigger4(){ // Return Btn on CashPage
  Serial.println("Trigger4: Cancel Btn on CashPage");

  digitalWrite(ENCOIN,LOW);
  waitForPay = false;
  myNex.writeStr("page 1");

  for(int i=1;i<=numBtn;i++){
    myNex.writeStr("b"+String(i)+".pco=0");   // Text Color = black
    myNex.writeStr("b" + String(i) + ".txt", String(cfgInfo.topupPrice[i-1]));
    myNex.writeStr("b"+String(i)+".picc=0");  //Btn color = cyan
    myNex.writeStr("tsw b"+String(i)+",1");   // Enable btn
  }
  // set enable for btn
  // myNex.writeStr("b6.txt","");
  myNex.writeStr("b6.picc=0");  
  myNex.writeStr("tsw b6,1"); 

  // set userInfo display
  Serial.printf("Old Tag: %u\n",tagId);
  myNex.writeStr("t1.txt",userinfo.tagId);
  myNex.writeStr("t2.txt",String(userinfo.credit));
  myNex.writeStr("t3.txt",String(userinfo.point));
}


void trigger5(){ // Cancel btn on cashPage
  Serial.println("Trigger5: Cancel btn on cashPage");
  digitalWrite(ENCOIN,LOW);
  waitForPay = false;
  topupValue = 0;
  coinCount = 0;
  coinValue = 0;
  billCount = 0;
  billValue = 0;

  tagId = 0;
  credit = 0;
  userinfo.tagId = "";
  userinfo.name = "";
  userinfo.thName = "";
  userinfo.credit = 0;
  userinfo.point = 0;

  #ifdef QRREADER
    qrPrice = "";
    qrSlip = "";
  #endif

  myNex.writeStr("page 0");
}


void trigger6(){ // QR Payment button page3
  // String response;
  Serial.println("Trigger6: QR Payment on cashPage");
  
  //Disable QR button
  myNex.writeStr("b3.picc=4");
  myNex.writeStr("tsw b3,0");

  digitalWrite(ENCOIN,LOW);
  waitForPay = false;

  Serial2.begin(9600,SERIAL_8N1, RXU2, TXU2);
  
  if(qrPrice.isEmpty() && (topupValue > 0)){
    Serial.println("Request QR Gen API Please wait.");

    backend.setVar("qrgen",cfgInfo.api.qrgen.c_str());  // Make sure qrgen variable is set.
    int res = backend.qrgen(cfgInfo.asset.branchCode.c_str(),cfgInfo.asset.assetCode.c_str(),topupValue,response);
    if(res == 200){
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, response);

      qrPrice = doc["qrtext"].as<String>();
      qrGenId = doc["qrgenID"].as<String>();
      shopName = doc["shopName"].as<String>();

      //Move to qr payment page
      myNex.writeStr("page 3");
      myNex.writeStr("qr0.txt",qrPrice);
      myNex.writeStr("shopName.txt",shopName);

    }else{
      qrPrice = "";
      shopName = "";
      Serial.println("Request QRgen failed. please try again.");
      myNex.writeStr("page 5");  // Activate Popup page
      myNex.writeStr("popup.msgTxt.txt","Request QRgen failed. please try again.");

      delay(3000);
      myNex.writeStr("page 2");
      myNex.writeStr("t2.txt","0");
      digitalWrite(ENCOIN,HIGH);
      waitForPay = true;
    }
  }
}

void trigger7(){
  Serial.println("Trigger7: Return Btn on QR Page");

  qrPrice = "";
  qrSlip = "";
  shopName = "";
  Serial2.flush();
  Serial2.eventQueueReset();
  Serial2.end();

  myNex.writeStr("page 2");
  digitalWrite(ENCOIN,HIGH);
  waitForPay = true;
  slipCheckFailed=false;
}

void trigger8(){
  Serial.println("Trigger8: Cancel Btn on QR Page");

  digitalWrite(ENCOIN,LOW);
  waitForPay = false;
  topupValue = 0;
  coinCount = 0;
  coinValue = 0;
  billCount = 0;
  billValue = 0;

  tagId = 0;
  credit = 0;
  userinfo.tagId = "";
  userinfo.name = "";
  userinfo.thName = "";
  userinfo.credit = 0;
  userinfo.point = 0;

  
  qrPrice = "";
  qrSlip = "";
  shopName = "";

  myNex.writeStr("page 0");  
}

void trigger9(){  // Cancel from numPad
  Serial.println("Trigger9: Cancel from numPad");
  topupValue=0;
  digitalWrite(ENCOIN,LOW);
  waitForPay = false;
  myNex.writeStr("page 1");

  for(int i=1;i<=numBtn;i++){
    myNex.writeStr("b"+String(i)+".pco=0");   // Text Color = black
    myNex.writeStr("b" + String(i) + ".txt", String(cfgInfo.topupPrice[i-1]));
    myNex.writeStr("b"+String(i)+".picc=0");  //Btn color = cyan
    myNex.writeStr("tsw b"+String(i)+",1");   // Enable btn
  }
  // set enable for btn
  // myNex.writeStr("b6.txt","");
  myNex.writeStr("b6.picc=0");  
  myNex.writeStr("tsw b6,1"); 

  // set userInfo display
  Serial.printf("Old Tag: %u\n",tagId);
  myNex.writeStr("t1.txt",userinfo.tagId);
  myNex.writeStr("t2.txt",String(userinfo.credit));
  myNex.writeStr("t3.txt",String(userinfo.point));  

}

void trigger10(){
  Serial.println("Trigger10");
}



// //********************************* Interrupt Function **********************************
gpio_config_t io_config;
xQueueHandle gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
  long gpio_num = (long) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void gpio_task(void *arg){
    gpio_num_t io_num;  

    for(;;){
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {  
            Serial.printf("\n[INT] GPIO[%d] intr, val: %d \n", io_num, gpio_get_level(io_num));
        } 

        switch(io_num){
            case COININ:
                if((gpio_get_level(io_num) == 0) && waitForPay){
                    coinCount++;
                    coinValue = coinCount * cfgInfo.pricePerCoin;
                    // topupValue += coinValue;
                    Serial.printf("CoinValue: %d\n",coinValue);
                }  
                break;
            case BILLIN:
                if((gpio_get_level(io_num) == 0) && waitForPay){
                    billCount++;
                    billValue = billCount * cfgInfo.pricePerBill;
                    // topupValue += billValue;
                    Serial.printf("BillValue: %d\n",billValue);
                }
                break;
            // case DSTATE:
            //     if(gpio_get_level(io_num) == 0){
            //       Serial.printf("[intr]->Door Open\n");
            //     }else{
            //       Serial.printf("[intr]->Door Close\n");
            //     }
            //     break;
            // case MODESW:
            //     break;
        }  
    }
}

void init_interrupt(void){
    //gpio_config_t io_conf;
    //This setting for Negative LOW coin Module
    Serial.printf("  Execute---Initial Interrupt Function\n");

    io_config.intr_type = GPIO_INTR_NEGEDGE;    
    io_config.pin_bit_mask = INTERRUPT_SET;
    io_config.mode = GPIO_MODE_INPUT;
    io_config.pull_up_en = (gpio_pullup_t)1;

    //configure GPIO with the given settings
    gpio_config(&io_config);

    //gpio_set_intr_type((gpio_num_t)COININ, GPIO_INTR_NEGEDGE);

    /*********** create a queue to handle gpio event from isr ************/
    gpio_evt_queue = xQueueCreate(10, sizeof(long)); 

    /*********** Set GPIO handler task ************/
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL); 

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

  
    gpio_isr_handler_add((gpio_num_t)COININ, gpio_isr_handler, (void*) COININ);
    gpio_isr_handler_add((gpio_num_t)BILLIN, gpio_isr_handler, (void*) BILLIN);
    //gpio_isr_handler_add((gpio_num_t)DSTATE, gpio_isr_handler, (void*) DSTATE);
    //gpio_isr_handler_add((gpio_num_t)MODESW, gpio_isr_handler, (void*) MODESW);
    //gpio_isr_handler_add((gpio_num_t)COINDOOR, gpio_isr_handler, (void*) COINDOOR);

}


// //********************************* End of Interrupt Function ********************************



//---------------------------------------- RFID ------------------------------------------

uint32_t getTagInfo(void){
  char buff[5]; // 3 digits, dash and \0.
  unsigned long cardTag=0;
          
  if ( ! rfid.PICC_IsNewCardPresent()) {
    return 0;
  }

  // Select one of the cards
  if ( ! rfid.PICC_ReadCardSerial()) {
    Serial.println("Bad read (was card removed too quickly?)");
    return 0;
  }

  if (rfid.uid.size == 0) {
    Serial.println("Bad card (size = 0)");
  } else {
    digitalWrite(BUZZ,HIGH);
    delay(120);
    digitalWrite(BUZZ,LOW);
    char tag[sizeof(rfid.uid.uidByte) * 4] = { 0 };

    for (int i = 0; i < rfid.uid.size; i++) {
      snprintf(buff, sizeof(buff), "%s%02X", i ? "-" : "", rfid.uid.uidByte[i]);
      strncat(tag, buff, sizeof(tag));
    };
    
    Serial.print("Good scan: ");
    Serial.println(tag);

    cardTag = hex2dec(rfid.uid.uidByte, rfid.uid.size);
    Serial.printf("Card in DEC: %u\n",cardTag);
  };

  // disengage with the card.
  rfid.PICC_HaltA();  

  return (uint32_t)cardTag;
}

void readRFID(void) { /* function readRFID */
  byte nuidPICC[4] = {0, 0, 0, 0};
  ////Read RFID card
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if ( ! rfid.PICC_IsNewCardPresent())
      return;
  // Verify if the NUID has been readed
  if (  !rfid.PICC_ReadCardSerial())
      return;
  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
  }
  activetime = 0;

  Serial.print(F("RFID In Hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  Serial.print("Result: ");
  Serial.println(hex2dec(rfid.uid.uidByte, rfid.uid.size));
  Serial.println();
  
  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  delay(1000);
}

/**
    Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  unsigned long cValue = 0;
  for (int i = 3; i >= 0; i--) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
      Serial.print(buffer[i], HEX);
  }
  // Serial.println(":");
  // cValue = (buffer[3] * 16777216) + (buffer[2] * 65536) + (buffer[1] * 256) + (buffer[0]);
  // Serial.println();
  // Serial.print("Value in Dec: ");
  // Serial.println(cValue);
  //CheckPHP(cValue); set http require for API
}

/**
    Helper routine to dump a byte array as dec values to Serial.
*/



/* ----------------------------------- function printDec ---------------------------------- */
void printDec(byte *buffer, byte bufferSize) {
  String card = "";
  for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
      Serial.print(buffer[i], DEC);
  }
}


/* ----------------------------------- function hex2dec ---------------------------------- */
unsigned long hex2dec(byte *buffer, byte bufferSize){
  return (buffer[3] * 16777216) + (buffer[2] * 65536) + (buffer[1] * 256) + (buffer[0]);
}



/* ----------------------------------- function mqConnection---------------------------------- */

void mqConnection(void){
  String jsonMsg = "";
  // char timeStamp[50];

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi Connection Lost....retry to connect");
    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      wifiMulti.run();
      delay(500);
    }

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("WiFi reonnected to the network!");
      Serial.printf("WiFi SSID: -> %s\n",WiFi.SSID().c_str());  
      localIP = WiFi.localIP().toString();
      Serial.printf("WiFi IP: -> %s\n",localIP.c_str());
      Serial.printf("WiFi MAC: -> %s\n",WiFi.BSSIDstr().c_str());
      Serial.printf("WiFi Channel: -> %d\n",WiFi.channel());
    }
  }


  if( !mqtt0.connected() && cfgInfo.mqtt[0].enable ){
    subTopic = cfgInfo.mqtt[0].topicSub + cfgInfo.asset.branchCode + "/" + cfgInfo.asset.assetCode;
    pubTopic = cfgInfo.mqtt[0].topicPub + cfgInfo.asset.branchCode + "/" + cfgInfo.asset.assetCode;

    mqtt0.setServer(cfgInfo.mqtt[0].mqtthost.c_str(),cfgInfo.mqtt[0].mqttport.toInt());
    mqtt0.setCallback(mqCallback);

    mqtt0.connect(cfgInfo.deviceId.c_str(),cfgInfo.mqtt[0].mqttuser.c_str(),cfgInfo.mqtt[0].mqttpass.c_str());
    if(mqtt0.connected()){
      mqtt0.subscribe(subTopic.c_str());
      Serial.printf("\n   |- Used MQTT-0 Host: %s\n",cfgInfo.mqtt[0].mqtthost.c_str());
      Serial.println("     |- Connected to MQTT-0");
      Serial.printf("     |- MQTT-0 Subscribe Topic: %s\n",subTopic.c_str());

      jsonMsg = " {\"Infomation\" : \"Mqtt connected\",\"timeStamp\":\" " + String(timeNowFormat) + " \" } ";
      
      Serial.printf("     |- MQTT-0 Publish response: %s\n",jsonMsg.c_str());
      mqtt0.publish(pubTopic.c_str(),jsonMsg.c_str());
    }else{
      Serial.print("     |- Not connected to MQTT-0 \n");
    }
  }


  if( !mqtt1.connected() && cfgInfo.mqtt[1].enable ){
    subTopic = cfgInfo.mqtt[1].topicSub + cfgInfo.asset.branchCode + "/" + cfgInfo.asset.assetCode;
    pubTopic = cfgInfo.mqtt[1].topicPub + cfgInfo.asset.branchCode + "/" + cfgInfo.asset.assetCode;

    mqtt1.setServer(cfgInfo.mqtt[1].mqtthost.c_str(),cfgInfo.mqtt[1].mqttport.toInt());
    mqtt1.setCallback(mqCallback);
   
    mqtt1.connect("cfgInfo.deviceId.c_str()",cfgInfo.mqtt[1].mqttuser.c_str(),cfgInfo.mqtt[1].mqttpass.c_str());
    if(mqtt1.connected()){
      mqtt1.subscribe( subTopic.c_str() );
      Serial.printf("\n   |- Used MQTT-1 Host: %s\n",cfgInfo.mqtt[1].mqtthost.c_str());
      Serial.println("     |- Connected to MQTT-1");
      Serial.printf("     |- MQTT-1 Subscript Topic: %s\n",subTopic.c_str());
    }else{
      Serial.print("     |- Not connected to MQTT-1 \n");
    }
  }



}


/* ----------------------------------- function mqCallback ---------------------------------- */
void mqCallback(char* topic, byte* payload, unsigned int length){
  // String jsonmsg;
  // DynamicJsonDocument doc(512);
  bool rebootFlag = false;
  bool updatedNeeded = false;
  bool publishFlag = false;

  Serial.println();
  Serial.println("Message arrived mqtt with topic: ");
  Serial.println(topic);
  Serial.println();

  Serial.println("This payload: ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  DeserializationError error = deserializeJson(doc, payload);
  // Test if parsing succeeds.

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  String action = doc["action"].as<String>();
  Serial.print("\nThis action paramater: ");
  Serial.println(action);
 
  
  //For each action 
  if( (action == "reboot") || (action == "reset") || (action == "restart")){
    //------------------------------ Action Reboot -----------------------------
    getTimeWithFormat(timeNowFormat,&timeNow);
    
    doc.clear();
    doc["response"] = "reboot";
    doc["status"] = "rebooting";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);
    publishFlag = true;
    rebootFlag = 1;
    // delay(1000);
    // ESP.restart();

  }else if(action == "ping"){  
    //------------------------------ Action Ping -----------------------------
    // getLocalTime(&timeNow);
    // strftime (timeNowFormat,40,"%d-%b-%G %T",&timeNow);

    getTimeWithFormat(timeNowFormat,&timeNow);
    
    doc.clear();
    doc["response"]="ping";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);
    doc["details"]["rssi"]=WiFi.RSSI();
    doc["details"]["firmware"]=cfgInfo.asset.firmware;
    publishFlag = true;

  }else if(action == "ota"){
    //------------------------------ Action OTA -----------------------------
    FOTA.setManifestURL( cfgInfo.manifest_url.c_str() ); // set url for ota.json
    updatedNeeded = FOTA.execHTTPcheck();

    doc.clear();
    doc["response"] = "ota";
    if(updatedNeeded){
      doc["status"] = "Updating";
    }else{
      doc["status"] = "Rejected";
    }

    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);
    publishFlag = true;

  }else if(action == "smartconfig"){
   Serial.println("Execute Action: smartConfig"); 
   //--------------------- Set WiFi with ESPTouch SmartConfig -------------------- 
    wifiCFG(cfgRAM, cfgInfo);

    doc.clear();
    doc["response"]="smartConfig";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["SSID"]=WiFi.SSID();
    doc["details"]["IP"]=WiFi.localIP();
    doc["details"]["RSSI"]=WiFi.RSSI();
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);
    publishFlag = true;

  }else if(action == "topupPrice"){
   //------------------------------ Set Price Button -----------------------------
  //  topupPrice = doc["price"].as<int>();
    Serial.println("Execute Action: topupPrice");
    JsonArray price = doc["price"].as<JsonArray>();
 
    cfgRAM.begin("config",false);
    int i = 1;
    for(JsonVariant v : price) {
      cfgRAM.putInt(("topupPrice" + i),v["value"].as<int>());
      cfgInfo.topupPrice[i-1] = cfgRAM.getInt(("topupPrice"+i));
      Serial.printf("NewPrice: %d\n",cfgInfo.topupPrice[i-1]);
      i++;
    }
    cfgRAM.end(); 
    
    getTimeWithFormat(timeNowFormat,&timeNow);

    doc.clear();
    doc["response"]="topupPrice";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);
    publishFlag = true;

  }else if(action == "setAsset"){
    String key = doc["key"]; //assetName, assetType, firmware, userPass, ntp1, ntp2
    String value = doc["value"];

    Serial.printf("Execute Action: setAsset -> key: %s, value: %s\n", key,value);

    // ----------- Save to NVRAM
    cfgRAM.begin("config",false);
      cfgRAM.putString(key.c_str(),value);
    cfgRAM.end();

    String tmpStr = String(key);
    tmpStr.toLowerCase();

    if(strcmp("assetName",tmpStr.c_str()) == 0){
        cfgInfo.asset.assetName = value;
    }else if(strcmp("assetType",tmpStr.c_str()) ==0 ){
        cfgInfo.asset.assetType = value;
    }else if(strcmp("firmware",tmpStr.c_str()) ==0 ){
        cfgInfo.asset.firmware = value;
    }else if(strcmp("userPass",tmpStr.c_str()) ==0 ){
        cfgInfo.asset.userPass = value;
    }

    Serial.println("Saved Variable to NVRAM.");

    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="setAsset";
    doc["key"]=key;
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);
    publishFlag = true;

  }else if(action == "setWifi"){
    
    String key = doc["key"];
    String value = doc["value"];

    Serial.println("Execute Action: WiFi");
    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="topupPrice";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);    
    publishFlag = true;

  }else if(action == "setMqtt"){
    String key = doc["key"]; //mqtt0.host, mqtt1.host
    String value = doc["value"];

    Serial.println("Execute Action: MQTT");
    cfgRAM.begin("config",false);
      cfgRAM.putString(key.c_str(),value);
    cfgRAM.end();

    String tmpStr = String(key);
    tmpStr.toLowerCase();

    Serial.printf("  |- setVar -> %s: %s\n",tmpStr.c_str(),value);
    
    if(strcmp("mqtt0.host",tmpStr.c_str()) == 0){
        cfgInfo.mqtt[0].mqtthost = value;
    }else if(strcmp("mqtt0.port",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[0].mqttport = value;
    }else if(strcmp("mqtt0.user",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[0].mqttuser = value;
    }else if(strcmp("mqtt0.pass",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[0].mqttpass = value;
    }else if(strcmp("mqtt0.topicPub",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[0].topicPub = value;
    }else if(strcmp("mqtt0.topicSub",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[0].topicSub = value;
    }

    if(strcmp("mqtt1.host",tmpStr.c_str()) == 0){
        cfgInfo.mqtt[1].mqtthost = value;
    }else if(strcmp("mqtt1.port",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[1].mqttport = value;
    }else if(strcmp("mqtt1.user",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[1].mqttuser = value;
    }else if(strcmp("mqtt1.pass",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[1].mqttpass = value;
    }else if(strcmp("mqtt1.topicPub",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[1].topicPub = value;
    }else if(strcmp("mqtt1.topicSub",tmpStr.c_str()) ==0 ){
        cfgInfo.mqtt[1].topicSub = value;
    }


    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="setMqtt";
    doc["key"]=key;
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow); 
    publishFlag = true;  

  }else if(action == "setVar"){  // Uri
    String varName = doc["varName"].as<String>();
    String value = doc["value"].as<String>();;
    Serial.println("Execute Action: setVar");
    
    // ----------- Save to NVRAM
    cfgRAM.begin("config",false);
      varName.toLowerCase();
      cfgRAM.putString(varName.c_str(),value);
    cfgRAM.end();
    Serial.println("Saved Variable to NVRAM.");

    // ------------ For Testing only
    backend.setVar(varName.c_str(),value.c_str());
    Serial.println("Set variable to API Class");
    // ------------

    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="setVar";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;   
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);   
    doc["details"]["varName"]=varName;
    doc["details"]["value"]=value;
    publishFlag = true;

  }else if(action == "cashEnable"){
    bool value = doc["value"];

    Serial.println("Execute Action: cashEnable");
    Serial.println(value);

    cfgRAM.begin("config",false);
      cfgInfo.cashEnable = value;
      cfgRAM.putBool("cashEnable",cfgInfo.cashEnable);
    cfgRAM.end();

    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="cashEnable";
    doc["status"]="success";
    doc["value"] = cfgInfo.cashEnable;
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);  
    publishFlag = true;  

  }else if(action == "qrEnable"){
    bool value = doc["value"];
    Serial.println("Execute Action: qrEnable");
    Serial.println(value);

    cfgRAM.begin("config",false);
      cfgInfo.qrEnable = value;
      cfgRAM.putBool("qrEnable",cfgInfo.qrEnable);
    cfgRAM.end();

    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="qrEnable";
    doc["status"]=cfgInfo.qrEnable?"enable":"disable";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);  
    publishFlag = true;

  }else if(action == "pricePerCoin"){
    int value = doc["value"].as<int>();;
    Serial.println("Execute Action: pricePerCoin");



    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="pricePerCoin";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;   
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);    
    publishFlag = true;

  }else if(action == "pricePerBill"){
    int value = doc["value"].as<int>();;
    Serial.println("Execute Action: pricePerBill");



    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="pricePerBill";
    doc["status"]="success";
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);    
    publishFlag = true;

  }else if(action == "maxTopup"){
    int value = doc["value"].as<int>();;

    Serial.println("Execute Action: maxTopup");
    cfgRAM.begin("config",false);
      cfgInfo.maxTopup = value;
      cfgRAM.putInt("maxTopup",cfgInfo.maxTopup);
    cfgRAM.end();

    myNex.writeNum("numPad.maxValue.val",cfgInfo.maxTopup);
    myNex.writeStr("topup.msgTxt.txt","Maximum Topup is " + String(cfgInfo.maxTopup));

    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="maxTopup";
    doc["status"]="success";
    doc["maxTopup"] = cfgInfo.maxTopup;
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);   
    publishFlag = true;
  }else if(action == "unLock"){
    bool value = doc["value"];

    if(value){
      digitalWrite(UNLOCK,value);
    }else{
      digitalWrite(UNLOCK,value);
    }

    getTimeWithFormat(timeNowFormat,&timeNow);
    doc.clear();
    doc["response"]="unLock";
    doc["status"]=value;
    doc["details"]["branchCode"] =cfgInfo.asset.branchCode;
    doc["details"]["assetCode"]=cfgInfo.asset.assetCode;    
    doc["details"]["timeStamp"]=timeNowFormat;
    doc["details"]["timeEpoch"]=mktime(&timeNow);   
    publishFlag = true;   
  }

  if(publishFlag){
    jsonmsg = "";
    serializeJson(doc,jsonmsg);
    Serial.println();
    Serial.print("pubTopic: "); Serial.println(pubTopic);
    Serial.print("Jsonmsg: ");Serial.println(jsonmsg);

    // Check connection before send 
    if( !mqtt0.connected() || !mqtt1.connected() ){
      mqConnection();
    }

    //Send response to mqtt server
    if(cfgInfo.mqtt[0].enable){
      mqtt0.publish(pubTopic.c_str(),jsonmsg.c_str());
    }

    if(cfgInfo.mqtt[1].enable){
      mqtt1.publish(pubTopic.c_str(),jsonmsg.c_str());
    }
    publishFlag = false;
  }

  if(rebootFlag == 1){
    //Mark reboot state to NVRAM
    cfgRAM.begin("state",false);
      cfgRAM.putUInt("actionState",1); //Rebooting
    cfgRAM.end();

    rebootFlag = 0;
    delay(500);
    ESP.restart();
  }

  // OTA Update process
  if (updatedNeeded){;
    myNex.writeStr("page 0");
    myNex.writeStr("vis title,0");
    myNex.writeStr("p0.pic=13");
    myNex.writeStr("msgTxt.y=320");
    myNex.writeStr("msgTxt.pco=63488");
    myNex.writeStr("vis msgTxt,1");
    myNex.writeStr("msgTxt.txt","Updating Firmware");
    FOTA.execOTA();
  }
}



void alvatoOtaFinishedCb(int partition, bool restart_after) {
    cfgRAM.begin("state",false);
      cfgRAM.putUInt("actionState",2); //Mark update firmware
    cfgRAM.end();
    Serial.println("[actionState] -> set flag OTA update finish. ");
}