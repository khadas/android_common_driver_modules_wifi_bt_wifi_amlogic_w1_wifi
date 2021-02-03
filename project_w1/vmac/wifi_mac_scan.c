#include "wifi_mac_com.h"
#include "wifi_mac_rate.h"

int saveie(unsigned char *iep, const unsigned char *ie)
{
    if (ie == NULL)
    {
        (iep)[1] = 0;
    }
    else
    {
        unsigned int ielen = ie[1]+2;
        if (iep != NULL)
            memcpy(iep, ie, ielen);
        return ielen;
    }
    return 0;
}

static int wifi_mac_scan_sta_compare(const struct scaninfo_entry *a, const struct scaninfo_entry *b)
{
    if ((a->scaninfo.SI_capinfo & WIFINET_CAPINFO_PRIVACY) &&
        (b->scaninfo.SI_capinfo & WIFINET_CAPINFO_PRIVACY) == 0)
        return 1;

    if ((a->scaninfo.SI_capinfo & WIFINET_CAPINFO_PRIVACY) == 0 &&
        (b->scaninfo.SI_capinfo & WIFINET_CAPINFO_PRIVACY))
        return -1;

    if (abs(b->connectcnt - a->connectcnt) > 1)
        return b->connectcnt - a->connectcnt;

    if (abs(b->scaninfo.SI_rssi - a->scaninfo.SI_rssi) < 5)
    {
        return wifi_mac_get_sta_mode((struct wifi_scan_info *)&b->scaninfo) - wifi_mac_get_sta_mode((struct wifi_scan_info *)&a->scaninfo);
    }
    return a->scaninfo.SI_rssi - b->scaninfo.SI_rssi;
}

static int
match_ssid(const unsigned char *ie,
           int nssid, const struct wifi_mac_ScanSSID ssids[])
{
    int i;

    for (i = 0; i < nssid; i++)
    {
        if (ie[1] == ssids[i].len &&
            memcmp(ie+2, ssids[i].ssid, ie[1]) == 0)
            return 1;
    }
    return 0;
}



static int
match_bss(struct wlan_net_vif *wnet_vif,
          const struct wifi_mac_scan_state *ss, const struct scaninfo_entry *se)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    const struct wifi_scan_info *scaninfo = &se->scaninfo;
    unsigned int fail;

    fail = 0;
    if (wifi_mac_chan_num_availd(wifimac, wifi_mac_chan2ieee(wifimac, scaninfo->SI_chan))==false)
    {
        fail |= STA_MATCH_ERR_CHAN;
        //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
    }

    if (wnet_vif->vm_opmode == WIFINET_M_IBSS)
    {
        if ((scaninfo->SI_capinfo & WIFINET_CAPINFO_IBSS) == 0)
        {
             fail |= STA_MATCH_ERR_BSS;
              //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
        }
    }
    else
    {
        if ((scaninfo->SI_capinfo & WIFINET_CAPINFO_ESS) == 0)
        {
            fail |= STA_MATCH_ERR_BSS;
            //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
        }
    }

    if (wnet_vif->vm_flags & WIFINET_F_PRIVACY)
    {
        if ((scaninfo->SI_capinfo & WIFINET_CAPINFO_PRIVACY) == 0)
        {
            fail |= STA_MATCH_ERR_PRIVACY;
             //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
        }
    }
    else
    {
        if (scaninfo->SI_capinfo & WIFINET_CAPINFO_PRIVACY)
        {
            fail |= STA_MATCH_ERR_PRIVACY;
             //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
        }
    }

    if (!check_rate(wnet_vif, scaninfo))
    {
        fail |= STA_MATCH_ERR_RATE;
        //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
    }

    if ((wnet_vif->vm_fixed_rate.mode == WIFINET_FIXED_RATE_NONE) ||
        (wnet_vif->vm_fixed_rate.rateinfo & WIFINET_RATE_MCS))
    {
        if (!check_ht_rate(wnet_vif, scaninfo))
        {
            fail |= STA_MATCH_ERR_HTRATE;
            //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
        }
    }

    /*get ssid, which we want to connect */
    if (!(wnet_vif->vm_flags & WIFINET_F_IGNORE_SSID) &&((ss->ss_nssid == 0) ||
            (match_ssid(scaninfo->SI_ssid, ss->ss_nssid, ss->ss_ssid)==0)))
    {
        fail |= STA_MATCH_ERR_SSID;
        //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
    }
    if ((wnet_vif->vm_flags & WIFINET_F_DESBSSID) &&
        !WIFINET_ADDR_EQ(wnet_vif->vm_des_bssid, scaninfo->SI_bssid))
    {
        fail |= STA_MATCH_ERR_BSSID;
         //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
    }
    if (se->connectcnt >= WIFINET_CONNECT_FAILS)
    {
        fail |= STA_MATCH_ERR_STA_FAILS_MAX;
         //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
    }
    if (se->se_notseen >= WIFINET_SCAN_AGE_NUM)
    {
         fail |= STA_MATCH_ERR_STA_PURGE_SCANS;
         //printk("%s(%d) fail 0x%x\n", __func__, __LINE__, fail);
    }

    if (!(fail & STA_MATCH_ERR_SSID)) {
        DPRINTF(AML_DEBUG_WARNING, "%s fail = 0x%x, ssid=%s, chan_pri_num=%d, SI_rssi =%d, vm_flags=%x bssid=%s chan_flags=0x%x \n",
            __func__,fail, ssidie_sprintf(scaninfo->SI_ssid), scaninfo->SI_chan->chan_pri_num, scaninfo->SI_rssi,
            wnet_vif->vm_flags, ether_sprintf(scaninfo->SI_bssid), scaninfo->SI_chan->chan_flags);
    }
    return fail;
}

static void wifi_mac_save_ssid(struct wlan_net_vif *wnet_vif, struct wifi_mac_scan_state *ss,
                             int nssid, const struct wifi_mac_ScanSSID ssids[])
{
    if ((nssid == 0) || (nssid > WIFINET_SCAN_MAX_SSID)) {
        return;
    }

    memcpy(ss->ss_ssid, ssids, nssid * sizeof(ssids[0]));
    ss->ss_nssid = nssid;
}

static struct scaninfo_entry *
wifi_mac_scan_get_best_node(struct wifi_mac_scan_state *ss, struct wlan_net_vif *wnet_vif)
{
    struct scaninfo_table *st = ss->ScanTablePriv;
    struct scaninfo_entry *se, *bestNode = NULL;

    WIFI_SCAN_SE_LIST_LOCK(st);
    list_for_each_entry(se,&st->st_entry,se_list)
    {
        DPRINTF(AML_DEBUG_SCAN, "<running> %s %d se%p st_entry%p \n",__func__,__LINE__,se, &st->st_entry);
        if (match_bss(wnet_vif, ss, se) == 0)
        {
            if (bestNode == NULL)
            {
                bestNode = se;
                DPRINTF(AML_DEBUG_SCAN, "<running> %s %d %p \n",__func__,__LINE__,bestNode);
            }
            else if (wifi_mac_scan_sta_compare(se, bestNode) > 0)
            {
                bestNode = se;
                DPRINTF(AML_DEBUG_SCAN, "<running> %s %d \n",__func__,__LINE__);
            }
        }
    }
    WIFI_SCAN_SE_LIST_UNLOCK(st);
    return bestNode;
}

static unsigned char
wifi_mac_scan_get_match_node(struct wifi_mac_scan_state *ss, struct wlan_net_vif *wnet_vif)
{
    struct scaninfo_table *st = ss->ScanTablePriv;
    struct scaninfo_entry *se = NULL;
    struct scaninfo_entry *se_next = NULL;
    unsigned char ret = 0;

    wnet_vif->vm_connchan.num = 0;
    WIFI_SCAN_SE_LIST_LOCK(st);
    list_for_each_entry_safe(se,se_next,&st->st_entry,se_list)
    {
        DPRINTF(AML_DEBUG_CONNECT, "<running> %s %d se%p st_entry%p \n",__func__,__LINE__,se, &st->st_entry);
        if (match_bss(wnet_vif, ss, se) == 0)
        {
             wnet_vif->vm_connchan.conn_chan[wnet_vif->vm_connchan.num++] = se->scaninfo.SI_chan;
             WIFINET_ADDR_COPY(wnet_vif->vm_connchan.da, se->scaninfo.SI_macaddr);
             WIFINET_ADDR_COPY(wnet_vif->vm_connchan.bssid, se->scaninfo.SI_bssid);
             list_del_init(&se->se_list);
             list_del_init(&se->se_hash);
             FREE(se,"sta_add.se");
             ret = 1;
        }
    }
    WIFI_SCAN_SE_LIST_UNLOCK(st);
    return ret;
}


