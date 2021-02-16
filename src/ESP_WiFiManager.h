/****************************************************************************************************************************
  ESP_WiFiManager.h
  For ESP8266 / ESP32 boards

  ESP_WiFiManager is a library for the ESP8266/Arduino platform
  (https://github.com/esp8266/Arduino) to enable easy
  configuration and reconfiguration of WiFi credentials using a Captive Portal
  inspired by:
  http://www.esp8266.com/viewtopic.php?f=29&t=2520
  https://github.com/chriscook8/esp-arduino-apboot
  https://github.com/esp8266/Arduino/blob/master/libraries/DNSServer/examples/CaptivePortalAdvanced/

  Modified from Tzapu https://github.com/tzapu/WiFiManager
  and from Ken Taylor https://github.com/kentaylor

  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager
  Licensed under MIT license
  Version: 1.4.3

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      07/10/2019 Initial coding
  1.0.1   K Hoang      13/12/2019 Fix bug. Add features. Add support for ESP32
  1.0.2   K Hoang      19/12/2019 Fix bug thatkeeps ConfigPortal in endless loop if Portal/Router SSID or Password is NULL.
  1.0.3   K Hoang      05/01/2020 Option not displaying AvailablePages in Info page. Enhance README.md. Modify examples
  1.0.4   K Hoang      07/01/2020 Add RFC952 setHostname feature.
  1.0.5   K Hoang      15/01/2020 Add configurable DNS feature. Thanks to @Amorphous of https://community.blynk.cc
  1.0.6   K Hoang      03/02/2020 Add support for ArduinoJson version 6.0.0+ ( tested with v6.14.1 )
  1.0.7   K Hoang      13/04/2020 Reduce start time, fix SPIFFS bug in examples, update README.md
  1.0.8   K Hoang      10/06/2020 Fix STAstaticIP issue. Restructure code. Add LittleFS support for ESP8266 core 2.7.1+
  1.0.9   K Hoang      29/07/2020 Fix ESP32 STAstaticIP bug. Permit changing from DHCP <-> static IP using Config Portal.
                                  Add, enhance examples (fix MDNS for ESP32)
  1.0.10  K Hoang      08/08/2020 Add more features to Config Portal. Use random WiFi AP channel to avoid conflict.
  1.0.11  K Hoang      17/08/2020 Add CORS feature. Fix bug in softAP, autoConnect, resetSettings.
  1.1.0   K Hoang      28/08/2020 Add MultiWiFi feature to autoconnect to best WiFi at runtime
  1.1.1   K Hoang      30/08/2020 Add setCORSHeader function to allow flexible CORS. Fix typo and minor improvement.
  1.1.2   K Hoang      17/08/2020 Fix bug. Add example.
  1.2.0   K Hoang      09/10/2020 Restore cpp code besides Impl.h code to use if linker error. Fix bug.
  1.3.0   K Hoang      04/12/2020 Add LittleFS support to ESP32 using LITTLEFS Library
  1.4.1   K Hoang      22/12/2020 Fix staticIP not saved. Add functions. Add complex examples. Sync with ESPAsync_WiFiManager
  1.4.2   K Hoang      14/01/2021 Fix examples' bug not using saved WiFi Credentials after losing all WiFi connections.
  1.4.3   K Hoang      23/01/2021 Fix examples' bug not saving Static IP in certain cases.
 *****************************************************************************************************************************/

#pragma once

#define ESP_WIFIMANAGER_VERSION     "ESP_WiFiManager v1.4.3"

#include "ESP_WiFiManager_Debug.h"

//KH, for ESP32
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else		//ESP32
  #include <WiFi.h>
  #include <WebServer.h>
#endif

#include <DNSServer.h>
#include <memory>
#undef min
#undef max
#include <algorithm>

//KH, for ESP32
#ifdef ESP8266
  extern "C"
  {
    #include "user_interface.h"
  }
  
  #define ESP_getChipId()   (ESP.getChipId())
#else		//ESP32
  #include <esp_wifi.h>
  #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#endif

// New in v1.4.0
typedef struct
{
  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;

}  WiFi_AP_IPConfig;

// Thanks to @Amorphous for the feature and code
// (https://community.blynk.cc/t/esp-wifimanager-for-esp32-and-esp8266/42257/13)
// To enable to configure from sketch
#if !defined(USE_CONFIGURABLE_DNS)
  #define USE_CONFIGURABLE_DNS        false
