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

Preferences cfgRAM;
CONFIG cfgInfo;
USERINFO userinfo;

int coinValue = 0;
int coinCount = 0;
int billValue = 0;
int billCount = 0;
int topupValue = 0;

int pricePerCoin = 1;
int pricePerBill = 10;

//For RFID Page
uint32_t tagId = 0;
int credit = 0;
int newCredit = 0;



boolean waitForPay = true;
boolean showSummary = false;
boolean showPageFlag = true;
int activePage = -1;

int topupPrice[]={1,2,5,10,20};


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


#ifdef NEXTION
#endif



#ifdef QRREADER
  String qrPrice="";
  String qrSlip="";
#endif

#ifdef EASY_NEXTION
  EasyNex myNex(Serial1);
  String topupValueStr;
#endif



void coincount(void);
void IRAM_ATTR gpio_isr_handler(void* arg);
void gpio_task(void *arg);
void init_interrupt(void);

void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);

unsigned long hex2dec(byte *buffer, byte bufferSize);
void readRFID(void);

uint32_t getTagInfo(void);

//Communication 
WiFiMulti wifiMulti;
WiFiClient espclient;

//Time
time_t tnow;
tm timenow;

alvato backend;


void setup() {
  initGPIO(INPUT_SET,OUTPUT_SET); 
  init_interrupt();

  Serial.begin(115200);

  #ifdef RGBLED 
    FastLED.addLeds<SK6812, RGB_LED, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    leds[0] = CRGB::Green;
    FastLED.show();
  #endif

  #ifdef RFID_SPI
    SPI.begin(SCLK, MISO, MOSI);
    rfid.PCD_Init();
    rfid.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  #endif

  #ifdef NEXTION
    Serial1.begin(9600,SERIAL_8N1, RXU1, TXU1);  //Nextion uart
  #endif

  #ifdef EASY_NEXTION  // For initial
    myNex.begin(9600,SERIAL_8N1,RXU1,TXU1);
  #endif

  #ifdef QRREADER
    Serial2.begin(9600,SERIAL_8N1, RXU2, TXU2);  // QR Reader uart
  #endif

  /*--------------------------------------- Setup Command ------------------------------------------*/

  digitalWrite(ENCOIN,LOW);
  digitalWrite(UNLOCK,LOW);

  #ifdef EASY_NEXTION  //For command
    myNex.writeStr("page 0");
    myNex.writeStr("p0.pic=13");
  #endif



  // -------------------- Initialize Configuraiton  --------------------
  // Step1: Get config from NV-RAM and save into config variable



  // -------------------- Finish Initialize Configuraiton --------------------


  
  // ------------------- WiFiMulti Add AccessPoint --------------------
  wifiMulti.addAP("myWiFi","1100110011");
  wifiMulti.addAP("Home173-AIS","1100110011");

  int wifitimeout=0;
  Serial.print("WiFi Connecting .");
  while ( (WiFi.status() != WL_CONNECTED) && (wifitimeout < 100)) {  
    wifiMulti.run();
    Serial.print(".");
    wifitimeout++;
    delay(200);
  }

  if(wifitimeout > 100){
    Serial.printf("WiFi connecting timeout....Restarting device. \n");  
    delay(1000);
    ESP.restart();
  }
  Serial.println();
  Serial.print("WiFi Connected with IP -> ");
  Serial.println(WiFi.localIP());
  // ------------------- Finish WiFiMulti --------------------

  // ------------------- Intitial Time Server ----------------
    Serial.printf("\nConnecting to TimeServer --> ");
  // cfginfo.asset.ntpServer1 = "1.th.pool.ntp.org";   // Comment on 1 Nov 65
  // cfginfo.asset.ntpServer2 = "asia.pool.ntp.org";   // Comment on 1 Nov 65
  configTime(7*3600,3600,"1.th.pool.ntp.org","asia.pool.ntp.org");

  struct tm timeNow;
  printLocalTime(&timeNow);

  // Serial.println(timeNow.tm_year);
  // Serial.println(timeNow.tm_mon);
  // Serial.println(timeNow.tm_mday);
  // Serial.println(timeNow.tm_hour);
  // Serial.println(timeNow.tm_min);
  // Serial.println(timeNow.tm_sec);


  
  // ------------------- Finish Intitial Time Server ----------------








  // -------------------- MQTT Initialize --------------------
  PubSubClient mqttclient(espclient);
  // -------------------- Finish MQTT Initialize --------------------

  Serial.println("Booting finish...System Ready");
  Serial.println();
  Serial.println();
  Serial.println("............... Console Monitoring ...............");
  #ifdef EASY_NEXTION  //For command
    myNex.writeStr("page 0");
    myNex.writeStr("p0.pic=10");

    myNex.writeStr("rtc0=" + String(1900+timeNow.tm_year));
    myNex.writeStr("rtc1=" + String(1+timeNow.tm_mon));
    myNex.writeStr("rtc2=" + String(timeNow.tm_mday));
    myNex.writeStr("rtc3=" + String(timeNow.tm_hour));
    myNex.writeStr("rtc4=" + String(timeNow.tm_min));
    myNex.writeStr("rtc5=" + String(timeNow.tm_sec));
  #endif
}