static int
wifi_mac_scan_connect(struct wifi_mac_scan_state *ss, struct wlan_net_vif *wnet_vif)
{
    struct scaninfo_entry *bestNode=NULL;
    struct wifi_channel        * chan;
    struct vm_wdev_priv *pwdev_priv = wdev_to_priv(wnet_vif->vm_wdev);

    const unsigned char zero_bssid[WIFINET_ADDR_LEN]= {0};

    /*AP doesn't get ssid and connect, so just return. */
    if (ss->scan_CfgFlags & WIFINET_SCANCFG_NOPICK) {
        DPRINTF(AML_DEBUG_CONNECT, "%s %d no pick, return\n",__func__,__LINE__);
        return 1;
    }

    if (pwdev_priv->connect_request == NULL) {
        DPRINTF(AML_DEBUG_CONNECT, "%s %d no connection cmd, return\n",__func__,__LINE__);
        return 1;
    }

    if (wnet_vif->vm_state > WIFINET_S_SCAN) {
        DPRINTF(AML_DEBUG_WARNING, "%s wrong state\n", __func__);
        return 1;
    }

    KASSERT((wnet_vif->vm_opmode == WIFINET_M_IBSS)
        ||(wnet_vif->vm_opmode == WIFINET_M_STA)
        ||(wnet_vif->vm_opmode == WIFINET_M_P2P_DEV)
        ||(wnet_vif->vm_opmode == WIFINET_M_P2P_CLIENT),
        ("wifi_mac_scan_connect vmopmode error opmode=%d !!",wnet_vif->vm_opmode));

    /*get a best ap from scan list, by comparing rssi, connect times... */
    bestNode = wifi_mac_scan_get_best_node(ss, wnet_vif);
    if (bestNode == NULL)
    {
        printk("%s %d no bss match, goto notfound\n", __func__,__LINE__);
        goto notfound;
    }

    if ((wnet_vif->vm_opmode == WIFINET_M_IBSS)
        && (WIFINET_ADDR_EQ(bestNode->scaninfo.SI_bssid, &zero_bssid[0])))
    {
        /* get ap work mode. */
        if( WIFINET_IS_CHAN_2GHZ( bestNode->scaninfo.SI_chan)  )
        {
            wnet_vif->vm_mac_mode =    WIFINET_MODE_11BGN;
        }
        else
        {
            wnet_vif->vm_mac_mode =    WIFINET_MODE_11GNAC;
        }

        DPRINTF(AML_DEBUG_SCAN, "<running> %s %d \n",__func__,__LINE__);
        wifi_mac_create_wifi(wnet_vif, bestNode->scaninfo.SI_chan);
        return 1;
    }
    else
    {
        DPRINTF(AML_DEBUG_SCAN|AML_DEBUG_CONNECT,
            "<running> %s %d bestNode %p connectcnt %d \n",
            __func__,__LINE__,bestNode,bestNode->connectcnt);
        bestNode->connectcnt++;
        bestNode->ConnectTime = jiffies;
        /*parse ap's beacon or probe resp frames and prepare to connect. */
        return wifi_mac_connect(wnet_vif,&bestNode->scaninfo);
    }

    DPRINTF(AML_DEBUG_CONNECT, "<running> %s %d \n",__func__,__LINE__);
    return 1;

notfound:
    //if adhoc mode, not find we must create a new
    if (wnet_vif->vm_opmode == WIFINET_M_IBSS)
    {
         if (wnet_vif->vm_curchan != WIFINET_CHAN_ERR)
            chan = wnet_vif->vm_curchan;
        else
            chan = wifi_mac_get_wm_chan(wnet_vif->vm_wmac);
        wifi_mac_create_wifi(wnet_vif, chan);
    }
    DPRINTF(AML_DEBUG_CONNECT, "%s %d entry=NULL, return\n",__func__,__LINE__);
    return 0;
}

static int wifi_mac_chk_ap_chan(struct wifi_mac_scan_state *ss, struct wlan_net_vif *wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    int channum;
    struct wifi_channel *c = NULL;

     //AP default work in 5G channel 149,wiait hostapd configure after init
    channum = DEFAULT_CHANNEL;
    DPRINTF(AML_DEBUG_SCAN|AML_DEBUG_CONNECT,
        "<%s> :  %s %d ++ vm_opmode %d\n",wnet_vif->vm_ndev->name,__func__,__LINE__,wnet_vif->vm_opmode);
    c = wifi_mac_find_chan(wifimac, channum, WIFINET_BWC_WIDTH80, channum + 6);
    if(c == NULL)
    {
        DPRINTF(AML_DEBUG_ERROR, "%s (%d) error channum=%d,  c=%p\n",
            __func__, __LINE__, channum, c);
        c  = ss->ss_chans[0];
    }
    DPRINTF(AML_DEBUG_SCAN|AML_DEBUG_CONNECT, "%s %d channel=%d  chan_flags = 0x%x\n",
        __func__, __LINE__,  channum, c->chan_flags);

    wifi_mac_create_wifi(wnet_vif, c);
    return 0;
}

int wifi_mac_scan_parse(struct wifi_mac *wifimac, wifi_mac_ScanIterFunc *f, void *arg)
{
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct scaninfo_table *st = ss->ScanTablePriv;
    struct scaninfo_entry *se = NULL, *SI_next = NULL;
    unsigned char count = 0;

    WIFI_SCAN_SE_LIST_LOCK(st);
    list_for_each_entry_safe(se,SI_next,&st->st_entry,se_list)
    {
        se->scaninfo.SI_age = jiffies - se->LastUpdateTime;
        (*f)(arg, &se->scaninfo);
        count++;
    }
    WIFI_SCAN_SE_LIST_UNLOCK(st);

    printk("%s scan_results:%d\n", __func__, count);
    return 0;
}

void wifi_mac_scan_flush(struct wifi_mac *wifimac)
{
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct scaninfo_table *st = ss->ScanTablePriv;
    struct scaninfo_entry *se=NULL, *next=NULL;

    DPRINTF(AML_DEBUG_SCAN, "<running> %s %d \n",__func__,__LINE__);

    WIFI_SCAN_SE_LIST_LOCK(st);
    list_for_each_entry_safe(se,next,&st->st_entry,se_list)
    {
        list_del_init(&se->se_list);
        list_del_init(&se->se_hash);
        FREE(se,"sta_add.se");
    }
    WIFI_SCAN_SE_LIST_UNLOCK(st);
}