#endif

typedef struct
{
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;
  IPAddress _sta_static_dns1;
  IPAddress _sta_static_dns2;
}  WiFi_STA_IPConfig;
//////

#define WFM_LABEL_BEFORE 1
#define WFM_LABEL_AFTER 2
#define WFM_NO_LABEL 0

/** Handle CORS in pages */
// Default false for using only whenever necessary to avoid security issue when using CORS (Cross-Origin Resource Sharing)
#ifndef USING_CORS_FEATURE
  // Contributed by AlesSt (https://github.com/AlesSt) to solve AJAX CORS protection problem of API redirects on client side
  // See more in https://github.com/khoih-prog/ESP_WiFiManager/issues/27 and https://en.wikipedia.org/wiki/Cross-origin_resource_sharing
  #define USING_CORS_FEATURE     false
#endif

//KH
//Mofidy HTTP_HEAD to WM_HTTP_HEAD_START to avoid conflict in Arduino esp8266 core 2.6.0+
const char WM_HTTP_200[] PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char WM_HTTP_HEAD_START[] PROGMEM = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /><title>{v}</title>";

// KH, update from v1.0.10
const char WM_HTTP_STYLE[] PROGMEM = "<style>div{padding:2px;font-size:1em;}body,textarea,input,select{background: 0;border-radius: 0;font: 14px sans-serif;margin: 0}textarea,input,select{outline: 0;font-size: 12px;padding: 8px;width: 90%} input{border-radius:0.5em} .btn a{text-decoration: none}.container{margin: auto;width: 100%}@media(min-width:1200px){.container{margin: auto;width: 30%}}@media(min-width:768px) and (max-width:1200px){.container{margin: auto;width: 50%;padding:20px;}}.btn{font-size: 1em;},.h2{font-size: 0em;}h1{font-size: 2em}.btn{background: #6A9C31;border-radius: 4px;border: 0;color: #fff;cursor: pointer;display: inline-block;margin: 2px 0;padding: 10px 14px 11px;width: 100%}.btn:hover{background: #810D70}.btn:active,.btn:focus{background: #08b}label>*{display: inline}form>*{display: block;margin-bottom: 10px}textarea:focus,input:focus,select:focus{border-color: #5ab}.msg{background: #def;border-left: 5px solid #59d;padding: 1.5em}.q{float: right;width: 64px;text-align: right}.l{background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center;background-size: 1em}input[type='checkbox']{float: left;width: 20px}.table td{padding:.5em;text-align:left}.table tbody>:nth-child(2n-1){background:#ddd}fieldset{border:0px;margin:0px;}</style>";
//////

// KH, update from v1.1.0
const char WM_HTTP_SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();document.getElementById('s1').value=l.innerText||l.textContent;document.getElementById('p1').focus();}</script>";
//////

// From v1.0.9 to permit disable or configure NTP from sketch
#ifndef USE_ESP_WIFIMANAGER_NTP
  // From v1.0.6 to enable NTP config
  #define USE_ESP_WIFIMANAGER_NTP     true
#endif

#if USE_ESP_WIFIMANAGER_NTP

const char WM_HTTP_SCRIPT_NTP_MSG[] PROGMEM = "";

// From v1.0.9 to permit disable or configure NTP from sketch
#ifndef USE_CLOUDFLARE_NTP
  #define USE_CLOUDFLARE_NTP          false
#endif

