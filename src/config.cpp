#include "config.h"
#include "kioskControl.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>




void initGPIO(unsigned long long INP, unsigned long long OUTP){
  gpio_config_t io_config;

  Serial.printf("  Execute---Initial GPIO Function\n");
  //*** Initial INTERRUPT PIN
//   io_config.intr_type = GPIO_INTR_NEGEDGE;
//   io_config.pin_bit_mask = INTR;
//   io_config.mode = GPIO_MODE_INPUT;
//   io_config.pull_up_en = GPIO_PULLUP_ENABLE;
//   gpio_config(&io_config);
  
  //*** Initial INPUT PIN
  io_config.pin_bit_mask = INP;
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_config);


  //*** Initial INPUT & OUTPUT PIN
  io_config.pin_bit_mask = OUTP;
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT_OUTPUT;
  gpio_config(&io_config);
}

void initCFG(CONFIG &cfg){
  Serial.println("[initCFG]->Start Initialize configuration");

  cfg.header="ALVATO";

  cfg.wifi[0].ssid = "myWiFi";
  cfg.wifi[0].key = "1100110011";

  cfg.maxTopup = 500;
  cfg.pricePerBill = 10;
  cfg.pricePerCoin = 1;
  cfg.topupPrice[0]=10;
  cfg.topupPrice[1]=15;
  cfg.topupPrice[2]=20;
  cfg.topupPrice[3]=25;
  cfg.topupPrice[4]=30;

  cfg.cashEnable = true;
  cfg.qrEnable = true;

  //Asset Configuration
  cfg.asset.merchantCode="10000";
  cfg.asset.branchCode="10000-0001";
  cfg.asset.assetCode="0123456789";
  cfg.asset.assetName="DEMO";
  cfg.asset.assetType="KIOSK";
  cfg.asset.assetMac="";
  cfg.asset.firmware="1.0.0";
  cfg.asset.userAdmin="admin";
  cfg.asset.userPass="4556"; //13897
  cfg.asset.ntp1="1.th.pool.ntp.org";
  cfg.asset.ntp2="asia.pool.ntp.org";

  
  // MQTT Configuration
  cfg.mqtt[0].enable = true;
  cfg.mqtt[0].mqtthost = "www.flipup.net",
  cfg.mqtt[0].mqttport = "1883";
  cfg.mqtt[0].mqttuser = "sammy";
  cfg.mqtt[0].mqttpass = "password";
  cfg.mqtt[0].topicPub = "alvato/backend/";
  cfg.mqtt[0].topicSub = "alvato/";

  cfg.mqtt[1].enable = !cfg.mqtt[0].enable;
  cfg.mqtt[1].mqtthost = "glad-whale.rmq.cloudamqp.com";
  cfg.mqtt[1].mqttport = "1883";
  cfg.mqtt[1].mqttuser = "ilsrjeyw";
  cfg.mqtt[1].mqttpass = "1riEi7_wHGpUA7r-oF76FF4ay81diLJr";
  cfg.mqtt[1].topicPub = "alvato/backend/";
  cfg.mqtt[1].topicSub = "alvato/";


  // API Configuration

  // change at Lumpinee with Tanod 24 Dec
  // cfg.api.apihost="https://alvato-wallet-5a36860a4b3d.herokuapp.com";


  cfg.api.apihost="http://192.168.146.34";
  cfg.api.apihostqr="https://alvato-wallet-5a36860a4b3d.herokuapp.com";
  cfg.api.apikey="";
  cfg.api.apisecret="";
  cfg.api.getuserinfo="/cashlesswww/S21checkuserkiosk.php";
  cfg.api.updatecredit="/cashlesswww/S22topupkiosk.php";
  cfg.api.heartbeat="/cashlesswww/S8heartbeat.php";
  cfg.api.newcointrans = "/api/transaction/v1.0.0/newCoinTrans";
  cfg.api.assetregister = "/api/asset/v1.0.0/newAsset";

  // change at Lumpinee with Tanod 24 Dec
  // cfg.api.getuserinfo="/api/cashlesswww/S21checkuserkiosk";
  // cfg.api.updatecredit="/api/cashlesswww/S22topupkiosk";
  // cfg.api.heartbeat="/api/cashlesswww/S8heartbeat";
  // cfg.api.newcointrans = "/api/transaction/v1.0.0/newCoinTrans";
  // cfg.api.assetregister = "/api/asset/v1.0.0/newAsset";

  // cfg.api.getuserinfo="/api/userinfo/v1.0.0/getByTag";
  // cfg.api.updatecredit="/api/userinfo/v1.0.0/updateCredit";
  // cfg.api.heartbeat="/api/cashlesswww/S8heartbeat";

  cfg.api.updatepoint="/api/userinfo/v1.0.0/updatePoint";
  cfg.api.qrgen="/api/maemanee/v1.0.0/qrgen";
  cfg.api.slipcheck="/api/easySlip/v1.0.0/slipCheckQrID";

  Serial.println("[initCFG]->Finished initialize configuration"); 
}