void wifi_mac_scan_rx(struct wlan_net_vif *wnet_vif, const struct wifi_mac_scan_param *sp,
    const struct wifi_frame *wh, int rssi)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct scaninfo_table *st = ss->ScanTablePriv;
    const unsigned char *macaddr = wh->i_addr2;
    struct scaninfo_entry *se = NULL;
    struct wifi_scan_info *ise;
    static struct wifi_channel *apchan =NULL;
    int hash;
    int index = 0;

    if (ss->scan_StateFlags & SCANSTATE_F_DISCARD)
    {
        DPRINTF(AML_DEBUG_SCAN,"%s %d drop \n",__func__,__LINE__);
        return;
    }

    DPRINTF(AML_DEBUG_SCAN, "<running> %s, %d %s chan %d \n", __func__, sp->ssid[1], ssidie_sprintf(sp->ssid), sp->chan);

    apchan = wifi_mac_scan_sta_get_ap_channel(wnet_vif, (struct wifi_mac_scan_param *)sp);
    if (apchan == NULL)
    {
         DPRINTF(AML_DEBUG_WARNING, "%s(%d) apchan = %p\n", __func__,__LINE__,apchan);
        return;
    }

    se = (struct scaninfo_entry *)NET_MALLOC(sizeof(struct scaninfo_entry), GFP_ATOMIC, "sta_add.se");

    if (se == NULL)
    {
        return ;
    }

    WIFINET_ADDR_COPY(se->scaninfo.SI_macaddr, macaddr);
    DPRINTF(AML_DEBUG_SCAN, "<running> %s se %p se->se_list 0x%p, %s, mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
        __func__, se, &se->se_list, ssidie_sprintf(sp->ssid), macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

    ise = &se->scaninfo;
    if (sp->ssid[1] != 0 &&
        (WIFINET_IS_PROBERSP(wh) || ise->SI_ssid[1] == 0))
        memcpy(ise->SI_ssid, sp->ssid, 2+sp->ssid[1]);

    KASSERT(sp->rates[1] <= WIFINET_RATE_MAXSIZE, ("rate set too large: %u", sp->rates[1]));
    memcpy(ise->SI_rates, sp->rates, 2+sp->rates[1]);

    if (sp->xrates != NULL)
    {
        KASSERT(sp->xrates[1] <= WIFINET_RATE_MAXSIZE, ("xrate set too large: %u", sp->xrates[1]));
        memcpy(ise->SI_exrates, sp->xrates, 2+sp->xrates[1]);
    }
    else
    {
        ise->SI_exrates[1] = 0;
    }

    WIFINET_ADDR_COPY(ise->SI_bssid, wh->i_addr3);

    if (se->LastUpdateTime == 0)
        se->se_avgrssi = rssi;
    else
        se->se_avgrssi = (se->se_avgrssi + rssi*3)>>2;
    se->scaninfo.SI_rssi = se->se_avgrssi;
    memcpy(ise->SI_tstamp.data, sp->tstamp, sizeof(ise->SI_tstamp));


    ise->SI_intval = sp->bintval;
    ise->SI_capinfo = sp->capinfo;
    ise->SI_chan = apchan;
    ise->SI_erp = sp->erp;
    ise->SI_timoff = sp->timoff;
    if (sp->tim != NULL)
    {
        ise->SI_dtimperiod =  ((const struct wifi_mac_tim_ie *) sp->tim)->tim_period;
    }

#ifdef CONFIG_WAPI
    saveie(&ise->SI_wai_ie[0], sp->wai);
#endif //#ifdef CONFIG_WAPI

#ifdef CONFIG_P2P
    for (index = 0; index < MAX_P2PIE_NUM; index++)
    {
        if (sp->p2p[index] != NULL) {
            saveie(&ise->SI_p2p_ie[index][0], sp->p2p[index]);
        }
    }
#endif //#ifdef CONFIG_P2P

#ifdef CONFIG_WFD
    saveie(&ise->SI_wfd_ie[0], sp->wfd);
#endif //#ifdef CONFIG_WFD

    saveie(&ise->SI_wme_ie[0], sp->wme);
    saveie(&ise->SI_wpa_ie[0], sp->wpa);
    saveie(&ise->SI_htcap_ie[0], sp->htcap);
    saveie(&ise->SI_htinfo_ie[0], sp->htinfo);
    saveie(&ise->SI_country_ie[0], sp->country);
    saveie(&ise->SI_rsn_ie[0], sp->rsn);
    saveie(&ise->SI_wps_ie[0], sp->wps);
    if (se->connectcnt && ((jiffies - se->ConnectTime) > WIFINET_CONNECT_CNT_AGE*HZ))
    {
        se->connectcnt = 0;
        WIFINET_DPRINTF_MACADDR( AML_DEBUG_CONNECT, macaddr,
            "%s: fails %u HZ %d 0x%lx", __func__, se->connectcnt,HZ,jiffies);
    }

    se->LastUpdateTime = jiffies;
    se->se_notseen = 0;

    saveie(ise->ie_vht_cap, sp->vht_cap);
    saveie(ise->ie_vht_opt, sp->vht_opt);
    saveie(ise->ie_vht_tx_pwr, sp->vht_tx_pwr);
    saveie(ise->ie_vht_ch_sw, sp->vht_ch_sw);
    saveie(ise->ie_vht_ext_bss_ld, sp->vht_ext_bss_ld);
    saveie(ise->ie_vht_quiet_ch, sp->vht_quiet_ch);
    saveie(ise->ie_vht_opt_md_ntf, sp->vht_opt_md_ntf);

    hash = STA_HASH(macaddr);

    WIFI_SCAN_SE_LIST_LOCK(st);
    list_add(&se->se_hash, &st->st_hash[hash]);
    list_add_tail(&se->se_list, &st->st_entry);
    WIFI_SCAN_SE_LIST_UNLOCK(st);
    return ;
}

static void quiet_intf (struct wlan_net_vif *wnet_vif, unsigned char enable)
{
    unsigned int qlen_real = WIFINET_SAVEQ_QLEN(&wnet_vif->vm_tx_buffer_queue);
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;

    if (wnet_vif->vm_opmode == WIFINET_M_STA) {
        if (wnet_vif->vm_state == WIFINET_S_CONNECTED) {
            DPRINTF(AML_DEBUG_PWR_SAVE, "%s, vid:%d, enable:%d, qlen_real:%d\n", __func__, wnet_vif->wnet_vif_id, enable, qlen_real);

            if (enable) {
                wnet_vif->vm_pstxqueue_flags |= WIFINET_PSQUEUE_PS4QUIET;
                wifimac->drv_priv->drv_ops.drv_set_is_mother_channel(wifimac->drv_priv, wnet_vif->wnet_vif_id, 0);
                wifi_mac_pwrsave_send_nulldata(wnet_vif->vm_mainsta, NULLDATA_PS, 1);

            } else {
                wnet_vif->vm_pstxqueue_flags &= ~WIFINET_PSQUEUE_PS4QUIET;

                if (qlen_real == 0) {
                    wifi_mac_pwrsave_send_nulldata(wnet_vif->vm_mainsta, NULLDATA_NONPS, 0);

                } else {
                    wifi_mac_buffer_txq_send_pre(wnet_vif);
                }
            }

        } else {
            wnet_vif->vm_pstxqueue_flags &= ~WIFINET_PSQUEUE_PS4QUIET;
        }
    }
}

void quiet_all_intf (struct wifi_mac *wifimac, unsigned char enable)
{
    struct wlan_net_vif *tmpwnet_vif = NULL, *tmpwnet_vif_next = NULL;

    list_for_each_entry_safe(tmpwnet_vif,tmpwnet_vif_next, &wifimac->wm_wnet_vifs, vm_next)
    {
        quiet_intf(tmpwnet_vif, enable);
    }
}

void wifi_mac_scan_notify_leave_or_back(struct wlan_net_vif *wnet_vif, unsigned char enable) {
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;

    if ((wifimac->wm_nrunning == 1)
        || ((wifimac->wm_nrunning == 2) && concurrent_check_is_vmac_same_pri_channel(wifimac))) {
        quiet_all_intf(wifimac, enable);

    } else if (wifimac->wm_nrunning == 2) {
        quiet_intf(wnet_vif, enable);
    }
}

void wifi_mac_set_scan_time(struct wlan_net_vif *wnet_vif) {
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    if (wnet_vif->vm_scan_before_connect_flag) {
        ss->scan_chan_wait = wnet_vif->vm_scan_time_before_connect;

    } else if (wifimac->wm_nrunning) {
        ss->scan_chan_wait = wnet_vif->vm_scan_time_connect;

    } else {
        ss->scan_chan_wait = wnet_vif->vm_scan_time_idle;
    }

    if (wnet_vif->vm_chan_switch_scan_flag) {
        ss->scan_chan_wait = wnet_vif->vm_scan_time_chan_switch;
    }
    //printk("%s change scan time to:%d\n", __func__, ss->scan_chan_wait);
    return;
}

static int vm_scan_setup_chan(struct wifi_mac_scan_state *ss, struct wlan_net_vif *wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_channel *c;
    int i=0;

#ifdef CONFIG_P2P
    struct wlan_net_vif *tmpwnet_vif;
    tmpwnet_vif = wifi_mac_get_wnet_vif_by_vid(wifimac, NET80211_P2P_VMAC);
#endif

    ss->scan_last_chan_index = 0;
    DPRINTF(AML_DEBUG_SCAN|AML_DEBUG_P2P, "%s %d wifimac->wm_nchans=%d \n",
        __func__, __LINE__, wifimac->wm_nchans);

    if (wnet_vif->vm_scan_before_connect_flag) {
        ss->scan_last_chan_index = wnet_vif->vm_connchan.num;

        for (i = 0; i < ss->scan_last_chan_index; i++) {
            ss->ss_chans[i] = wnet_vif->vm_connchan.conn_chan[i];
        }
    } else if (wnet_vif->vm_chan_switch_scan_flag) {
        ss->scan_last_chan_index = 1;
        ss->ss_chans[0] = wnet_vif->vm_switchchan;
    } else {
        WIFI_CHANNEL_LOCK(wifimac);
        for (i = 0; i < wifimac->wm_nchans; i++)
        {
            c = &wifimac->wm_channels[i];
            /* both 2.4G and 5G set WIFINET_BW_20 flag.*/
            if (c->chan_bw == WIFINET_BWC_WIDTH20)
            {
    #ifdef CONFIG_P2P
                if ((wnet_vif->vm_p2p != NULL) && (wnet_vif->vm_p2p->p2p_enable)
                    && (wnet_vif->vm_p2p->social_channel))
                {
                    if ((c->chan_cfreq1 != SOCIAL_CHAN_1)
                       && (c->chan_cfreq1 != SOCIAL_CHAN_2)
                       && (c->chan_cfreq1 != SOCIAL_CHAN_3))
                    {
                        continue;
                    }
                    ss->ss_chans[ss->scan_last_chan_index++] = c;
                }
                else
    #endif//CONFIG_P2P
                {
                    ss->ss_chans[ss->scan_last_chan_index++] = c;
                    DPRINTF(AML_DEBUG_SCAN,"%s %d chan_index=%d,chan_pri_num=%d,flag=%x,freq %d %p\n",
                        __func__,__LINE__,i,c->chan_pri_num,c->chan_flags,c->chan_cfreq1,c);
                }
            }
        }
        WIFI_CHANNEL_UNLOCK(wifimac);
    }

    wifi_mac_set_scan_time(wnet_vif);
    DPRINTF(AML_DEBUG_SCAN, "%s vid:%d ss->scan_next_chan_index=%d \
        ss->scan_last_chan_index=%d, ss->scan_chan_wait=0x%xms, HZ = %d LINUX_VERSION_CODE =%x\n",
        __FUNCTION__, wnet_vif->wnet_vif_id, ss->scan_next_chan_index, ss->scan_last_chan_index,
        ss->scan_chan_wait, HZ, LINUX_VERSION_CODE);

    return 0;
}