#if USE_CLOUDFLARE_NTP
const char WM_HTTP_SCRIPT_NTP[] PROGMEM = "<script src='https://cdnjs.cloudflare.com/ajax/libs/jstimezonedetect/1.0.4/jstz.min.js'></script><script>var timezone=jstz.determine();console.log('Your timezone is:' + timezone.name());document.getElementById('timezone').innerHTML = timezone.name();</script>";
#else
const char WM_HTTP_SCRIPT_NTP[] PROGMEM = "<script>(function(e){var t=function(){\"use strict\";var e=\"s\",n=function(e){var t=-e.getTimezoneOffset();return t!==null?t:0},r=function(e,t,n){var r=new Date;return e!==undefined&&r.setFullYear(e),r.setDate(n),r.setMonth(t),r},i=function(e){return n(r(e,0,2))},s=function(e){return n(r(e,5,2))},o=function(e){var t=e.getMonth()>7?s(e.getFullYear()):i(e.getFullYear()),r=n(e);return t-r!==0},u=function(){var t=i(),n=s(),r=i()-s();return r<0?t+\",1\":r>0?n+\",1,\"+e:t+\",0\"},a=function(){var e=u();return new t.TimeZone(t.olson.timezones[e])},f=function(e){var t=new Date(2010,6,15,1,0,0,0),n={\"America/Denver\":new Date(2011,2,13,3,0,0,0),\"America/Mazatlan\":new Date(2011,3,3,3,0,0,0),\"America/Chicago\":new Date(2011,2,13,3,0,0,0),\"America/Mexico_City\":new Date(2011,3,3,3,0,0,0),\"America/Asuncion\":new Date(2012,9,7,3,0,0,0),\"America/Santiago\":new Date(2012,9,3,3,0,0,0),\"America/Campo_Grande\":new Date(2012,9,21,5,0,0,0),\"America/Montevideo\":new Date(2011,9,2,3,0,0,0),\"America/Sao_Paulo\":new Date(2011,9,16,5,0,0,0),\"America/Los_Angeles\":new Date(2011,2,13,8,0,0,0),\"America/Santa_Isabel\":new Date(2011,3,5,8,0,0,0),\"America/Havana\":new Date(2012,2,10,2,0,0,0),\"America/New_York\":new Date(2012,2,10,7,0,0,0),\"Asia/Beirut\":new Date(2011,2,27,1,0,0,0),\"Europe/Helsinki\":new Date(2011,2,27,4,0,0,0),\"Europe/Istanbul\":new Date(2011,2,28,5,0,0,0),\"Asia/Damascus\":new Date(2011,3,1,2,0,0,0),\"Asia/Jerusalem\":new Date(2011,3,1,6,0,0,0),\"Asia/Gaza\":new Date(2009,2,28,0,30,0,0),\"Africa/Cairo\":new Date(2009,3,25,0,30,0,0),\"Pacific/Auckland\":new Date(2011,8,26,7,0,0,0),\"Pacific/Fiji\":new Date(2010,11,29,23,0,0,0),\"America/Halifax\":new Date(2011,2,13,6,0,0,0),\"America/Goose_Bay\":new Date(2011,2,13,2,1,0,0),\"America/Miquelon\":new Date(2011,2,13,5,0,0,0),\"America/Godthab\":new Date(2011,2,27,1,0,0,0),\"Europe/Moscow\":t,\"Asia/Yekaterinburg\":t,\"Asia/Omsk\":t,\"Asia/Krasnoyarsk\":t,\"Asia/Irkutsk\":t,\"Asia/Yakutsk\":t,\"Asia/Vladivostok\":t,\"Asia/Kamchatka\":t,\"Avrupa/Türkiye\":t,\"Australia/Perth\":new Date(2008,10,1,1,0,0,0)};return n[e]};return{determine:a,date_is_dst:o,dst_start_for:f}}();t.TimeZone=function(e){\"use strict\";var n={\"America/Denver\":[\"America/Denver\",\"America/Mazatlan\"],\"America/Chicago\":[\"America/Chicago\",\"America/Mexico_City\"],\"America/Santiago\":[\"America/Santiago\",\"America/Asuncion\",\"America/Campo_Grande\"],\"America/Montevideo\":[\"America/Montevideo\",\"America/Sao_Paulo\"],\"Asia/Beirut\":[\"Asia/Beirut\",\"Europe/Helsinki\",\"Europe/Istanbul\",\"Asia/Damascus\",\"Asia/Jerusalem\",\"Asia/Gaza\"],\"Pacific/Auckland\":[\"Pacific/Auckland\",\"Pacific/Fiji\"],\"America/Los_Angeles\":[\"America/Los_Angeles\",\"America/Santa_Isabel\"],\"America/New_York\":[\"America/Havana\",\"America/New_York\"],\"America/Halifax\":[\"America/Goose_Bay\",\"America/Halifax\"],\"America/Godthab\":[\"America/Miquelon\",\"America/Godthab\"],\"Asia/Dubai\":[\"Europe/Moscow\"],\"Asia/Dhaka\":[\"Asia/Yekaterinburg\"],\"Asia/Jakarta\":[\"Asia/Omsk\"],\"Asia/Shanghai\":[\"Asia/Krasnoyarsk\",\"Australia/Perth\"],\"Asia/Tokyo\":[\"Asia/Irkutsk\"],\"Australia/Brisbane\":[\"Asia/Yakutsk\"],\"Pacific/Noumea\":[\"Asia/Vladivostok\"],\"Pacific/Tarawa\":[\"Asia/Kamchatka\"],\"Africa/Johannesburg\":[\"Asia/Gaza\",\"Africa/Cairo\"],\"Asia/Baghdad\":[\"Europe/Minsk\"]},r=e,i=function(){var e=n[r],i=e.length,s=0,o=e[0];for(;s<i;s+=1){o=e[s];if(t.date_is_dst(t.dst_start_for(o))){r=o;return}}},s=function(){return typeof n[r]!=\"undefined\"};return s()&&i(),{name:function(){return r}}},t.olson={},t.olson.timezones={\"-720,0\":\"Etc/GMT+12\",\"-660,0\":\"Pacific/Pago_Pago\",\"-600,1\":\"America/Adak\",\"-600,0\":\"Pacific/Honolulu\",\"-570,0\":\"Pacific/Marquesas\",\"-540,0\":\"Pacific/Gambier\",\"-540,1\":\"America/Anchorage\",\"-480,1\":\"America/Los_Angeles\",\"-480,0\":\"Pacific/Pitcairn\",\"-420,0\":\"America/Phoenix\",\"-420,1\":\"America/Denver\",\"-360,0\":\"America/Guatemala\",\"-360,1\":\"America/Chicago\",\"-360,1,s\":\"Pacific/Easter\",\"-300,0\":\"America/Bogota\",\"-300,1\":\"America/New_York\",\"-270,0\":\"America/Caracas\",\"-240,1\":\"America/Halifax\",\"-240,0\":\"America/Santo_Domingo\",\"-240,1,s\":\"America/Santiago\",\"-210,1\":\"America/St_Johns\",\"-180,1\":\"America/Godthab\",\"-180,0\":\"America/Argentina/Buenos_Aires\",\"-180,1,s\":\"America/Montevideo\",\"-120,0\":\"Etc/GMT+2\",\"-120,1\":\"Etc/GMT+2\",\"-60,1\":\"Atlantic/Azores\",\"-60,0\":\"Atlantic/Cape_Verde\",\"0,0\":\"Etc/UTC\",\"0,1\":\"Europe/London\",\"60,1\":\"Europe/Berlin\",\"60,0\":\"Africa/Lagos\",\"60,1,s\":\"Africa/Windhoek\",\"120,1\":\"Asia/Beirut\",\"120,0\":\"Africa/Johannesburg\",\"180,0\":\"Asia/Baghdad\",\"180,1\":\"Europe/Moscow\",\"210,1\":\"Asia/Tehran\",\"240,0\":\"Asia/Dubai\",\"240,1\":\"Asia/Baku\",\"270,0\":\"Asia/Kabul\",\"300,1\":\"Asia/Yekaterinburg\",\"300,0\":\"Asia/Karachi\",\"330,0\":\"Asia/Kolkata\",\"345,0\":\"Asia/Kathmandu\",\"360,0\":\"Asia/Dhaka\",\"360,1\":\"Asia/Omsk\",\"390,0\":\"Asia/Rangoon\",\"420,1\":\"Asia/Krasnoyarsk\",\"420,0\":\"Asia/Jakarta\",\"480,0\":\"Asia/Shanghai\",\"480,1\":\"Asia/Irkutsk\",\"525,0\":\"Australia/Eucla\",\"525,1,s\":\"Australia/Eucla\",\"540,1\":\"Asia/Yakutsk\",\"540,0\":\"Asia/Tokyo\",\"570,0\":\"Australia/Darwin\",\"570,1,s\":\"Australia/Adelaide\",\"600,0\":\"Australia/Brisbane\",\"600,1\":\"Asia/Vladivostok\",\"600,1,s\":\"Australia/Sydney\",\"630,1,s\":\"Australia/Lord_Howe\",\"660,1\":\"Asia/Kamchatka\",\"660,0\":\"Pacific/Noumea\",\"690,0\":\"Pacific/Norfolk\",\"720,1,s\":\"Pacific/Auckland\",\"720,0\":\"Pacific/Tarawa\",\"765,1,s\":\"Pacific/Chatham\",\"780,0\":\"Pacific/Tongatapu\",\"780,1,s\":\"Pacific/Apia\",\"840,0\":\"Pacific/Kiritimati\"},typeof exports!=\"undefined\"?exports.jstz=t:e.jstz=t})(this);</script><script>var timezone=jstz.determine();console.log('Your timezone is:' + timezone.name());document.getElementById('timezone').innerHTML = timezone.name();</script>";
#endif