void loop() {

  //Update time

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

        showPageFlag = true;
        showSummary = true;
        tagId=0;
        userinfo.tagId="";
        userinfo.name="";
        userinfo.thName="";
        userinfo.credit=0;
        userinfo.point=0;

        break;
      case 1: // Page1
   
        if(topupPrice[0] > 0){
          size_t size = sizeof(topupPrice) / sizeof(topupPrice[0]);
          for(int i=0;i<size;i++){
            if(tagId == 0){
              myNex.writeStr("tsw b"+String(i+1)+",1");
              myNex.writeStr("b"+String(i+1)+".txt",String(topupPrice[i]));
              myNex.writeStr("b"+String(i+1)+".pco=27500");
              myNex.writeStr("b"+String(i+1)+".picc=1");
              myNex.writeStr("tsw b"+String(i+1)+",0");
            }else{
              myNex.writeStr("tsw b"+String(i+1)+",1");
              myNex.writeStr("b"+String(i+1)+".txt",String(topupPrice[i]));
              myNex.writeStr("b"+String(i+1)+".pco=0");
              myNex.writeStr("b"+String(i+1)+".picc=0");
            }
          } 
        }
        
        // Serial.println(WiFi.RSSI());
        if(tagId == 0){
          myNex.writeStr("tsw t4,1"); 
          myNex.writeStr("t4.picc=1");
          myNex.writeStr("tsw t4,0");          

          tagId = getTagInfo();

          if(tagId != 0){
            myNex.writeStr("tsw t4,1"); 
            myNex.writeStr("t4.picc=0");
             
            Serial.printf("New Tag: %u\n",tagId);
            // myNex.writeStr("t0.txt",String(tagId));

            String tag = String(tagId);
            Serial.println(tag);

            String response;
            int res = backend.getUserInfo(tag.c_str(),response);
            if(res == 200){
              Serial.printf("Response: %d\n",res);
              Serial.println(response);

              StaticJsonDocument<512> doc;
              DeserializationError error = deserializeJson(doc, response);
              if (error) {
                  Serial.print(F("deserializeJson() failed: "));
                  Serial.println(error.f_str());
                  
              }else{
                userinfo.tagId = doc["tagId"].as<String>();
                userinfo.name = doc["name"].as<String>();
                userinfo.thName = doc["thName"].as<String>();
                userinfo.credit = doc["credit"].as<int>();
                userinfo.point = doc["point"].as<int>();

                Serial.println(userinfo.name);
                
                myNex.writeStr("t0.txt",userinfo.tagId);
                myNex.writeStr("t1.txt",userinfo.name);
                myNex.writeStr("t2.txt",String(userinfo.credit));
                myNex.writeStr("t3.txt",String(userinfo.point));
              }
            }else{//Request error
              myNex.writeStr("popup.content.txt","Getting user info failed. Please contact staff");
              myNex.writeStr("page 9");
            }
          }
        }else{
          if((myNex.lastCurrentPageId == 2) && showPageFlag){
            Serial.printf("Old Tag: %u\n",tagId);
            myNex.writeStr("t1.txt",userinfo.tagId);
            myNex.writeStr("t2.txt",String(userinfo.credit));
            myNex.writeStr("t3.txt",String(userinfo.point));
            showPageFlag = false;
          }
        }
        
        // readRFID();
        break;
      case 2: // Topup page
        // Serial.printf("Active Page: %d\n",myNex.currentPageId);
        break;
      case 3: // Cash Page
        // Serial.printf("Active Page: %d\n",myNex.currentPageId);
        
        if(waitForPay == true){
          Serial.printf("TopupNow: %d\n",coinValue + billValue);
        }
        
  
        if((coinValue > 0) || (billValue > 0)){
          myNex.writeStr("t1.txt",String(coinValue + billValue)); 

          myNex.writeStr("tsw b1,0");
          myNex.writeStr("b1.picc=6");
          myNex.writeStr("tsw b2,0");
          myNex.writeStr("b2.picc=6");
          myNex.writeStr("tsw b3,0");
          myNex.writeStr("b3.picc=6");


          Serial.printf("TopupValue: %d\n",topupValue);
    
          if(topupValue == (coinValue + billValue)){
            myNex.writeStr("t1.txt",String(coinValue + billValue));
            waitForPay = false;
            digitalWrite(ENCOIN,LOW);
            coinCount = 0;
            coinValue = 0;
            billCount = 0;
            billValue = 0;

            Serial.println("Disable ENCOIN");
            delay(3000);
            showSummary = true;

            //Update database
            String response;
            int rescode = backend.updateCredit(userinfo.tagId.c_str(),userinfo.credit+topupValue,response);
            if(rescode == 200){
              delay(2000);
              // StaticJsonDocument<512> doc;
              // DeserializationError error = deserializeJson(doc, payload);
              // if (error) {
              //     Serial.print(F("deserializeJson() failed: "));
              //     Serial.println(error.f_str());
                  
              // }else{

              // }              

              myNex.writeStr("page 5");
            }else{
              //Faile to update.
              myNex.writeStr("popup.content.txt","Failed to update credit. Please contack staff");
              myNex.writeStr("page 9");
            }
          }
        }
        break;
      case 4: // QR Page
        #ifdef QRREADER
          if(qrPrice.isEmpty() && (topupValue != 0)){ //Request QR from backend API.
          }else{   // Waiting for scan qr
            if(qrSlip.isEmpty()){
              if(Serial2.available()){
                digitalWrite(BUZZ,HIGH);
                delay(100);
                digitalWrite(BUZZ,LOW);
                qrSlip = Serial2.readString();
                Serial.print("Serial2: ");
                Serial.println(qrSlip);
              }

              //Call api for slipCheck
            }
          }
        #endif
        break;
      case 5: // Summary Page
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
          myNex.writeStr("t0.txt",userinfo.tagId);
          myNex.writeStr("t1.txt",userinfo.name);
          myNex.writeStr("t2.txt",String(userinfo.credit));
          myNex.writeStr("t3.txt",String(userinfo.credit + topupValue));

          //Reset all variable
          // tagId=0;
          // userinfo.tagId="";
          // userinfo.name="";
          // userinfo.thName="";
          // userinfo.credit=0;
          // userinfo.point=0;

          // showPageFlag = true;
        }
        break;
      case 6: // Settings Page
        break;
    }
    myNex.NextionListen();
  #endif

}