#ifdef FW_RF_CALIBRATION
static void
wifi_mac_scan_send_probe_timeout(SYS_TYPE param1,SYS_TYPE param2,
    SYS_TYPE param3,SYS_TYPE param4,SYS_TYPE param5)
{
    struct wifi_mac_scan_state *ss = (struct wifi_mac_scan_state *)param1;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    unsigned char i;
    unsigned char j;

    os_timer_ex_cancel(&ss->ss_probe_timer, CANCEL_SLEEP);
    //printk("%s, ss->scan_StateFlags:%08x, ss->ss_nssid:%d\n", __func__, ss->scan_StateFlags, ss->ss_nssid);

    if (ss->scan_CfgFlags & WIFINET_SCANCFG_ACTIVE)
    {
        struct net_device *dev = wnet_vif->vm_ndev;

        for (i = 0; i < ss->ss_nssid; i++)
            for (j = 0; j < 2; ++j)
                wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, dev->broadcast,
                    dev->broadcast, ss->ss_ssid[i].ssid, ss->ss_ssid[i].len, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);

        if (!ss->ss_nssid && !wnet_vif->vm_p2p->p2p_enable && !wnet_vif->vm_scan_before_connect_flag) {
            wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, dev->broadcast,
                dev->broadcast, ss->ss_ssid[i].ssid, ss->ss_ssid[i].len, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
        }
    }
}

int wifi_mac_scan_send_probe_timeout_ex(void *arg)
{
    struct wifi_mac_scan_state *ss = (struct wifi_mac_scan_state *) arg;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;

    //printk("%s, ss->scan_StateFlags:%08x\n", __func__, ss->scan_StateFlags);

    WIFI_SCAN_LOCK(ss);
    if (ss->scan_StateFlags & SCANSTATE_F_SEND_PROBEREQ_AGAIN) {
        ss->scan_StateFlags &= ~SCANSTATE_F_SEND_PROBEREQ_AGAIN;
        wifi_mac_add_work_task(wifimac, wifi_mac_scan_send_probe_timeout, NULL,
            (SYS_TYPE)arg, (SYS_TYPE)wnet_vif, 0, 0, 0);

        DPRINTF(AML_DEBUG_SCAN, "%s %d ss->scan_next_chan_index = %d, ss->scan_StateFlags:%08x\n",
            __func__,__LINE__,ss->scan_next_chan_index, ss->scan_StateFlags);
    }
    WIFI_SCAN_UNLOCK(ss);

    return OS_TIMER_NOT_REARMED;
}

void wifi_mac_scan_channel(struct wifi_mac *wifimac)
{
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    unsigned char i = 0;
    struct wifi_channel *chan;
    enum wifi_mac_macmode last_mac_mode;

    //printk("%s, ss->scan_StateFlags:%08x\n", __func__, ss->scan_StateFlags);
    if (!(ss->scan_StateFlags & SCANSTATE_F_CHANNEL_SWITCH_COMPLETE)) {
        return;
    }
    ss->scan_StateFlags &= ~SCANSTATE_F_WAIT_CHANNEL_SWITCH;
    ss->scan_StateFlags &= ~SCANSTATE_F_CHANNEL_SWITCH_COMPLETE;
    ss->scan_StateFlags &= ~SCANSTATE_F_DISCARD;

    if (!((wifimac->wm_nrunning == 2) && (!concurrent_check_is_vmac_same_pri_channel(wifimac)))) {
        os_timer_ex_start_period(&ss->ss_scan_timer, ss->scan_chan_wait);
    }

    if ((wifimac->wm_nrunning == 1)
        || ((wifimac->wm_nrunning == 2) && (concurrent_check_is_vmac_same_pri_channel(wifimac)))) {
        ss->scan_StateFlags |= SCANSTATE_F_RESTORE;
        ss->scan_StateFlags &= ~SCANSTATE_F_NOTIFY_AP;
    }

    chan = ss->ss_chans[ss->scan_next_chan_index++];
    /* vm_mac_mode default is 11GNAC */
    last_mac_mode = wnet_vif->vm_mac_mode;
    if (chan->chan_pri_num >= 1 && chan->chan_pri_num <= 14) {
        if (wnet_vif->vm_p2p->p2p_enable) {
            wnet_vif->vm_mac_mode = WIFINET_MODE_11GN;
        } else {
            wnet_vif->vm_mac_mode = WIFINET_MODE_11BGN;
        }

    } else {
        wnet_vif->vm_mac_mode = WIFINET_MODE_11GNAC;
    }

    if (last_mac_mode != wnet_vif->vm_mac_mode)
        wifi_mac_set_legacy_rates(&wnet_vif->vm_legacy_rates, wnet_vif);

    if (ss->scan_CfgFlags & WIFINET_SCANCFG_ACTIVE)
    {
        struct net_device *dev = wnet_vif->vm_ndev;

        DPRINTF(AML_DEBUG_SCAN, "%s vid:%d, next_chan_index = %d, chan=%d freq=%d, p2p_enable:%d\n", __func__,
            wnet_vif->wnet_vif_id, ss->scan_next_chan_index, chan->chan_pri_num, chan->chan_cfreq1, wnet_vif->vm_p2p->p2p_enable);

        if (!wnet_vif->vm_p2p->p2p_enable && !wnet_vif->vm_scan_before_connect_flag && !wnet_vif->vm_chan_switch_scan_flag) {
             wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, dev->broadcast,
                 dev->broadcast, "", 0, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
        }

        for (i = 0; i < ss->ss_nssid; i++) {
            if (wnet_vif->vm_scan_before_connect_flag) {
                wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, wnet_vif->vm_connchan.da,
                    wnet_vif->vm_connchan.bssid, ss->ss_ssid[i].ssid, ss->ss_ssid[i].len, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
            } else if (wnet_vif->vm_chan_switch_scan_flag) {
                wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, wnet_vif->vm_des_bssid,
                    wnet_vif->vm_des_bssid, ss->ss_ssid[i].ssid, ss->ss_ssid[i].len, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
            } else {
                wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, dev->broadcast,
                    dev->broadcast, ss->ss_ssid[i].ssid, ss->ss_ssid[i].len, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
            }
        }

        ss->scan_StateFlags |= SCANSTATE_F_SEND_PROBEREQ_AGAIN;
        if (ss->scan_chan_wait == wnet_vif->vm_scan_time_connect) {
            os_timer_ex_start_period(&ss->ss_probe_timer, 8);

        } else {
            os_timer_ex_start_period(&ss->ss_probe_timer, 20);
        }
    }

    DPRINTF(AML_DEBUG_SCAN, "%s OS_SET_TIMER = %d next_chn\n", __func__, ss->scan_chan_wait);
}

void wifi_mac_switch_scan_channel(struct wifi_mac *wifimac)
{
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct wifi_channel *chan;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;

    //printk("%s, ss->scan_StateFlags:%08x\n", __func__, ss->scan_StateFlags);
    if (ss->scan_next_chan_index >= ss->scan_last_chan_index) {
        DPRINTF(AML_DEBUG_ERROR, " %s (scan_next_chan_index >= scan_last_chan_index) drop!!!\n", __func__);
        return;
    }

    chan = ss->ss_chans[ss->scan_next_chan_index];
    if ((wifimac->wm_curchan != NULL) && (chan->chan_pri_num == wifimac->wm_curchan->chan_pri_num)) {
        ss->scan_StateFlags |= SCANSTATE_F_CHANNEL_SWITCH_COMPLETE;
        wifi_mac_scan_channel(wifimac);

    } else {
        ss->scan_StateFlags |= SCANSTATE_F_WAIT_CHANNEL_SWITCH;
        wifi_mac_ChangeChannel(wifimac, chan, 0, wnet_vif->wnet_vif_id);
        //os_timer_ex_start_period(&ss->ss_scan_timer, 30);
    }
}
#endif