#else
const char WM_HTTP_SCRIPT_NTP_MSG[] PROGMEM = "";
const char WM_HTTP_SCRIPT_NTP[] PROGMEM = "";
#endif

// KH, update from v1.0.10
const char WM_HTTP_HEAD_END[] PROGMEM = "</head><body><div class=\"container\"><div style=\"text-align:center;margin:auto;display:block;\"><img style=\"padding:20px\" width=\"200px\" src=\"data:image/png;base64,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\" /><br/>";

const char WM_FLDSET_START[]  PROGMEM = "<fieldset>";
const char WM_FLDSET_END[]    PROGMEM = "</fieldset>";
const char WM_FLDSET_BORDER_START[]  PROGMEM = "<fieldset style='border: 2px solid black; border-radius:1.5em'>";
//////

const char WM_HTTP_PORTAL_OPTIONS[] PROGMEM = "<form action=\"/wifi\" method=\"get\"><button style=\"width:70%;margin-top:4em; \" class=\"btn\">Ayarlar</button></form><br/>";
const char WM_HTTP_ITEM[] PROGMEM = "<div style='padding-bottom:5px'><a href=\"#p\" onclick=\"c(this)\">{v}</a>&nbsp;<span class=\"q {i}\">{r}%</span></div>";
const char JSON_ITEM[] PROGMEM = "{\"SSID\":\"{v}\", \"Encryption\":{i}, \"Quality\":\"{r}\"}";

