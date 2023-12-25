#include <Arduino.h>
#include <Preferences.h>
// #include "config.h"

class alvato{
    private:
    public:
        // String uri_alvato = "https://student-wallet.vercel.app"; // Vercel
        // String uri_alvato = "https://alvato-wallet-5a36860a4b3d.herokuapp.com"; // on Heroku

        String api_apiHost = "";
        String api_apiHostQR = "";
        String api_apiKey = "";
        String api_apiSecret = "";
        String api_getUserInfo="";                  //"/api/userinfo/getByTag";
        String api_updateCredit="";
        String api_heartbeat="";
        String api_updatePoint="";
        String api_slipCheck = "";
        String api_qrgen = "";
        String api_newCoinTrans ="";
        String api_register = "";


        // int assetRegister();
        // int getAssetCFG();

        int topupKiosk(const char* tagId, int value, const char* asset,  String &payload);
        int checkUser(const char* tagId,String &payload);
        int heartbeat(const char* station, const char* ip, String &payload);
        int newCoinTrans(const char *branch, const char *asset,const char* tagId, int value, String &payload);
        int getUserInfo(const char* tagId,String &payload);
        int updateCredit(const char* tagId, int value, const char* asset, String &payload);
        int updatePoint(const char* tagId,int value, String &payload);
        int qrgen(const char *branchCode, const char *assetCode, int value, String &payload);
        int slipCheck(const char *branchCode, const char *assetCode,const char* tagId,const char *qrtxt, const char *qrgenID,String &payload);
        // int heartBeat(const char *branchCode, const char *assetCode);
        int assetRegister(const char *branchCode, const char *macAddr, const char *firmware, String &payload);
        
        
        void setVar(const char *varName, const char *value);
        String getVar(const char *varName);
   
};