void scan_next_chan(struct wifi_mac *wifimac)
{
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    unsigned char i = 0;
    unsigned char j = 0;
    struct wifi_channel *chan;
    enum wifi_mac_macmode last_mac_mode;
    unsigned int send_packet_num = 2;

    if (ss->scan_next_chan_index >= ss->scan_last_chan_index)
    {
        DPRINTF(AML_DEBUG_ERROR, " %s (scan_next_chan_index >= scan_last_chan_index) drop!!!\n", __func__);
        return;
    }

    if (!((wifimac->wm_nrunning == 2) && (!concurrent_check_is_vmac_same_pri_channel(wifimac)))) {
        //printk("%s\n", __func__);
        os_timer_ex_start_period(&ss->ss_scan_timer, ss->scan_chan_wait);
    }

    if ((wifimac->wm_nrunning == 1)
        || ((wifimac->wm_nrunning == 2) && (concurrent_check_is_vmac_same_pri_channel(wifimac)))) {
        ss->scan_StateFlags |= SCANSTATE_F_RESTORE;
        ss->scan_StateFlags &= ~SCANSTATE_F_NOTIFY_AP;
    }
    chan = ss->ss_chans[ss->scan_next_chan_index++];
    wifi_mac_ChangeChannel(wifimac, chan, 0, wnet_vif->wnet_vif_id);

    ss->scan_StateFlags &= ~SCANSTATE_F_DISCARD;
    /* vm_mac_mode default is 11GNAC */
    last_mac_mode = wnet_vif->vm_mac_mode;
    if (chan->chan_pri_num >= 1 && chan->chan_pri_num <= 14) {
        if (wnet_vif->vm_p2p->p2p_enable) {
            wnet_vif->vm_mac_mode = WIFINET_MODE_11GN;
        } else {
            wnet_vif->vm_mac_mode = WIFINET_MODE_11BGN;
        }

    } else {
        wnet_vif->vm_mac_mode = WIFINET_MODE_11GNAC;
    }

    if (last_mac_mode != wnet_vif->vm_mac_mode)
        wifi_mac_set_legacy_rates(&wnet_vif->vm_legacy_rates, wnet_vif);

    if (ss->scan_CfgFlags & WIFINET_SCANCFG_ACTIVE)
    {
        struct net_device *dev = wnet_vif->vm_ndev;

        DPRINTF(AML_DEBUG_SCAN, "%s vid:%d, next_chan_index = %d, chan=%d freq=%d, p2p_enable:%d\n", __func__,
            wnet_vif->wnet_vif_id, ss->scan_next_chan_index, chan->chan_pri_num, chan->chan_cfreq1, wnet_vif->vm_p2p->p2p_enable);

        if (!wnet_vif->vm_p2p->p2p_enable) {
            for (i = 0; i < send_packet_num; i++)
                wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, dev->broadcast,
                    dev->broadcast, "", 0, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
            send_packet_num = 1;
        }

        for (i = 0; i < ss->ss_nssid; i++)
            for (j = 0; j < send_packet_num; j++)
                wifi_mac_send_probereq(wnet_vif->vm_mainsta, wnet_vif->vm_myaddr, dev->broadcast,
                    dev->broadcast, ss->ss_ssid[i].ssid, ss->ss_ssid[i].len, wnet_vif->vm_opt_ie, wnet_vif->vm_opt_ie_len);
    }

    DPRINTF(AML_DEBUG_SCAN, "%s OS_SET_TIMER = %d next_chn\n", __func__, ss->scan_chan_wait);
}

static void
wifi_mac_scan_timeout(SYS_TYPE param1,SYS_TYPE param2,
    SYS_TYPE param3,SYS_TYPE param4,SYS_TYPE param5)
{
    struct wifi_mac_scan_state *ss = (struct wifi_mac_scan_state *) param1;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    unsigned char scandone;
    unsigned char need_notify_ap = 1;
    struct wifi_channel *chan = WIFINET_CHAN_ERR;
    struct drv_private *drv_priv = wifimac->drv_priv;

    os_timer_ex_cancel(&ss->ss_scan_timer, CANCEL_SLEEP);

    //check replayer count
    if (wifimac->wm_scanplayercnt != (unsigned long)param5) {
        DPRINTF(AML_DEBUG_WARNING, "%s wm_scanplayercnt %ld, ignore... \n", __func__, wifimac->wm_scanplayercnt);
        return;
    }

    if (!wnet_vif->vm_mainsta) {
        DPRINTF(AML_DEBUG_WARNING, "%s vid:%d vm_mainsta is NULL, cancel scan\n", __func__, wnet_vif->wnet_vif_id);
        ss->scan_StateFlags |= SCANSTATE_F_CANCEL;
    }

    if (!(wifimac->wm_flags & WIFINET_F_SCAN)) {
        DPRINTF(AML_DEBUG_WARNING, "%s %d drop wm_flags  0x%x \n",__func__,__LINE__,wifimac->wm_flags );
        goto end;
    }

    if (ss->scan_StateFlags & SCANSTATE_F_DISCONNECT_REQ_CANCEL) {
        DPRINTF(AML_DEBUG_WARNING, "%s end scan due to disconnect, no need to restore channel\n", __func__);
        goto end;
    }

    if (!(ss->scan_StateFlags & SCANSTATE_F_START)) {
        DPRINTF(AML_DEBUG_WARNING, "%s SCANSTATE_F_START flag not set, end scan\n", __func__);
        goto end;
    }

    scandone = (ss->scan_next_chan_index >= ss->scan_last_chan_index) ||
        (ss->scan_StateFlags & SCANSTATE_F_CANCEL);

    DPRINTF(AML_DEBUG_SCAN,"%s(%d) nxt chn 0x%x, lst chn 0x%x ss_flag 0x%x scandone %d\n", __func__, __LINE__,
        ss->scan_next_chan_index, ss->scan_last_chan_index, ss->scan_StateFlags, scandone);

    if (ss->scan_next_chan_index < ss->scan_last_chan_index) {
        chan = ss->ss_chans[ss->scan_next_chan_index];
        if ((chan != WIFINET_CHAN_ERR) && (wifimac->wm_curchan != WIFINET_CHAN_ERR)
            && (chan->chan_pri_num == wifimac->wm_curchan->chan_pri_num) && (wifimac->wm_curchan->chan_bw == 0)) {
            need_notify_ap = 0;
        }
    }

    //printk("%s, ss->scan_StateFlags:%08x, scandone:%d, need_notify_ap:%d\n", __func__, ss->scan_StateFlags, scandone, need_notify_ap);
    if (wifimac->wm_nrunning != 0) {
        if (ss->scan_StateFlags & SCANSTATE_F_RESTORE) {
            wifi_mac_restore_wnet_vif_channel(wnet_vif);
            ss->scan_StateFlags &= ~SCANSTATE_F_RESTORE;
            ss->scan_StateFlags |= SCANSTATE_F_DISCARD;

            if (scandone) {
                DPRINTF(AML_DEBUG_WARNING, "%s scandone, end scan\n", __func__);
                goto end;
            }

            ss->scan_StateFlags |= SCANSTATE_F_WAIT_TBTT;
            os_timer_ex_start_period(&ss->ss_scan_timer, WIFINET_SCAN_DEFAULT_INTERVAL);
            return;

        } else if  (!scandone && need_notify_ap) {
            struct wlan_net_vif *connect_wnet = wifi_mac_running_wnet_vif(wifimac);

            if (!((connect_wnet != NULL) && (connect_wnet->vm_opmode == WIFINET_M_HOSTAP))) {
                if (!(ss->scan_StateFlags & SCANSTATE_F_NOTIFY_AP)) {
                    wifi_mac_scan_notify_leave_or_back(wnet_vif, 1);
                    ss->scan_StateFlags |= SCANSTATE_F_NOTIFY_AP;
                    os_timer_ex_start_period(&wifimac->wm_scan->ss_scan_timer, 10);
                    return;

                } else {
                    if (!drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
                        ss->scan_StateFlags |= SCANSTATE_F_WAIT_PKT_CLEAR;
                        return;
                    }
                }
            } else {
                //add NOA in the near future
            }
        }
    }

    DPRINTF(AML_DEBUG_SCAN,"%s done 0x%x, ss_flag 0x%x\n", __func__, scandone, ss->scan_StateFlags);
    if (!scandone) {
        #ifndef FW_RF_CALIBRATION
            scan_next_chan(wifimac);
            return;
        #else
            if (ss->scan_StateFlags & SCANSTATE_F_CHANNEL_SWITCH_COMPLETE) {
                wifi_mac_scan_channel(wifimac);

            } else {
                wifi_mac_switch_scan_channel(wifimac);
            }
            return;
        #endif
    }

end:
    wifi_mac_end_scan(ss);
    return;
}

int wifi_mac_scan_timeout_ex(void *arg)
{
    struct wifi_mac_scan_state *ss = (struct wifi_mac_scan_state *) arg;
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;

    //printk("%s, ss->scan_StateFlags:%08x\n", __func__, ss->scan_StateFlags);

    WIFI_SCAN_LOCK(ss);
    if (ss->scan_StateFlags & SCANSTATE_F_WAIT_TBTT) {
        ss->scan_StateFlags &= ~SCANSTATE_F_WAIT_TBTT;
    }

    if (ss->scan_StateFlags & SCANSTATE_F_WAIT_CHANNEL_SWITCH) {
        ss->scan_StateFlags &= ~SCANSTATE_F_WAIT_CHANNEL_SWITCH;
        ss->scan_StateFlags |= SCANSTATE_F_CHANNEL_SWITCH_COMPLETE;
    }

    if (ss->scan_StateFlags & SCANSTATE_F_SEND_PROBEREQ_AGAIN) {
        ss->scan_StateFlags &= ~SCANSTATE_F_SEND_PROBEREQ_AGAIN;
    }

    wifimac->wm_scanplayercnt++;
    wifi_mac_add_work_task(wifimac, wifi_mac_scan_timeout, NULL,
        (SYS_TYPE)arg, (SYS_TYPE)wnet_vif, 0, 0, (SYS_TYPE)wifimac->wm_scanplayercnt);
    WIFI_SCAN_UNLOCK(ss);

    DPRINTF(AML_DEBUG_SCAN, "%s %d ss->scan_next_chan_index = %d, ss->scan_StateFlags:%08x\n",
        __func__,__LINE__,ss->scan_next_chan_index, ss->scan_StateFlags);

    return OS_TIMER_NOT_REARMED;
}

