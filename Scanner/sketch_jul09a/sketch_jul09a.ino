#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "soc/rtc_wdt.h"
#include "CRC32.h"


#define BYTES_PER_SSID 32
#define BYTES_PER_MAC 6
#define MAX_CHANNEL 11

int channel = 1;

static wifi_country_t wifi_country = {.cc = "US", .schan = 1, .nchan = 11}; // Most recent esp32 library struct

typedef struct
{
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

String mac2String(const uint8_t ar[]) {
  String s;
  for (byte i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%02X", ar[i]);
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}

void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type)
{
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  String srcMac = mac2String(hdr->addr1);
  String dstMac = mac2String(hdr->addr2);
  
  Serial.println("1," + (String)channel + "," + srcMac + "," + dstMac);

  if (ppkt->payload[0] == 0x40) {
    char ssid[BYTES_PER_SSID + 1] = {0};
    int ssidLength = ppkt->payload[25];

    for (int i = 0; i < ssidLength; i++)
    {
        // Serial.print((char)ppkt->payload[26 + i]);
        ssid[i] = (char)ppkt->payload[26 + i];
    }
    ssid[ssidLength] = '\0';
    Serial.println("2," + (String)channel + "," + (String)dstMac + "," + (String)ssid);
  } else if (ppkt->payload[0] == 0x80)
  {
    int ssidLength = ppkt->payload[37];
    char ssid[BYTES_PER_SSID + 1];
    strncpy(ssid, (const char*)ppkt->payload+38, ssidLength);
    ssid[ssidLength] = '\0';

    Serial.println("3," + (String)channel + "," + (String)dstMac + "," + (String)ssid);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  tcpip_adapter_init();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_country(&wifi_country); /* set country for channel range [1, 13] */
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void loop() {
  if (channel < MAX_CHANNEL) {
    channel += 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  } else {
    channel = 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }
  
  // Stay on each Wi-Fi channel for 1 second
  delay(1000);
}
