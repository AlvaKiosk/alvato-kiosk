#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "alvato.h"

int alvato::getUserInfo(const char* tagId,String &payload){
    HTTPClient http;
    
    String url = this->uri_alvato + "/api/userinfo/v1.0.0/getByTag" + "?tagId=" +tagId;

    //Tanod API S21checkuserkios
    // String url = "http://192.168.105.34/cashlesswww/S21checkuserkiosk.php?tagId="  +String(tagId);

    // Serial.println(url);

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

int alvato::updateCredit(const char* tagId, int value, String &payload){
    HTTPClient http;
    String reqbody;
    String url = this->uri_alvato + "/api/userinfo/v1.0.0/updateCredit";
    StaticJsonDocument<100> doc;

    doc["tagId"] = tagId;
    doc["credit"] = value;
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

int alvato::updatePoint(const char* tagId, int value, String &payload){
    HTTPClient http;
    String reqbody;
    String url = uri_alvato + "/api/userinfo/v1.0.0/updatePoint";
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

int alvato::qrgen(const char* branchCode, const char* assetCode, int value, String &payload){
    HTTPClient http;
    String reqbody;
    String url = uri_alvato + "/api/maemanee/v1.0.0/qrgen";
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

int alvato::slipCheck(const char* branchCode, const char* assetCode, const char* qrtxt, const char* qrgenID,String &payload){
    HTTPClient http;
    String reqbody;
    String url = uri_alvato + "/api/easySlip/v1.0.0/slipCheckQrID";
    StaticJsonDocument<512> doc;

    doc["branchCode"] = branchCode;
    doc["assetCode"] = assetCode;
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