// KH, update from v1.1.0
const char WM_HTTP_FORM_START[] PROGMEM = "<form method=\"get\" action=\"wifisave\"><fieldset><div><label>Bağlantı Adı:</label><input id=\"s\" name=\"s\" length=32  placeholder=\"Lütfen modeminizi seçiniz.\"><div></div></div><div><label>Şifre:</label><input id=\"p\" name=\"p\" length=64 placeholder=\"Lütfen şifrenizi giriniz.\"></div></fieldset>";
//////

// KH, add from v1.0.10
const char WM_HTTP_FORM_LABEL_BEFORE[] PROGMEM = "<div><label for=\"{i}\">{p}</label><input id=\"{i}\" name=\"{n}\" length={l} placeholder=\"{p}\" value=\"{v}\" {c}><div></div></div>";
const char WM_HTTP_FORM_LABEL_AFTER[] PROGMEM = "<div><input id=\"{i}\" name=\"{n}\" length={l} placeholder=\"{p}\" value=\"{v}\" {c}><label for=\"{i}\">{p}</label><div></div></div>";
//////

const char WM_HTTP_FORM_LABEL[] PROGMEM = "<label for=\"{i}\">{p}</label>";
const char WM_HTTP_FORM_PARAM[] PROGMEM = "<input id=\"{i}\" name=\"{n}\" length={l} placeholder=\"{p}\" value=\"{v}\" {c}>";

const char WM_HTTP_FORM_END[] PROGMEM = "<button class=\"btn\" style=\"width:70%\" type=\"submit\">Kaydet</button></form>";

// KH, update from v1.1.0
const char WM_HTTP_SAVED[] PROGMEM = "<div class=\"msg\"><b>Bağlantı kaydedildi!</b><br></div>";
//////

const char WM_HTTP_END[] PROGMEM = "</div></body></html>";

//KH, from v1.1.0
const char WM_HTTP_HEAD_CL[]         PROGMEM = "Content-Length";
const char WM_HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char WM_HTTP_HEAD_CT2[]        PROGMEM = "text/plain";

//KH Add repeatedly used const
const char WM_HTTP_CACHE_CONTROL[]   PROGMEM = "Cache-Control";
const char WM_HTTP_NO_STORE[]        PROGMEM = "no-cache, no-store, must-revalidate";
const char WM_HTTP_PRAGMA[]          PROGMEM = "Pragma";
const char WM_HTTP_NO_CACHE[]        PROGMEM = "no-cache";
const char WM_HTTP_EXPIRES[]         PROGMEM = "Expires";
const char WM_HTTP_CORS[]            PROGMEM = "Access-Control-Allow-Origin";
const char WM_HTTP_CORS_ALLOW_ALL[]  PROGMEM = "*";