static void
scan_start_task(SYS_TYPE param1,SYS_TYPE param2,
    SYS_TYPE param3,SYS_TYPE param4,SYS_TYPE param5)
{
    struct wlan_net_vif *wnet_vif = (struct wlan_net_vif *)param4;
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    WIFI_SCAN_LOCK(ss);
    if ((wnet_vif->wnet_vif_replaycounter != (int)param5) || (wifimac->wm_scanplayercnt != (unsigned long)param3)
        || (ss->scan_StateFlags & SCANSTATE_F_START) || !(wifimac->wm_flags & WIFINET_F_SCAN)) {
        printk("%s scan_StateFlags:%04x, wm_scanplayercnt:%ld\n", __func__, ss->scan_StateFlags, wifimac->wm_scanplayercnt);
        WIFI_SCAN_UNLOCK(ss);
        return;
    }

    ss->scan_StateFlags = 0;
    ss->scan_StateFlags |= SCANSTATE_F_START;
    WIFI_SCAN_UNLOCK(ss);
    os_timer_ex_cancel(&ss->ss_scan_timer, CANCEL_SLEEP);

    if (!wnet_vif->vm_mainsta) {
        DPRINTF(AML_DEBUG_WARNING, "%s vid:%d vm_mainsta is NULL, alloc new sta\n", __func__, wnet_vif->wnet_vif_id);

        /*sta side allocate a sta buffer to save ap capabilities and information */
        wnet_vif->vm_mainsta = wifi_mac_get_sta_node(&wnet_vif->vm_sta_tbl, wnet_vif, wnet_vif->vm_myaddr);
        if (wnet_vif->vm_mainsta == NULL)
        {
            DPRINTF(AML_DEBUG_ERROR, "ERROR::%s %d alloc sta FAIL!!! \n",__func__,__LINE__);
            return;
        }
    }

    if ((ss->scan_CfgFlags & WIFINET_SCANCFG_CHANSET) == 0)
    {
        DPRINTF(AML_DEBUG_SCAN, "%s(%d)\n", __func__, __LINE__);
        vm_scan_setup_chan(ss,wnet_vif);
    }
    wifimac->drv_priv->stop_noa_flag = 0;
    wifi_mac_scan_start(wifimac);

    //set up scan channel index to 0
    ss->scan_next_chan_index = 0;
    printk("%s wm_nrunning:%d\n", __func__, wifimac->wm_nrunning);

    if (wifimac->wm_nrunning == 0) {
        wifi_mac_scan_timeout_ex(ss);

    } else if (wifimac->wm_nrunning == 1) {

        ss->scan_StateFlags |= SCANSTATE_F_WAIT_TBTT;
        os_timer_ex_start_period(&ss->ss_scan_timer, 110);
    }

    return;
}

void wifi_mac_check_switch_chan_result(struct wlan_net_vif * wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct scaninfo_table *st = ss->ScanTablePriv;
    struct scaninfo_entry *se = NULL, *SI_next = NULL;
    unsigned char find = 0;

    WIFI_SCAN_SE_LIST_LOCK(st);
    list_for_each_entry_safe(se,SI_next,&st->st_entry,se_list)
    {
        se->scaninfo.SI_age = jiffies - se->LastUpdateTime;
        if (!strcmp(se->scaninfo.SI_ssid + 2, wnet_vif->vm_des_ssid[0].ssid)
            && WIFINET_ADDR_EQ(wnet_vif->vm_des_bssid, se->scaninfo.SI_bssid)
            && wnet_vif->vm_switchchan->chan_pri_num == se->scaninfo.SI_chan->chan_pri_num) {
            find = 1;
        }
    }
    WIFI_SCAN_SE_LIST_UNLOCK(st);
    if (find == 1 || wnet_vif->vm_chan_switch_scan_count == 5) {
        wnet_vif->vm_mainsta->sta_channel_switch_mode = 0;
        wnet_vif->vm_chan_switch_scan_count = 0;
        wnet_vif->vm_chan_switch_scan_flag = 0;
    }
}

void wifi_mac_end_scan( struct wifi_mac_scan_state *ss)
{
    struct wlan_net_vif *wnet_vif = ss->VMacPriv;
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;

    if (!(wifimac->wm_flags & WIFINET_F_SCAN)) {
        return;
    }

    os_timer_ex_cancel(&ss->ss_scan_timer, CANCEL_SLEEP);
    DPRINTF(AML_DEBUG_SCAN, "%s chan_index = %d ,scan_CfgFlags 0x%x, wifimac->wm_nrunning is:%d\n",
        __func__, ss->scan_next_chan_index, ss->scan_CfgFlags, wifimac->wm_nrunning);

    if (wifimac->wm_nrunning == 1) {
        struct wlan_net_vif *connect_wnet = wifi_mac_running_wnet_vif(wifimac);
        if (connect_wnet->vm_opmode == WIFINET_M_HOSTAP &&
            connect_wnet->vm_p2p->noa_app_ie[WIFINET_APPIE_FRAME_BEACON].length) {
            printk("%s,%d, clear noa ie\n", __func__, __LINE__);
            connect_wnet->vm_p2p->ap_mode_set_noa_enable = 0;
            wifi_mac_rm_app_ie(&connect_wnet->vm_p2p->noa_app_ie[WIFINET_APPIE_FRAME_BEACON]);
            vm_p2p_update_beacon_app_ie(connect_wnet);
            wifimac->drv_priv->stop_noa_flag = 1;
            if (P2P_NoA_START_FLAG(connect_wnet->vm_p2p->HiP2pNoaCountNow)) {
                p2p_noa_start_irq(connect_wnet->vm_p2p, wifimac->drv_priv);
            }
        }
    } else {
        wifimac->drv_priv->stop_noa_flag = 0;
    }

    wifi_mac_scan_end(wifimac);
    wifimac->wm_lastscan = jiffies;
    ss->scan_StateFlags = 0;

    /* restore mac mode after scanning */
    if (wnet_vif->vm_mainsta != NULL) {
        wnet_vif->vm_mac_mode = wnet_vif->vm_mainsta->sta_bssmode;
        wifi_mac_set_legacy_rates(&wnet_vif->vm_legacy_rates, wnet_vif);
    }

    if ((wnet_vif->vm_opmode != WIFINET_M_HOSTAP)
       && (wnet_vif->vm_opmode != WIFINET_M_P2P_GO)
       && (wnet_vif->vm_opmode != WIFINET_M_WDS)
       && (wnet_vif->vm_opmode != WIFINET_M_MONITOR)) {
        wifi_mac_scan_connect(ss, wnet_vif);
    } else if ((wnet_vif->vm_opmode == WIFINET_M_HOSTAP)
                ||(wnet_vif->vm_opmode == WIFINET_M_P2P_GO)) {
        DPRINTF(AML_DEBUG_SCAN|AML_DEBUG_P2P,
                "%s(%d) vm_opmode %d scan_CfgFlags 0x%x\n",
                __func__,__LINE__,wnet_vif->vm_opmode,ss->scan_CfgFlags);
        if (ss->scan_CfgFlags  & WIFINET_SCANCFG_CREATE) {
            wifi_mac_chk_ap_chan(ss, wnet_vif);
        }
    }

    /*upload scanning result and notify scanning done to upper
    after processing scan result. */
    if (ss->scan_CfgFlags & WIFINET_SCANCFG_USERREQ) {
        wifi_mac_notify_scan_done(wnet_vif);
        ss->scan_CfgFlags &= (~WIFINET_SCANCFG_USERREQ);
    }

    wifi_mac_restore_wnet_vif_channel(wnet_vif);
    wifimac->wm_flags &= ~WIFINET_F_SCAN;
    ss->scan_CfgFlags = 0;
    wifimac->wm_p2p_connection_protect = 0;
    wnet_vif->vm_pstxqueue_flags &= ~WIFINET_PSQUEUE_PS4QUIET;
    wnet_vif->vm_scan_before_connect_flag = 0;
    if (wnet_vif->vm_chan_switch_scan_flag) {
        wifi_mac_check_switch_chan_result(wnet_vif);
        if (wnet_vif->vm_mainsta->sta_channel_switch_mode) {
            wifi_mac_start_scan(wnet_vif, WIFINET_SCANCFG_USERREQ | WIFINET_SCANCFG_ACTIVE | WIFINET_SCANCFG_FLUSH |
                                WIFINET_SCANCFG_CREATE, wnet_vif->vm_des_nssid, wnet_vif->vm_des_ssid);
            wnet_vif->vm_chan_switch_scan_count++;
        }
    }

    printk("%s---> scan finish, vid:%d, clean vm_flags 0x%x\n", __func__, wnet_vif->wnet_vif_id, wifimac->wm_flags);

    if (1) {
        struct wlan_net_vif *wnet_vif_next = NULL, *wnet_vif_tmp = NULL;
        list_for_each_entry_safe(wnet_vif_tmp, wnet_vif_next, &wifimac->wm_wnet_vifs, vm_next)
        {
            WIFINET_PWRSAVE_LOCK(wnet_vif);
            wnet_vif_tmp->vm_pwrsave.ips_state = WIFINET_PWRSAVE_AWAKE;
            WIFINET_PWRSAVE_UNLOCK(wnet_vif);
        }
    }
    os_timer_ex_start_period(&wnet_vif->vm_pwrsave.ips_timer_presleep, wnet_vif->vm_pwrsave.ips_inactivitytime);
}