void showCFG(CONFIG &cfg){
  Serial.println("[showCFG]-> Start Display configuration"); 

  Serial.printf("[showCFG]-> Header : %s\n", cfg.header.c_str());
  Serial.printf("[showCFG]-> Device ID: %s\n",cfg.deviceId.c_str());



  Serial.println("[showCFG]->Finished Display configuration"); 
}

void getNVCFG(Preferences nvcfg, CONFIG &cfg){
  Serial.println("[getNVCFG]->Start initialize configuration"); 
  // nvcfg.begin("config",false);    //Open config
    
  // nvcfg.end();
  Serial.println("[getNVCFG]->Finished initialize configuration"); 
}


void printLocalTime(tm * timeinfo){
  char buff[30];

  // struct tm timeinfo;
  if (!getLocalTime(timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println(timeinfo, "%A, %B %d %Y %H:%M:%S");
}


void wifiCFG(Preferences nvcfg, CONFIG &cfg){
    Serial.println("\r\nPlease open ESPTouch on your mobile to setup WiFi");
    WiFi.beginSmartConfig();
    //Wait for SmartConfig packet from mobile
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("SmartConfig received.");

    //Wait for WiFi to connect to AP
    Serial.println("Waiting for WiFi Connection");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    nvcfg.begin("config",false);
      if(!nvcfg.isKey("wifi1.ssid") || (nvcfg.isKey("wifi2.ssid"))){  // not found wifi2  save wifi2
        cfg.wifi[0].ssid = WiFi.SSID();
        cfg.wifi[0].key = WiFi.psk();
        nvcfg.putString("wifi1.ssid",cfg.wifi[0].ssid);
        nvcfg.putString("wifi1.key",cfg.wifi[0].key);
        Serial.print("   |- WiFi-1 -> SSID: "); Serial.print(cfg.wifi[0].ssid);
        Serial.print(" Pass: "); Serial.println(cfg.wifi[0].key);
      }else if(!nvcfg.isKey("wifi2.ssid")){  // found wifi2  then overwrite wifi1
        cfg.wifi[1].ssid = WiFi.SSID();
        cfg.wifi[1].key = WiFi.psk();
        nvcfg.putString("wifi2.ssid",cfg.wifi[1].ssid);
        nvcfg.putString("wifi2.key",cfg.wifi[1].key);
        Serial.print("   |- WiFi-1 -> SSID: "); Serial.print(cfg.wifi[0].ssid);
        Serial.print(" Pass: "); Serial.println(cfg.wifi[0].key);
      }
    nvcfg.end();
    // ESP.restart();
}


void getTimeWithFormat(char *_buff,tm *_tm){
    getLocalTime(_tm);
    strftime (_buff,40,"%d-%b-%G %T",_tm);
}