#if USE_AVAILABLE_PAGES
const char WM_HTTP_AVAILABLE_PAGES[] PROGMEM = "<h3>Available Pages</h3><table class=\"table\"><thead><tr><th>Page</th><th>Function</th></tr></thead><tbody><tr><td><a href=\"/\">/</a></td><td>Menu page.</td></tr><tr><td><a href=\"/wifi\">/wifi</a></td><td>Show WiFi scan results and enter WiFi configuration.</td></tr><tr><td><a href=\"/wifisave\">/wifisave</a></td><td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr><tr><td><a href=\"/close\">/close</a></td><td>Close the configuration server and configuration WiFi network.</td></tr><tr><td><a href=\"/i\">/i</a></td><td>This page.</td></tr><tr><td><a href=\"/r\">/r</a></td><td>Delete WiFi configuration and reboot. ESP device will not reconnect to a network until new WiFi configuration data is entered.</td></tr><tr><td><a href=\"/state\">/state</a></td><td>Current device state in JSON format. Interface for programmatic WiFi configuration.</td></tr><tr><td><a href=\"/scan\">/scan</a></td><td>Run a WiFi scan and return results in JSON format. Interface for programmatic WiFi configuration.</td></tr></table>";
#else
const char WM_HTTP_AVAILABLE_PAGES[] PROGMEM = "";
#endif

//KH
#define WIFI_MANAGER_MAX_PARAMS 20

/////////////////////////////////////////////////////////////////////////////
// New in v1.4.0
typedef struct
{
  const char *_id;
  const char *_placeholder;
  char       *_value;
  int         _length;
  int         _labelPlacement;

}  WMParam_Data;
//////

class ESP_WMParameter 
{
  public:
    ESP_WMParameter(const char *custom);
    //ESP_WMParameter(const char *id, const char *placeholder, const char *defaultValue, int length);
    //ESP_WMParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom);
    ESP_WMParameter(const char *id, const char *placeholder, const char *defaultValue, int length, 
                    const char *custom = "", int labelPlacement = WFM_LABEL_BEFORE);
    
    // New in v1.4.0              
    ESP_WMParameter(WMParam_Data WMParam_data);                      
    //////
                   
    ~ESP_WMParameter();
    
    // New in v1.4.0
    void setWMParam_Data(WMParam_Data WMParam_data);
    void getWMParam_Data(WMParam_Data &WMParam_data);
    //////

    const char *getID();
    const char *getValue();
    const char *getPlaceholder();
    int         getValueLength();
    int         getLabelPlacement();
    const char *getCustomHTML();
    
  private:
  
#if 1
    WMParam_Data _WMParam_data;
#else  
    const char *_id;
    const char *_placeholder;
    char       *_value;
    int         _length;
    int         _labelPlacement;
#endif
    
    const char *_customHTML;

    void init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement);

    friend class ESP_WiFiManager;
};

#define USE_DYNAMIC_PARAMS				true
#define DEFAULT_PORTAL_TIMEOUT  	60000L

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You have to explicitly specify false to disable the feature.
#ifndef USE_STATIC_IP_CONFIG_IN_CP
  #define USE_STATIC_IP_CONFIG_IN_CP          false
#endif

class ESP_WiFiManager
{
  public:

    ESP_WiFiManager(const char *iHostname = "");

    ~ESP_WiFiManager();

    // Update feature from v1.0.11. Can use with STA staticIP now
    bool          autoConnect();
    bool          autoConnect(char const *apName, char const *apPassword = NULL);
    //////

    //if you want to start the config portal
    bool          startConfigPortal();
    bool          startConfigPortal(char const *apName, char const *apPassword = NULL);

    // get the AP name of the config portal, so it can be used in the callback
    String        getConfigPortalSSID();
    // get the AP password of the config portal, so it can be used in the callback
    String        getConfigPortalPW();

    void          resetSettings();

    //sets timeout before webserver loop ends and exits even if there has been no setup.
    //usefully for devices that failed to connect at some point and got stuck in a webserver loop
    //in seconds setConfigPortalTimeout is a new name for setTimeout
    void          setConfigPortalTimeout(unsigned long seconds);
    void          setTimeout(unsigned long seconds);

