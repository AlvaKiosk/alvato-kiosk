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
    struct SSID{
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
        String assetCode;
        String assetName;
        String assetMac;
    };

    struct MQTTCFG{
        String mqtthost;
        String mqttport;
        String mqttuser;
        String mqttpass;
    };

    struct APIHOST{
        String  apihost;
        String  apikey;
        String  apisecret;
    };

    struct CONFIG{
        String header;  //To verify it is asset of company
        ASSET asset;
        APIHOST apihost[2];
        MQTTCFG mqttcfg[2];
        SSID wifissid[2];
        int pricePerCoin = 0;
        int pricePerBill = 0;
        int topupPrice[6];
        int maxTopup = 0;
    };




    // void IRAM_ATTR gpio_isr_handler(void* arg);
    // void gpio_task(void *arg);
    // void init_interrupt(void);
    void initGPIO(unsigned long long INP, unsigned long long OUTP);

    void printLocalTime(tm * timeinfo);

    //Function Load config from NV-RAM
    

//----------------------------------- Global Variable define here -----------------------------------


// int pricePerCoin = 0;
// int pricePerBill = 0;


//---------------------------------------------------------------------------------------------------




#endif