void wifi_mac_notify_ap_success(struct wlan_net_vif *wnet_vif) {
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct drv_private *drv_priv = wifimac->drv_priv;
    struct wlan_net_vif *p2p_vmac = drv_priv->drv_wnet_vif_table[NET80211_P2P_VMAC];

    DPRINTF(AML_DEBUG_SCAN, "%s vid:%d\n", __func__, wnet_vif->wnet_vif_id);

    //scan branch
    if ((wifimac->wm_nrunning > 0) && (wifimac->wm_flags & WIFINET_F_SCAN)
        && (wifimac->wm_scan->scan_StateFlags & SCANSTATE_F_NOTIFY_AP)) {
        if (drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
            os_timer_ex_cancel(&wifimac->wm_scan->ss_scan_timer, 1);
            wifi_mac_scan_timeout_ex(wifimac->wm_scan);

        } else {
            //printk("still have pkt in hal, wait\n");
            wifimac->wm_scan->scan_StateFlags |= SCANSTATE_F_NOTIFY_AP_SUCCESS;
        }
    }

    //p2p branch
    if ((wifimac->wm_nrunning == 1)
        && (p2p_vmac->vm_p2p->p2p_flag & P2P_WAIT_SWITCH_CHANNEL)) {
        if (drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
            wifi_mac_ChangeChannel(wifimac, p2p_vmac->vm_p2p->work_channel, 0, p2p_vmac->wnet_vif_id);
            p2p_vmac->vm_p2p->p2p_flag &= ~P2P_WAIT_SWITCH_CHANNEL;

        } else {
            //printk("still have pkt in hal, wait\n");
            p2p_vmac->vm_p2p->p2p_flag &= ~P2P_WAIT_SWITCH_CHANNEL;
            p2p_vmac->vm_p2p->p2p_flag |= P2P_ALLOW_SWITCH_CHANNEL;
        }
    }

    //vsdb channel switch branch
    if (wifimac->wm_nrunning == 2) {
        #ifdef  CONFIG_CONCURRENT_MODE
            if (wifimac->wm_vsdb_flags & CONCURRENT_NOTIFY_AP) {
                wifimac->wm_vsdb_flags &= ~CONCURRENT_NOTIFY_AP;
                if (drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
                    concurrent_vsdb_do_channel_change(wifimac);

                } else {
                    //printk("still have pkt in hal, wait\n");
                    wifimac->wm_vsdb_flags |= CONCURRENT_NOTIFY_AP_SUCCESS;
                }
            }
        #endif
    }
}

void wifi_mac_notify_pkt_clear(struct wifi_mac *wifimac) {
    struct drv_private *drv_priv = wifimac->drv_priv;
    struct wlan_net_vif *p2p_vmac = drv_priv->drv_wnet_vif_table[NET80211_P2P_VMAC];

    DPRINTF(AML_DEBUG_SCAN, "%s\n", __func__);

    if ((wifimac->wm_nrunning > 0) && (wifimac->wm_flags & WIFINET_F_SCAN)
        && ((wifimac->wm_scan->scan_StateFlags & SCANSTATE_F_NOTIFY_AP_SUCCESS)
        || (wifimac->wm_scan->scan_StateFlags & SCANSTATE_F_WAIT_PKT_CLEAR))) {
        if (drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
            wifimac->wm_scan->scan_StateFlags &= ~SCANSTATE_F_NOTIFY_AP_SUCCESS;
            wifimac->wm_scan->scan_StateFlags &= ~SCANSTATE_F_WAIT_PKT_CLEAR;
            os_timer_ex_cancel(&wifimac->wm_scan->ss_scan_timer, 1);
            wifi_mac_scan_timeout_ex(wifimac->wm_scan);
        }
    }

    //p2p branch
    if ((wifimac->wm_nrunning == 1)
        && (p2p_vmac->vm_p2p->p2p_flag & P2P_ALLOW_SWITCH_CHANNEL)) {
        if (drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
            wifi_mac_ChangeChannel(wifimac, p2p_vmac->vm_p2p->work_channel, 0, p2p_vmac->wnet_vif_id);
            p2p_vmac->vm_p2p->p2p_flag &= ~P2P_ALLOW_SWITCH_CHANNEL;
        }
    }

    if (wifimac->wm_nrunning == 2) {
        #ifdef  CONFIG_CONCURRENT_MODE
            if (wifimac->wm_vsdb_flags & CONCURRENT_NOTIFY_AP_SUCCESS) {
                if (drv_priv->hal_priv->hal_ops.hal_tx_empty()) {
                    concurrent_vsdb_do_channel_change(wifimac);
                }
            }
        #endif
    }
}

void wifi_mac_cancel_scan(struct wlan_net_vif *wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    DPRINTF(AML_DEBUG_SCAN, "<%s> :  %s %d ++ vm_opmode %d\n",
        wnet_vif->vm_ndev->name,__func__,__LINE__,wnet_vif->vm_opmode);

    if (wifimac->wm_flags & WIFINET_F_SCAN)
    {
        ss->scan_StateFlags |= SCANSTATE_F_CANCEL;
        printk("<running> %s %d wnet_vif_id = %d\n",__func__,__LINE__,wnet_vif->wnet_vif_id);
        if (ss->scan_StateFlags & SCANSTATE_F_START)
        {
            os_timer_ex_cancel(&wifimac->wm_scan->ss_scan_timer, 1);
            wifi_mac_scan_timeout_ex(wifimac->wm_scan);
        }
    }
}

int vm_scan_user_set_chan(struct wlan_net_vif *wnet_vif,
    struct cfg80211_scan_request *request)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;
    struct wifi_channel *c;
    int i=0,j=0;

    ss->scan_last_chan_index = 0;
    DPRINTF(AML_DEBUG_SCAN, "%s %d wm_nchans=%d request_ch %d\n",
        __func__, __LINE__, wifimac->wm_nchans,request->n_channels);

    WIFI_CHANNEL_LOCK(wifimac);
    for (j = 0; j < request->n_channels; j++) {
        for (i = 0; i < wifimac->wm_nchans; i++) {
            c = &wifimac->wm_channels[i];

            if (c->chan_bw == WIFINET_BWC_WIDTH20) {
                if (wnet_vif->vm_p2p_support && wnet_vif->vm_p2p->social_channel) {
                    if ((c->chan_cfreq1 != SOCIAL_CHAN_1) && (c->chan_cfreq1 != SOCIAL_CHAN_2) && (c->chan_cfreq1 != SOCIAL_CHAN_3)) {
                        continue;
                    }
                }

                if (c->chan_cfreq1 != request->channels[j]->center_freq) {
                    continue;
                }

                ss->ss_chans[ss->scan_last_chan_index++] = c;
            }
        }
    }
    WIFI_CHANNEL_UNLOCK(wifimac);

    wifi_mac_set_scan_time(wnet_vif);
    DPRINTF(AML_DEBUG_SCAN, "%s vid:%d ss->scan_next_chan_index=%d \
        ss->scan_last_chan_index=%d, ss->scan_chan_wait=0x%xms, HZ = %d LINUX_VERSION_CODE =%x\n",
        __FUNCTION__, wnet_vif->wnet_vif_id, ss->scan_next_chan_index, ss->scan_last_chan_index,
        ss->scan_chan_wait, HZ, LINUX_VERSION_CODE);

    return 0;
}

int wifi_mac_start_scan(struct wlan_net_vif *wnet_vif, int flags,
    unsigned int nssid, const struct wifi_mac_ScanSSID ssids[])
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    if (wifimac->wm_mac_exit)
    {
        DPRINTF(AML_DEBUG_WARNING, "<%s> : %s drop scan due to interface down\n", wnet_vif->vm_ndev->name, __func__);
        return 0;

    } else if (wifimac->wm_flags & WIFINET_F_SCAN) {
        DPRINTF(AML_DEBUG_WARNING, "<%s> : %s drop scan due to last scan not finish\n", wnet_vif->vm_ndev->name, __func__);
        return 0;

    } else if (wnet_vif->vm_scan_hang) {
        DPRINTF(AML_DEBUG_WARNING, "<%s> : %s drop scan due to scan hang\n", wnet_vif->vm_ndev->name, __func__);
        return 0;
    }

