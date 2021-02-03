#ifndef __VMAC_WIFI_INPUT_H__
#define __VMAC_WIFI_INPUT_H__

#include "wifi_mac_var.h"

#define UNSUPPORTED_AUTH_ALGORITHM 13
#define SUCCESS 0

#define HAS_SEQ(type)   ((type & 0x4) == 0)

#define WIFINET_VERIFY_ELEMENT(__elem, __maxlen) do {         \
                if ((__elem) == NULL) {                     \
                        WIFINET_DPRINTF( AML_DEBUG_ELEMID,        \
                                          "%s", "no " #__elem );              \
                        wnet_vif->vif_sts.sts_rx_elem_miss++;         \
                        return;                         \
                }                               \
                if ((__elem)[1] > (__maxlen)) {                 \
                        WIFINET_DPRINTF( AML_DEBUG_ELEMID,        \
                                          "bad " #__elem " len %d", (__elem)[1]);     \
                        wnet_vif->vif_sts.sts_rx_elem_too_long++;          \
                        return;                         \
                }                               \
        } while (0)

#define WIFINET_VERIFY_LENGTH(_len, _minlen) do {         \
                if ((_len) < (_minlen)) {                   \
                        WIFINET_DPRINTF( AML_DEBUG_ELEMID,        \
                                          "%s", "ie too short");              \
                        wnet_vif->vif_sts.sts_rx_elem_too_short++;            \
                        return;                         \
                }                               \
        } while (0)


#define WIFINET_VERIFY_SSID(_ni, _ssid) do {              \
                if ((_ssid) && (_ssid)[1] != 0 &&                      \
                                ((_ssid)[1] != (_ni)->sta_esslen ||              \
                                 memcmp((_ssid) + 2, (_ni)->sta_essid, (_ssid)[1]) != 0)) {   \
                        wnet_vif->vif_sts.sts_rx_ssid_mismatch++;         \
                        return;                         \
                }                               \
        } while (0)


#define WAI_OUI_BE                 0x0050f201
#define WPA_SEL(x)  (((x)<<24)|WPA_OUI)
#define RSN_SEL(x)  (((x)<<24)|RSN_OUI)

#define MS(_v, _f)  (((_v) & _f) >> _f##_S)
#define ISREASSOC(_st)  ((_st) == WIFINET_FC0_SUBTYPE_REASSOC_RESP)

int iswpsoui(const unsigned char *frm);
int iswfdoui(const unsigned char *frm);
int isp2poui(const unsigned char *frm);

int wifi_mac_rx_get_wnet_vifid(struct wifi_mac *wifimac,struct wifi_frame *wh);
int wifi_mac_input(void *, struct sk_buff *, struct wifi_mac_rx_status *);
int wifi_mac_input_all(struct wifi_mac *wifimac,struct sk_buff *skb, struct wifi_mac_rx_status *rs);
int wpa_cipher(unsigned char *sel, unsigned char *keylen);
int wpa_keymgmt(unsigned char *sel);
int rsn_cipher(unsigned char *sel, unsigned char *keylen);
int rsn_keymgmt(unsigned char *sel);
void wifi_mac_savenie(unsigned char **, const unsigned char *, size_t, char *name);
void wifi_mac_saveie(unsigned char **, const unsigned char *, char *name);
void wifi_mac_parse_htcap(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_htinfo(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_vht_cap(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_vht_opt(struct wifi_station *sta, unsigned char *ie);

void wifi_mac_parse_vht_txpw(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_vht_ch_sw_wrp(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_vht_ext_bss_ld(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_vht_quiet_ch(struct wifi_station *sta, unsigned char *ie);
void wifi_mac_parse_vht_op_md_ntf(struct wifi_station *sta, unsigned char opt_mode);
void wifi_mac_vht_ie_parse_all(struct wifi_station *sta, struct wifi_mac_scan_param* sp);

void wifi_mac_recv_beacon(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb,int rssi);
void wifi_mac_recv_probersp(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb,int rssi);
void wifi_mac_recv_probe_req(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb,int rssi);
void wifi_mac_recv_assoc_req(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb, unsigned int channel, int rssi);
void wifi_mac_recv_assoc_rsp(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb, unsigned int channel);
void wifi_mac_recv_deauth(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb, unsigned int channel);
void wifi_mac_recv_disassoc(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb, unsigned int channel);
void wifi_mac_recv_action(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, struct sk_buff *skb, unsigned int channel);
void wifi_mac_recv_mgmt(struct wifi_station *, struct sk_buff *, int, int, unsigned int);
void wifi_mac_parse_group_id_mgmt(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, unsigned char *frame);
void wifi_mac_parse_operate_mode_notification_mgmt(struct wlan_net_vif *wnet_vif, struct wifi_station *sta, unsigned char *frame);
void wifi_mac_check_mic(struct wifi_station *, struct sk_buff *);

#endif