    //sets timeout for which to attempt connecting, usefull if you get a lot of failed connects
    void          setConnectTimeout(unsigned long seconds);


    void          setDebugOutput(bool debug);
    //defaults to not showing anything under 8% signal quality if called
    void          setMinimumSignalQuality(int quality = 8);
    
    // KH, new from v1.0.10 to enable dynamic/random channel
    int           setConfigPortalChannel(int channel = 1);
    //////
    
    //sets a custom ip /gateway /subnet configuration
    void          setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
    
    // New in v1.4.0
    void          setAPStaticIPConfig(WiFi_AP_IPConfig  WM_AP_IPconfig);
    void          getAPStaticIPConfig(WiFi_AP_IPConfig  &WM_AP_IPconfig);
    //////
    
    //sets config for a static IP
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
    
    // New in v1.4.0
    void          setSTAStaticIPConfig(WiFi_STA_IPConfig  WM_STA_IPconfig);
    void          getSTAStaticIPConfig(WiFi_STA_IPConfig  &WM_STA_IPconfig);
    //////
    
#if USE_CONFIGURABLE_DNS
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn,
                                       IPAddress dns_address_1, IPAddress dns_address_2);
#endif

    //called when AP mode and config portal is started
    void          setAPCallback(void(*func)(ESP_WiFiManager*));
    //called when settings have been changed and connection was successful
    void          setSaveConfigCallback(void(*func)());

#if USE_DYNAMIC_PARAMS
    //adds a custom parameter
    bool 				addParameter(ESP_WMParameter *p);
#else
    //adds a custom parameter
    void 				addParameter(ESP_WMParameter *p);
#endif

    //if this is set, it will exit after config, even if connection is unsucessful.
    void          setBreakAfterConfig(bool shouldBreak);
    //if this is set, try WPS setup when starting (this will delay config portal for up to 2 mins)
    //TODO
    //if this is set, customise style
    void          setCustomHeadElement(const char* element);
    //if this is true, remove duplicated Access Points - defaut true
    void          setRemoveDuplicateAPs(bool removeDuplicates);
    //Scan for WiFiNetworks in range and sort by signal strength
    //space for indices array allocated on the heap and should be freed when no longer required
    int           scanWifiNetworks(int **indicesptr);

    // return SSID of router in STA mode got from config portal. NULL if no user's input //KH
    String				getSSID() 
    {
      return _ssid;
    }

    // return password of router in STA mode got from config portal. NULL if no user's input //KH
    String				getPW() 
    {
      return _pass;
    }
    
    // New from v1.1.0
    // return SSID of router in STA mode got from config portal. NULL if no user's input //KH
    String				getSSID1() 
    {
      return _ssid1;
    }

    // return password of router in STA mode got from config portal. NULL if no user's input //KH
    String				getPW1() 
    {
      return _pass1;
    }
    
    #define MAX_WIFI_CREDENTIALS        2
    
    String				getSSID(uint8_t index) 
    {
      if (index == 0)
        return _ssid;
      else if (index == 1)
        return _ssid1;
      else     
        return String("");
    }
    
    String				getPW(uint8_t index) 
    {
      if (index == 0)
        return _pass;
      else if (index == 1)
        return _pass1;
      else     
        return String("");
    }
    //////
    
    // New from v1.1.1, for configure CORS Header, default to WM_HTTP_CORS_ALLOW_ALL = "*"
#if USING_CORS_FEATURE
    void setCORSHeader(const char* CORSHeaders)
    {     
      _CORS_Header = CORSHeaders;

      LOGWARN1(F("Set CORS Header to : "), _CORS_Header);
    }
    
    const char* getCORSHeader()
    {
      return _CORS_Header;
    }
#endif   
    
    //returns the list of Parameters
    ESP_WMParameter** getParameters();
    // returns the Parameters Count
    int           getParametersCount();

    const char*   getStatus(int status);

#ifdef ESP32
    String getStoredWiFiSSID();
    String getStoredWiFiPass();
