#include <ESP8266WiFi.h>
#include <lwip/napt.h>

#define NAPT 1000
#define NAPT_PORT 10

const char* hostname     = "extender-uz";
const char* ssid_ap      = hostname;
const char* key_ap       = "nou8haiy";
const char* ssid_station = "point-uz";
const char* key_station  = "nou8haiy";
int         channel      = 2;

IPAddress ipaddress_ap (192,168,5,254);
IPAddress gateway_ap   (192,168,5,254);
IPAddress netmask_ap   (255,255,255,0);

IPAddress ipaddress_station (192,168,4,20);
IPAddress gateway_station   (192,168,4,20);
IPAddress netmask_station   (255,255,255,0);

void setup() {
    WiFi.mode(WIFI_OFF);
    WiFi.setPhyMode(WIFI_PHY_MODE_11B);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.hostname(hostname);
    WiFi.mode(WIFI_AP_STA);
    WiFi.config(ipaddress_station, gateway_station, netmask_station);
    WiFi.begin(ssid_station, key_station, channel);
    while (WiFi.status() != WL_CONNECTED) {
        delay(50);
    }
    WiFi.softAPConfig(ipaddress_ap, gateway_ap, netmask_ap);
    WiFi.softAP(ssid_ap, key_ap, channel);

    ip_napt_init(NAPT, NAPT_PORT);
    ip_napt_enable_no(SOFTAP_IF, 1);
}

void loop() {
}