#ifdef CONFIG_P2P
    if ((wifimac->wm_flags & WIFINET_F_NOSCAN) && (wnet_vif->vm_p2p->p2p_enable == 0)) {
        DPRINTF(AML_DEBUG_WARNING, "%s %d  not allow start scan beacon p2p_enable!!!!! \n", __func__,__LINE__);
        return 0;
    }
#endif//CONFIG_P2P

    wifimac->wm_flags |= WIFINET_F_SCAN;
    ss->scan_CfgFlags |= (flags & WIFINET_SCANCFG_MASK);

    //if opmode is AP or monitor ,we not need connect bss at scan end
    if ((wnet_vif->vm_opmode != WIFINET_M_IBSS) && (wnet_vif->vm_opmode != WIFINET_M_STA)) {
        ss->scan_CfgFlags |= WIFINET_SCANCFG_NOPICK;
    }
    wifi_mac_save_ssid(wnet_vif, ss, nssid, ssids);

    if (!wnet_vif->vm_scan_before_connect_flag && ((ss->VMacPriv != wnet_vif) || (ss->scan_CfgFlags & WIFINET_SCANCFG_FLUSH))) {
        wifi_mac_scan_flush(wifimac);
        ss->VMacPriv = wnet_vif;
    }

    printk("%s vid:%d---> scan start, CfgFlags is:%08x\n", __func__, wnet_vif->wnet_vif_id, ss->scan_CfgFlags);

    wifimac->wm_scanplayercnt++;
    wifi_mac_add_work_task(wifimac, scan_start_task, NULL, (SYS_TYPE)ss, 0, (SYS_TYPE)wifimac->wm_scanplayercnt,
        (SYS_TYPE)wnet_vif, (SYS_TYPE)wnet_vif->wnet_vif_replaycounter);

    os_timer_ex_start_period(&ss->ss_scan_timer, WIFINET_SCAN_PROTECT_TIME);
    return 1;
}

int wifi_mac_scan_before_connect(struct wifi_mac_scan_state *ss, struct wlan_net_vif *wnet_vif, int flags)
{
    if (wifi_mac_scan_get_match_node(ss, wnet_vif) == 0)
    {
         printk("%s %d no bss match\n", __func__,__LINE__);
         return 0;
    }

    wnet_vif->vm_scan_before_connect_flag = 1;
    ss->scan_CfgFlags |= WIFINET_SCANCFG_CONNECT;
    wifi_mac_start_scan(wnet_vif, flags, wnet_vif->vm_des_nssid, wnet_vif->vm_des_ssid);
    return 1;
}

int wifi_mac_chk_scan(struct wlan_net_vif *wnet_vif, int flags,
    unsigned int nssid, const struct wifi_mac_ScanSSID ssids[])
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    if (wifimac->wm_flags & WIFINET_F_SCAN) {
        return 1;
    }

    /*if no  FORCE & SCAN flag and  if within 60s after last scanning,
    we can try to connect with last scanning result, without scanning again. */
    if ((wnet_vif->vm_opmode == WIFINET_M_STA)
        && time_before(jiffies, wifimac->wm_lastscan + SCAN_VALID_DEFAULT))
    {
        if (wifi_mac_scan_before_connect(ss, wnet_vif, flags)) {
            return 1;
        }
    }

    ss->scan_CfgFlags |= WIFINET_SCANCFG_CONNECT;
    printk("%s flags:%08x\n", __func__, flags);
    return wifi_mac_start_scan(wnet_vif, flags,  nssid, ssids);
}

void wifi_mac_scan_vattach(struct wlan_net_vif *wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    wnet_vif->vm_scan_time_idle = WIFINET_SCAN_TIME_IDLE_DEFAULT;
    wnet_vif->vm_scan_time_connect = WIFINET_SCAN_TIME_CONNECT_DEFAULT;
    wnet_vif->vm_scan_time_before_connect = WIFINET_SCAN_TIME_BEFORE_CONNECT;
    wnet_vif->vm_scan_time_chan_switch = WIFINET_SCAN_TIME_CHANNEL_SWITCH;
    ss->VMacPriv = wnet_vif;
    vm_scan_setup_chan(ss,wnet_vif);
}

void wifi_mac_scan_vdetach(struct wlan_net_vif *wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    if (ss->VMacPriv == wnet_vif)
    {
        if (wifimac->wm_flags & WIFINET_F_SCAN)
        {
            os_timer_ex_cancel(&ss->ss_scan_timer, CANCEL_SLEEP);
            wifimac->wm_flags &= ~WIFINET_F_SCAN;
            printk("%s(%d):-->clean vm_flags 0x%x\n", __func__, __LINE__, wifimac->wm_flags);
        }
        wifi_mac_scan_flush(wifimac);
    }
}

void wifi_mac_scan_attach(struct wifi_mac *wifimac)
{
    struct wifi_mac_scan_state *ss;
    struct scaninfo_table *st;
    int i=0;

    wifimac->wm_roaming = WIFINET_ROAMING_MANUAL;

    ss = (struct wifi_mac_scan_state *)NET_MALLOC(sizeof(struct wifi_mac_scan_state),
        GFP_KERNEL, "wifi_mac_scan_attach.ss");
    if (ss != NULL)
    {
        wifimac->wm_scan = ss;
    }
    else
    {
        DPRINTF(AML_DEBUG_ERROR, "<ERROR> %s %d\n",__func__,__LINE__);
        wifimac->wm_scan = NULL;
        return ;
    }
    st = (struct scaninfo_table *)NET_MALLOC(sizeof(struct scaninfo_table),
        GFP_KERNEL, "sta_attach.st");

    if (st == NULL)
    {
        FREE(wifimac->wm_scan, "wifi_mac_scan_attach.ss");
        wifimac->wm_scan = NULL;
        DPRINTF(AML_DEBUG_SCAN, "<ERROR> %s %d\n",__func__,__LINE__);
        return ;
    }
    spin_lock_init(&st->st_lock);
    spin_lock_init(&ss->scan_lock);

    INIT_LIST_HEAD(&st->st_entry);
    for(i = 0; i < STA_HASHSIZE; i++)
    {
        INIT_LIST_HEAD(&st->st_hash[i]);
    }
    os_timer_ex_initialize(&ss->ss_scan_timer, 0, wifi_mac_scan_timeout_ex, ss);
#ifdef FW_RF_CALIBRATION
    os_timer_ex_initialize(&ss->ss_probe_timer, 0, wifi_mac_scan_send_probe_timeout_ex, ss);
#endif

    wifimac->wm_scan->ScanTablePriv = st;
}

void wifi_mac_scan_detach(struct wifi_mac *wifimac)
{
    struct wifi_mac_scan_state *ss = wifimac->wm_scan;

    if (ss != NULL)
    {
        DPRINTF(AML_DEBUG_INIT, "<running> %s %d \n",__func__,__LINE__);
        os_timer_ex_del(&ss->ss_scan_timer, CANCEL_SLEEP);

#ifdef FW_RF_CALIBRATION
        os_timer_ex_del(&ss->ss_probe_timer, CANCEL_SLEEP);
#endif

        if (ss->ScanTablePriv != NULL)
        {
            wifi_mac_scan_flush(wifimac);
            FREE(ss->ScanTablePriv,"sta_attach.st");
        }

        wifimac->wm_flags &= ~WIFINET_F_SCAN;
        printk("%s(%d):-->clean vm_flags 0x%x\n", __func__, __LINE__, wifimac->wm_flags);

        FREE(wifimac->wm_scan,"wifi_mac_scan_attach.ss");
        wifimac->wm_scan = NULL;
    }
}

void wifi_mac_process_tx_error(struct wlan_net_vif *wnet_vif)
{
    struct wifi_mac *wifimac = wnet_vif->vm_wmac;
    int flag = WIFINET_SCANCFG_ACTIVE
                | WIFINET_SCANCFG_NOPICK
                | WIFINET_SCANCFG_USERREQ
                | WIFINET_SCANCFG_FLUSH;
    int cnt = 0;

    printk("%s, %d \n", __func__, __LINE__);
    wifi_mac_cancel_scan(wnet_vif);
    /* waiting for completing scan process */
    while (wifimac->wm_flags & WIFINET_F_SCAN)
    {
        msleep(20);
        if (cnt++ > 20)
        {
            printk("<%s>:%s %d wait scan end fail when process tx error \n",
                wnet_vif->vm_ndev->name, __func__, __LINE__);
            return;
        }
    }
    wifi_mac_start_scan(wnet_vif, flag, wnet_vif->vm_des_nssid, wnet_vif->vm_des_ssid);
}