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
int maxTopupValue = 300;
int manualTopup = 0;

String branchCode = "10000-0001";
String assetCode = "962402E7E0";
String qrGenId = "";


boolean waitForPay = true;
boolean showSummary = false;
boolean showPageFlag = true;
boolean slipCheckFailed = false;
int activePage = -1;

int topupPrice[]={1,2,5,10,20};
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


#ifdef NEXTION
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
    // Serial2.begin(9600,SERIAL_8N1, RXU2, TXU2);  // QR Reader uart
    
  #endif

  /*--------------------------------------- Setup Command ------------------------------------------*/

  digitalWrite(ENCOIN,LOW);
  digitalWrite(UNLOCK,LOW);

  #ifdef EASY_NEXTION  //For command
    myNex.writeStr("page 0");
    myNex.writeStr("cover.msgTxt.txt","");
    myNex.writeStr("p0.pic=13");
    myNex.writeStr("vis title,0");
  #endif



  // -------------------- Initialize Configuraiton  --------------------
  // Step1: Get config from NV-RAM and save into config variable

  numBtn = sizeof(topupPrice) / sizeof(topupPrice[0]);

  // -------------------- Finish Initialize Configuraiton --------------------
  //cfgInfo

  
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
    //Move to cover page
    myNex.writeStr("page 0"); //Cover

    //Set Cover msgTxt
    myNex.writeStr("cover.msgTxt.txt","Please touch to start");
    myNex.writeStr("cover.msgTxt.pco=65535");
    myNex.writeStr("vis msgTxt,0");

    //Set 1st to show on cover page
    myNex.writeStr("p0.pic=10");
    myNex.writeStr("title.pic=32"); //"กรุณาสัมพัส"
    myNex.writeStr("vis title,1");

    //Set RTC on Nextion.
    myNex.writeStr("rtc0=" + String(1900+timeNow.tm_year));
    myNex.writeStr("rtc1=" + String(1+timeNow.tm_mon));
    myNex.writeStr("rtc2=" + String(timeNow.tm_mday));
    myNex.writeStr("rtc3=" + String(timeNow.tm_hour));
    myNex.writeStr("rtc4=" + String(timeNow.tm_min));
    myNex.writeStr("rtc5=" + String(timeNow.tm_sec));

    //set maxValue for numpad
    myNex.writeNum("numpad.maxVal.val",200);
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
        if(tagId == 0){
          //Enable timer triggger   if do nothing for 1 min then move to page 0
          myNex.writeStr("tm1.en=1");

          // set button = Disable for btn 1-5 
          for(int i=1;i<=numBtn;i++){
            myNex.writeStr("b"+String(i)+".pco=27500");
            myNex.writeStr("b" + String(i) + ".txt", String(topupPrice[i-1]));
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
      
            String response;
            int res = backend.getUserInfo(tag.c_str(),response);
            // Serial.print("Response status: ");
            // Serial.println(res);
            if(res == 200){
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

                Serial.printf("[page1]->getuserinfo: %s\n",userinfo.name);
                
                // myNex.writeStr("t1.txt",userinfo.name);
                myNex.writeStr("t1.txt",userinfo.tagId);
                myNex.writeStr("t2.txt",String(userinfo.credit));
                myNex.writeStr("t3.txt",String(userinfo.point));

                // set button = Disable for btn 1-5 
                for(int i=1;i<=numBtn;i++){
                  myNex.writeStr("b"+String(i)+".pco=0");   // Text Color = black
                  myNex.writeStr("b" + String(i) + ".txt", String(topupPrice[i-1]));
                  myNex.writeStr("b"+String(i)+".picc=0");  //Btn color = cyan
                  myNex.writeStr("tsw b"+String(i)+",1");   // Enable btn
                }
                // set enable for btn
                // myNex.writeStr("b6.txt","");
                myNex.writeStr("b6.picc=0");  
                myNex.writeStr("tsw b6,1"); 
              }
            }else{//Request error
              myNex.writeStr("popup.t0.txt","User with given card not found. Please try again.");
              myNex.writeStr("page 5");
              tagId=0;
            }
          }
        }else{
            
            if(showPageFlag){
              for(int i=1;i<=numBtn;i++){
                myNex.writeStr("b"+String(i)+".pco=0");   // Text Color = black
                myNex.writeStr("b" + String(i) + ".txt", String(topupPrice[i-1]));
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
              showPageFlag = false;
            }
        }     



        // end readRFID();
        break;
      case 2: // Cash page
        if(waitForPay == true){
          Serial.printf("TopupNow: %d\n",coinValue + billValue);
        }
        
        if((coinValue > 0) || (billValue > 0)){
          myNex.writeStr("t2.txt",String(coinValue + billValue)); 

          myNex.writeStr("tsw b1,0");
          myNex.writeStr("b1.picc=4");
          myNex.writeStr("tsw b2,0");
          myNex.writeStr("b2.picc=4");
          myNex.writeStr("tsw b3,0");
          myNex.writeStr("b3.picc=4");


          Serial.printf("TopupValue: %d\n",topupValue);
    
          if(topupValue == (coinValue + billValue)){
            myNex.writeStr("t2.txt",String(coinValue + billValue));
            waitForPay = false;
            digitalWrite(ENCOIN,LOW);
            Serial.println("Disable ENCOIN");

            coinCount = 0;
            coinValue = 0;
            billCount = 0;
            billValue = 0;

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

              myNex.writeStr("page 4");
            }else{
              //Faile to update.
              myNex.writeStr("popup.content.txt","Failed to update credit. Please contack staff");
              myNex.writeStr("page 5");
            }
          }
        }        
        break;
      case 3: // QR Page
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
                String response;
                int rescode = backend.slipCheck(branchCode.c_str(),assetCode.c_str(),qrSlip.c_str(),qrGenId.c_str(),response);
                Serial.printf("[Page3 slipCheck]->rescode: %d\n",rescode);

                //End serial2 port
                Serial2.end();

                if(rescode == 200){
                  //Display correct icon.
                  myNex.writeStr("p2.pic=16");
                  myNex.writeStr("vis p2,1");
          
                  //UpdateCredit here.  check by qrgenId
                  rescode = 0;
                  rescode = backend.updateCredit(userinfo.tagId.c_str(),userinfo.credit+topupValue,response);
                  if(rescode == 200){
                    delay(2000);
                    myNex.writeStr("page 4");
                  }else{
                    //Faile to update.
                    myNex.writeStr("popup.content.txt","Failed to update credit. Please contack staff");
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

          
        }
        break;
      case 6: // numPad Page
        // if(topupValue > 0){
        //   myNex.writeStr("show.txt",String(topupValue));
        // }
        break;
    }
    myNex.NextionListen();
  #endif

}

void trigger0(){     //get topupValue
  Serial.println("Trigger0: Setting Btn many page");
  myNex.writeStr("page 7");
  // topupValue = myNex.readStr("Topup.spv0.txt").toInt();
  // // topupValue = topupValueStr.toInt();

  // Serial.print("Topup Value: ");
  // Serial.println(topupValue);
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

  //Check over maxTopupValue
  if(topupValue < maxTopupValue){
    digitalWrite(ENCOIN,HIGH);
    waitForPay = true;
  }else{
    myNex.writeStr("Topup value more than " + String(maxTopupValue));
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
    myNex.writeStr("b" + String(i) + ".txt", String(topupPrice[i-1]));
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
  String response;
  Serial.println("Trigger6: QR Payment on cashPage");

  digitalWrite(ENCOIN,LOW);
  waitForPay = false;

  Serial2.begin(9600,SERIAL_8N1, RXU2, TXU2);
  
  if(qrPrice.isEmpty() && (topupValue > 0)){
    Serial.println("Request QR Gen API Please wait.");
    int res = backend.qrgen(branchCode.c_str(),assetCode.c_str(),topupValue,response);
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
    myNex.writeStr("b" + String(i) + ".txt", String(topupPrice[i-1]));
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