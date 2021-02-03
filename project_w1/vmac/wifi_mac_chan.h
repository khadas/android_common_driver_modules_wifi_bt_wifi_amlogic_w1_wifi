#ifndef __NET80211_CHAN_H__
#define __NET80211_CHAN_H__

#define WIFI_CHANNEL_LOCK(wifimac) OS_SPIN_LOCK_IRQ(&(wifimac)->channel_lock, (wifimac)->channel_lock_flag)
#define WIFI_CHANNEL_UNLOCK(wifimac) OS_SPIN_UNLOCK_IRQ(&(wifimac)->channel_lock, (wifimac)->channel_lock_flag)

void wifi_mac_update_country_chan_list(struct wifi_mac *wifimac);
void wifi_mac_chan_setup(void * ieee, unsigned int wMode, int countrycode_ex);
int wifi_mac_chan_attach (struct wifi_mac *wifimac);
unsigned char wifi_mac_chan_num_availd (struct wifi_mac *wifimac, unsigned char channum);
int wifi_mac_recv_bss_intol_channelCheck(struct wifi_mac *wifimac, struct wifi_mac_ie_intolerant_report *intol_ie);
struct wifi_channel * wifi_mac_scan_sta_get_ap_channel(struct wlan_net_vif *wnet_vif, struct wifi_mac_scan_param *sp);
unsigned int wifi_mac_mhz2chan(unsigned int freq);
struct wifi_channel * wifi_mac_find_chan(struct wifi_mac *wifimac, int chan, int bw, int center_chan);
int wifi_mac_set_wnet_vif_channel(struct wlan_net_vif *wnet_vif,  int chan, int bw, int center_chan);
void wifi_mac_set_wnet_vif_chan_ex(SYS_TYPE param1,SYS_TYPE param2,SYS_TYPE param3, SYS_TYPE param4,SYS_TYPE param5);
struct wifi_channel * wifi_mac_get_wm_chan (struct wifi_mac *wifimac);
struct wifi_channel * wifi_mac_get_connect_wnet_vif_channel(struct wlan_net_vif *wnet_vif);
void wifi_mac_restore_wnet_vif_channel(struct wlan_net_vif *wnet_vif);
unsigned int   wifi_mac_chan2ieee(struct wifi_mac *, const struct wifi_channel *);
unsigned int   wifi_mac_Ieee2mhz(unsigned int, unsigned int);
struct wifi_channel *wifi_mac_get_main_vmac_channel(struct wifi_mac *wifimac);
struct wifi_channel *wifi_mac_get_p2p_vmac_channel(struct wifi_mac *wifimac);
void chan_dbg(struct wifi_channel *chan,  char* str, int line);
void  wifi_mac_update_chan_list_by_country(int country_code, int support_opt[],int opt_num,struct wifi_mac *wifimac);
#endif//__NET80211_CHAN_H__