#endif

    String WiFi_SSID()
    {
#ifdef ESP8266
      return WiFi.SSID();
#else
      return getStoredWiFiSSID();
#endif
    }

    String WiFi_Pass()
    {
#ifdef ESP8266
      return WiFi.psk();
#else
      return getStoredWiFiPass();
#endif
    }

    void setHostname()
    {
      if (RFC952_hostname[0] != 0)
      {
#ifdef ESP8266
        WiFi.hostname(RFC952_hostname);
#else		//ESP32
        // See https://github.com/espressif/arduino-esp32/issues/2537
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(RFC952_hostname);
#endif
      }
    }

  private:
    std::unique_ptr<DNSServer>        dnsServer;

    //KH, for ESP32
#ifdef ESP8266
    std::unique_ptr<ESP8266WebServer> server;
#else		//ESP32
    std::unique_ptr<WebServer>        server;
#endif

#define RFC952_HOSTNAME_MAXLEN      24
    char RFC952_hostname[RFC952_HOSTNAME_MAXLEN + 1];

    char* getRFC952_hostname(const char* iHostname);

    void          setupConfigPortal();
    void          startWPS();
    //const char*   getStatus(int status);

    const char*   _apName = "no-net";
    const char*   _apPassword = NULL;
    
    String        _ssid   = "";
    String        _pass   = "";
    
    // New from v1.1.0
    String        _ssid1  = "";
    String        _pass1  = "";
    //////
    
    // From v1.0.6 with timezone info
    String        _timezoneName         = "";

    unsigned long _configPortalTimeout  = 0;

    unsigned long _connectTimeout       = 0;
    unsigned long _configPortalStart    = 0;

    int           numberOfNetworks;
    int           *networkIndices;
    
    // KH, new from v1.0.10 to enable dynamic/random channel
    // default to channel 1
    #define MIN_WIFI_CHANNEL      1
    #define MAX_WIFI_CHANNEL      11    // Channel 12,13 is flaky, because of bad number 13 ;-)

    int _WiFiAPChannel = 1;
    //////

    // New in v1.4.0
    WiFi_AP_IPConfig  _WiFi_AP_IPconfig;
    
    WiFi_STA_IPConfig _WiFi_STA_IPconfig = { IPAddress(0, 0, 0, 0), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0),
                                             IPAddress(192, 168, 2, 1), IPAddress(8, 8, 8, 8) };
    //////
    
    int           _paramsCount            = 0;
    int           _minimumQuality         = -1;
    bool          _removeDuplicateAPs     = true;
    bool          _shouldBreakAfterConfig = false;
    bool          _tryWPS                 = false;

    const char*   _customHeadElement      = "";

    int           status                  = WL_IDLE_STATUS;
    
    // New from v1.1.0, for configure CORS Header, default to WM_HTTP_CORS_ALLOW_ALL = "*"
#if USING_CORS_FEATURE
    const char*     _CORS_Header          = WM_HTTP_CORS_ALLOW_ALL;   //"*";
#endif   
    //////
    
    // New v1.0.8
    void          setWifiStaticIP();
    
    // New v1.1.0
    int           reconnectWifi();
    //////
    
    // New v1.0.11
    int           connectWifi(String ssid = "", String pass = "");
    //////
    
    uint8_t       waitForConnectResult();

    void          handleRoot();
    void          handleWifi();
    void          handleWifiSave();
    void          handleServerClose();
    void          handleInfo();
    void          handleState();
    void          handleScan();
    void          handleReset();
    void          handleNotFound();
    bool          captivePortal();
    
    void          reportStatus(String &page);

    // DNS server
    const byte    DNS_PORT = 53;

    //helpers
    int           getRSSIasQuality(int RSSI);
    bool          isIp(String str);
    String        toStringIp(IPAddress ip);

    bool          connect;
    bool          stopConfigPortal = false;
    
    bool          _debug = false;     //true;

    void(*_apcallback)  (ESP_WiFiManager*)  = NULL;
    void(*_savecallback)()              = NULL;

#if USE_DYNAMIC_PARAMS
    int                    _max_params;
    ESP_WMParameter** _params;
#else
    ESP_WMParameter* _params[WIFI_MANAGER_MAX_PARAMS];
#endif

    template <typename Generic>
    void          DEBUG_WM(Generic text);

    template <class T>
    auto optionalIPFromString(T *obj, const char *s) -> decltype(obj->fromString(s)) {
      return  obj->fromString(s);
    }
    auto optionalIPFromString(...) -> bool {
      LOGINFO("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0 or newer for Custom IP configuration to work.");
      return false;
    }
};

#include "ESP_WiFiManager-Impl.h"