void trigger0(){     //get topupValue
  Serial.println("Trigger0");
  myNex.writeStr("page 7");
  // topupValue = myNex.readStr("Topup.spv0.txt").toInt();
  // // topupValue = topupValueStr.toInt();

  // Serial.print("Topup Value: ");
  // Serial.println(topupValue);
}

void trigger1(){ // Return Button 
  Serial.println("Trigger1");
  int currentPage = myNex.currentPageId;
  Serial.printf("Trigger1->Page Now: %d\n",currentPage);
  switch(currentPage){
    case 0:
      break;
    case 1:
      Serial.println("Trigger1->Reset variable");
      tagId = 0;
      credit = 0;
      userinfo.tagId = "";
      userinfo.name = "";
      userinfo.thName = "";
      userinfo.credit = 0;
      userinfo.point = 0;

      Serial.println("Trigger1->change to page 0");
      myNex.writeStr("page 0");
      break;
    case 2:
      digitalWrite(ENCOIN,LOW);
      waitForPay = false;
      topupValue = 0;
      coinCount = 0;
      coinValue = 0;
      billCount = 0;
      billValue = 0;
      showPageFlag = true;
      myNex.writeStr("page 1");
      break;
    case 3:
      digitalWrite(ENCOIN,LOW);
      waitForPay = false;
      myNex.writeStr("page 2");
      if(topupPrice[0] > 0){
        size_t size = sizeof(topupPrice) / sizeof(topupPrice[0]);
        for(int i=0;i<size;i++){
          myNex.writeStr("b"+String(i+1)+".txt",String(topupPrice[i]));
        } 
      }
      break;
  }
}

