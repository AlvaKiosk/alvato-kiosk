#define CODENAME "alvatokiosk"
#define FIRMWARE "1.0.0"

#ifndef config_h
    #define config_h

    #include <Arduino.h>
    #include <Preferences.h>

    #define QRREADER 
    #define RGBLED
    #define RFID_SPI
    // #define NEXTION
    // #define CASH
    #define EASY_NEXTION


    // --------------- Structure ---------------
    struct WIFI{
        String ssid;
        String key;
    };

    struct USERINFO{
        String tagId;
        String name;
        String thName;
        int    credit;
        int    point;
    };

    struct ASSET{
        String merchantCode;
        String branchCode;
        String assetCode;
        String assetName;
        String assetType;
        String assetMac;
        String firmware;
        String ntp1;
        String ntp2;
        String userAdmin;
        String userPass;
    };

    struct MQTTCFG{
        bool enable = false;
        String mqtthost;
        String mqttport;
        String mqttuser;
        String mqttpass;
        String topicPub;
        String topicSub;
    };

    struct APISERVICE{
        String  apihost;
        String  apikey;
        String  apisecret;
        String  apihostqr;
        String  assetregister;
        String  getuserinfo;
        String  updatecredit;
        String  heartbeat;
        String  updatepoint;
        String  qrgen;
        String  slipcheck;
        String  newcointrans;
    };


    struct CONFIG{
        String header;  //To verify it is asset of company
        String manifest_url; // url for ota.json
        String deviceId;
        ASSET asset;
        MQTTCFG mqtt[2];
        WIFI wifi[2];
        APISERVICE api;
        bool cashEnable;
        bool qrEnable;
        int pricePerCoin;
        int pricePerBill;
        int topupPrice[5];
        int maxTopup;
    };




 


    // void IRAM_ATTR gpio_isr_handler(void* arg);
    // void gpio_task(void *arg);
    // void init_interrupt(void);
    void initGPIO(unsigned long long INP, unsigned long long OUTP);

    void initCFG(CONFIG &cfg);
    void showCFG(CONFIG &cfg);
    void getNVCFG(Preferences nvcfg, CONFIG &cfg);

    void printLocalTime(tm * timeinfo);
    void getTimeWithFormat(char *_buff,tm *_tm);

    void wifiCFG(Preferences nvcfg, CONFIG &cfg);
    //Function Load config from NV-RAM
    

//----------------------------------- Global Variable define here -----------------------------------


// int pricePerCoin = 0;
// int pricePerBill = 0;


//---------------------------------------------------------------------------------------------------




#endif

