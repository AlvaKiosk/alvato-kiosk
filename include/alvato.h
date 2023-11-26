#include <Arduino.h>
#include <Preferences.h>

class alvato{
    private:
    public:
        // String uri_alvato = "https://student-wallet.vercel.app"; // Vercel
        String uri_alvato = "https://alvato-wallet-5a36860a4b3d.herokuapp.com";
        String uri_getUserInfo="/api/userinfo/getByTag";
        String uri_updateCredit="";
        String uri_heartbeat="";


        int getUserInfo(const char* tagId,String &payload);
        int updateCredit(const char* tagId, int value, String &payload);
        int updatePoint(const char* tagId,int value, String &payload);

        int qrgen(const char* branchCode, const char* assetCode, int value, String &payload);
        int slipCheck(const char* branchCode, const char* assetCode, const char* qrtxt, const char* qrgenID,String &payload);
};