void trigger2(){ //New RFID Button
  Serial.println("Trigger2");
  switch(myNex.currentPageId){
    case 1:
      tagId = 0;
      userinfo.tagId = "";
      userinfo.name = "";
      userinfo.thName = "";
      userinfo.credit = 0;
      userinfo.point = 0;
      break;
    case 2:
      tagId = 0;
      break;
  }

}

void trigger3(){ // OK btn  RFID Page
  Serial.println("Trigger3: OK btn RFID Page");

  if(!tagId){
    Serial.println("Page not change due to No userInfo");
  }else{
    myNex.writeStr("page 2");
  }

  if(topupPrice[0] > 0){
    size_t size = sizeof(topupPrice) / sizeof(topupPrice[0]);
    for(int i=0;i<size;i++){
        myNex.writeStr("b"+String(i+1)+".txt",String(topupPrice[i]));
      } 
  }
  // Get price and put over button
  // size_t size = sizeof(topupPrice) / sizeof(topupPrice[0]);
  // for(int i=0;i<size;i++){
  //   if(topupPrice[i] > 0){
  //     myNex.writeStr("b"+String(i+1)+".txt",String(topupPrice[i]));
  //   } 
  // }
}

void trigger4(){ // Price Select
  Serial.println("Trigger4: Price selection");
  
  // Serial.printf("Active Page: %d\n",myNex.currentPageId);
  // topupValue = myNex.readStr("t0.txt").toInt();
  // Serial.println(topupValue);

  // myNex.writeStr("page 3"); 
  // myNex.writeStr("t1.txt","0");
  
  digitalWrite(ENCOIN,HIGH);
  waitForPay = true;
  
}

void trigger5(){ // Cancel to Home
  Serial.println("Trigger5: Cancel button all page");
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
}

void trigger6(){ // QR Payment button page3
  String response;

  Serial.println("Trigger6: QR Payment");
  if(qrPrice.isEmpty() && (topupValue != 0)){
    int res = backend.qrgen("branchCode","assetCode",topupValue,response);
    if(res == 200){
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, response);

      qrPrice = doc["qrtext"].as<String>();
      myNex.writeStr("qr0.txt",qrPrice);
    }else{
      qrPrice = "";
      Serial.print("QR gen failed. please try again");
    }
  }
}

void trigger7(){
  Serial.println("Trigger7");
}

void trigger8(){
  Serial.println("Trigger8");
}

void trigger9(){
  Serial.println("Trigger9");
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
                    coinValue = coinCount * pricePerCoin;
                    // topupValue += coinValue;
                    Serial.printf("CoinValue: %d\n",coinValue);
                }  
                break;
            case BILLIN:
                if((gpio_get_level(io_num) == 0) && waitForPay){
                    billCount++;
                    billValue = billCount * pricePerBill;
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
void printDec(byte *buffer, byte bufferSize) {
  String card = "";
  for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
      Serial.print(buffer[i], DEC);
  }
}

 unsigned long hex2dec(byte *buffer, byte bufferSize){
  return (buffer[3] * 16777216) + (buffer[2] * 65536) + (buffer[1] * 256) + (buffer[0]);
 }