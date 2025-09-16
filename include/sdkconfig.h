#ifndef SDKCONFIG_H
#define SDKCONFIG_H

// ===== Target selection =====
#define CONFIG_IDF_TARGET_ESP32S3 1
#define CONFIG_IDF_TARGET "esp32s3"

// ===== Newlib =====
#define CONFIG_NEWLIB_NANO_FORMAT 1

// ===== FreeRTOS =====
#define CONFIG_FREERTOS_MAX_TASK_NAME_LEN 32
// Tick rate used in many Arduino core places (HWCDC, timing macros)
#define CONFIG_FREERTOS_HZ 1000

// ===== LwIP / esp_netif =====
// WiFi libs expect IPv6 APIs and u_addr union in ip_addr_t -> enable IPv6.
#define CONFIG_LWIP_IPV6 1
// DHCP coarse timer used by lwipopts.h on ESP32
#define CONFIG_LWIP_DHCP_COARSE_TIMER_SECS 60
// TCP oversize (must pick exactly one)
#define CONFIG_LWIP_TCP_OVERSIZE_MSS 1
// #define CONFIG_LWIP_TCP_OVERSIZE_QUARTER_MSS 1
// #define CONFIG_LWIP_TCP_OVERSIZE_DISABLE 1
// ===== LwIP / esp_netif (more) =====
#define CONFIG_LWIP_IPV6_NUM_ADDRESSES    3   // default in IDF
// Enable DHCP server types (fixes dhcps_lease_t usage in WiFiGeneric.cpp)
#define CONFIG_LWIP_DHCP_SERVER           1
// Some cores still check the legacy name; define both to be safe:
#define CONFIG_LWIP_DHCPS                 1


// ===== ESP Event (ISR posting used by HWCDC) =====
#define CONFIG_ESP_EVENT_POST_FROM_ISR 1

// ===== WiFi init defaults expected by WIFI_INIT_CONFIG_DEFAULT() =====
// Reasonable, common values (taken from typical IDF defaults)
#define CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM   10
#define CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM  32
// 0: static, 1: dynamic  (dynamic is the common Arduino default)
#define CONFIG_ESP32_WIFI_TX_BUFFER_TYPE         1
// Use dynamic RX management buffers
#define CONFIG_ESP_WIFI_DYNAMIC_RX_MGMT_BUF      1
// ESPNOW max encrypt peers (Arduino core references this in init macro)
#define CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM   0
// ---- TCP/IP adapter compatibility (brings back dhcps_lease_t, etc.) ----
#define CONFIG_ESP_NETIF_TCPIP_ADAPTER_COMPATIBLE 1
// Optional but harmless; some cores check this name too:
#define CONFIG_LWIP_DHCPS_LEASE 1


// ---- Notes ----
// If you see any further "CONFIG_XXXX not declared" errors, tell me the exact
// symbol and I'll add the minimal define here (or via -D in build_flags).

#endif // SDKCONFIG_H



