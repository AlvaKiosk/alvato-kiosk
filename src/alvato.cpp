#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "alvato.h"
// #include "config.h"


int alvato::topupKiosk(const char* tagId, int value, const char* asset, String &payload){
    HTTPClient http;
    String reqbody;

    String url = api_apiHost + api_updateCredit;
    // String url = this->uri_alvato + "/api/userinfo/v1.0.0/updateCredit";


    StaticJsonDocument<100> doc;

    doc["card"] = tagId;
    doc["value"] = value;
    doc["assetName"]=asset;
    serializeJson(doc,reqbody);
    Serial.println(reqbody);
    
    
    Serial.println(url);
    http.begin(url);
    int rescode = http.PUT(reqbody);
    payload = http.getString();
    if(rescode == 200){
        Serial.println("updateCredit success"); 
    }else{
        Serial.println("updateCredit failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;
}
int alvato::checkUser(const char* tagId,String &payload){
    HTTPClient http;
    
    String url = api_apiHost + api_getUserInfo + "?tagId=" +tagId;
    Serial.print("URL: ");
    Serial.println(url);

    http.begin(url);
    int rescode = http.GET();
    payload = http.getString();
    if(rescode == 200){
        Serial.println("[getByTag]-> Get userinfo by tag sucessful");
    }else{
        Serial.println("[getByTag]-> Get userInfo by tag failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;
}
int alvato::heartbeat(const char* station, const char* ip, String &payload){
    HTTPClient http;
    
    String url = api_apiHost + api_heartbeat + "?station=" + station + "&ip=" + ip;

    http.begin(url);

    int rescode = http.GET();
    payload = http.getString();
    if(rescode == 200){
        Serial.println("[heartbeat]-> Send update sucessful");
    }else{
        Serial.println("[heartbeat]-> Send update failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;
}

int alvato::newCoinTrans(const char *branch, const char *asset,const char* tagId, int value, String &payload){
    HTTPClient http;
    String reqbody;

    String url = api_apiHost + api_newCoinTrans;
    // String url = this->uri_alvato + "/api/userinfo/v1.0.0/updateCredit";


    StaticJsonDocument<100> doc;


    doc["assetName"] = asset;
    doc["assetCode"] = asset;
    doc["branchCode"] = branch;
    doc["tagId"] = tagId;
    doc["credit"] = String(value);
    doc["amount"] = String(value);

    serializeJson(doc,reqbody);
    Serial.println(reqbody);
    
    Serial.println(url);
    http.begin(url);
    int rescode = http.POST(reqbody);
    payload = http.getString();
    if(rescode == 200){
        Serial.println("Transaction created success"); 
    }else{
        Serial.println("Transaction create failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;
}

int alvato::getUserInfo(const char* tagId,String &payload){
    HTTPClient http;
    
    String url = api_apiHost + api_getUserInfo + "?tagId=" +tagId;

    // String url = this->uri_alvato + "/api/userinfo/v1.0.0/getByTag" + "?tagId=" +tagId;

    //Tanod API S21checkuserkios
    // String url = "http://192.168.105.34/cashlesswww/S21checkuserkiosk.php?tagId="  +String(tagId);

    // Serial.println(url);
    Serial.print("URL: ");
    Serial.println(url);

    http.begin(url);
    int rescode = http.GET();
    payload = http.getString();
    Serial.print("[getByTag] -> payload: ");
    Serial.println(payload);

    if(rescode == 200){
        Serial.println("[getByTag]-> Get userinfo by tag sucessful");
    }else{
        Serial.println("[getByTag]-> Get userInfo by tag failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;
}

int alvato::updateCredit(const char* tagId, int value, const char* asset,  String &payload){
    HTTPClient http;
    String reqbody;

    String url = api_apiHost + api_updateCredit;
    // String url = this->uri_alvato + "/api/userinfo/v1.0.0/updateCredit";


    StaticJsonDocument<100> doc;

    doc["tagId"] = tagId;
    doc["credit"] = value;

    doc["card"] = tagId;
    doc["value"] = value;
    doc["assetName"] = asset;

    serializeJson(doc,reqbody);

    Serial.println(url);
    Serial.println(reqbody);
    
   
    http.begin(url);
    int rescode = http.PUT(reqbody);
    payload = http.getString();
    if(rescode == 200){
        Serial.println("updateCredit success"); 
    }else{
        Serial.println("updateCredit failed");
        Serial.println(rescode);
        Serial.println(payload);
    }
    http.end();
    Serial.println(payload);
    return rescode;
}

int alvato::updatePoint(const char* tagId, int value, String &payload){
    HTTPClient http;
    String reqbody;
    
    String url = api_apiHost + api_updatePoint;
    // String url = uri_alvato + "/api/userinfo/v1.0.0/updatePoint";

    StaticJsonDocument<100> doc;

    doc["tagId"] = tagId;
    doc["point"] = value;
    serializeJson(doc,reqbody);

    http.begin(url);
    int rescode = http.PUT(reqbody);
    payload = http.getString();
    if(rescode == 200){
        Serial.println("updatePoint success"); 
    }else{
        Serial.println("updatePoint failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;
}

int alvato::qrgen(const char *branchCode, const char *assetCode, int value, String &payload){
    HTTPClient http;
    String reqbody;


    // String url = api_apiHost + api_qrgen;    // without tanod api
    String url = api_apiHostQR + api_qrgen;

    



    StaticJsonDocument<100> doc;

    doc["branchCode"] = branchCode;
    doc["assetCode"] = assetCode;
    doc["amount"]= String(value);
    serializeJson(doc,reqbody);

    Serial.print("req payload");
    Serial.println(reqbody);

    http.begin(url);
    int rescode = http.POST(reqbody);
    payload = http.getString();
    if(rescode == 200){
        // payload = http.getString();
        Serial.println("qrGen success");    
    }else{
        Serial.println("qrGen failed"); 
    }
    http.end();
    Serial.println(payload);
    return rescode;
}

int alvato::slipCheck(const char *branchCode, const char *assetCode, const char* tagId, const char* qrtxt, const char* qrgenID,String &payload){
    HTTPClient http;
    String reqbody;
    // String url = api_apiHost + api_slipCheck;
    String url = api_apiHostQR + api_slipCheck;
   

    StaticJsonDocument<512> doc;

    doc["branchCode"] = branchCode;
    doc["assetCode"] = assetCode;
    doc["tagId"] = tagId;
    doc["qrText"]= qrtxt;
    doc["qrgenID"]=qrgenID;
    serializeJson(doc,reqbody);
    Serial.print("[slipCheck]->payload");
    Serial.println(reqbody);

    http.begin(url);
    int rescode = http.POST(reqbody);
    Serial.printf("[slipCheck->rescode]: %d\n",rescode);
    payload = http.getString();
    if(rescode == 200){
        Serial.println("slipCheck success"); 
    }else{
        Serial.println("slipCheck failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;    
}



int alvato::assetRegister(const char *branchCode, const char *macAddr, const char *firmware, String &payload){
    HTTPClient http;
    String reqbody;
    String url = api_apiHostQR + api_register;
    Serial.print("url: ");
    Serial.println(url);
    // String url = uri_alvato + "/api/easySlip/v1.0.0/slipCheckQrID";

    StaticJsonDocument<512> doc;

    doc["branchCode"] = branchCode;
    doc["deviceMac"] = macAddr;
    doc["firmware"] = firmware;
    // doc["assetName"] = assetName;
 
    serializeJson(doc,reqbody);
    Serial.print("[assetRegister]->payload: ");
    Serial.println(reqbody);

    http.begin(url);
    int rescode = http.POST(reqbody);
    payload = http.getString();

    Serial.printf("[assetRegister]->rescode: %d\n",rescode);
    Serial.printf("[assetRegister]->payload: %s\n",payload.c_str());
    
    if(rescode == 200){
        Serial.println("assetRegister success"); 
    }else{
        Serial.println("assetRegister failed");
    }
    http.end();
    Serial.println(payload);
    return rescode;    
}
void alvato::setVar(const char *varName, const char *value){
    String tmpStr = String(varName);
    tmpStr.toLowerCase();

    Serial.printf("  |- setVar -> %s: %s\n",tmpStr.c_str(),value);
    
    if(strcmp("apihost",tmpStr.c_str()) == 0){
        api_apiHost = value;
    }else if(strcmp("apihostqr",tmpStr.c_str()) ==0 ){
        api_apiHostQR = value;
    }else if(strcmp("apikey",tmpStr.c_str()) ==0 ){
        api_apiKey = value;
    }else if(strcmp("apisecret",tmpStr.c_str()) ==0 ){
        api_apiSecret = value;    
    }else if(strcmp("getuserinfo",tmpStr.c_str()) == 0){
        api_getUserInfo = value;
    }else if(strcmp("updatecredit",tmpStr.c_str()) == 0){
        api_updateCredit = value;
    }else if(strcmp("updatepoint",tmpStr.c_str()) == 0){
        api_updatePoint = value;
    }else if(strcmp("heartbeat",tmpStr.c_str()) == 0){
        api_heartbeat = value;
    }else if(strcmp("slipcheck",tmpStr.c_str()) == 0){
        api_slipCheck = value;
    }else if(strcmp("qrgen",tmpStr.c_str()) == 0){
        api_qrgen = value;
    }else if(strcmp("newcointrans",tmpStr.c_str()) == 0){
        api_newCoinTrans = value;
    }else if(strcmp("assetregister",tmpStr.c_str()) == 0){
        api_register = value;
    }
}

String alvato::getVar(const char *varName){
    String tmpStr = String(varName);
    tmpStr.toLowerCase();

    if(strcmp("apihost",tmpStr.c_str()) == 0){
        return api_apiHost.c_str();
    }else if(strcmp("apihostqr",tmpStr.c_str()) ==0 ){
        return api_apiHostQR.c_str();
    }else if(strcmp("apikey",tmpStr.c_str()) ==0 ){
        return api_apiKey.c_str();
    }else if(strcmp("apisecret",tmpStr.c_str()) ==0 ){
        return api_apiSecret.c_str();    
    }else if(strcmp("getuserinfo",tmpStr.c_str()) == 0){
        return api_getUserInfo.c_str();
    }else if(strcmp("updatecredit",tmpStr.c_str()) == 0){
        return api_updateCredit.c_str();
    }else if(strcmp("updatepoint",tmpStr.c_str()) == 0){
        return api_updatePoint.c_str();
    }else if(strcmp("heartbeat",tmpStr.c_str()) == 0){
        return api_heartbeat.c_str();
    }else if(strcmp("slipcheck",tmpStr.c_str()) == 0){
        return api_slipCheck.c_str();
    }else if(strcmp("qrgen",tmpStr.c_str()) == 0){
        return api_qrgen.c_str();
    }else if(strcmp("newcointrans",tmpStr.c_str()) == 0){
        return api_newCoinTrans.c_str();
    }else if(strcmp("assetegister",tmpStr.c_str()) == 0){
        return api_register.c_str();
    }else{
        return "not found";
    }               
}
