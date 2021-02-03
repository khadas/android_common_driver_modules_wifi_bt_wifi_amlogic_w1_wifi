#include "rf_calibration.h"

#ifdef RF_CALI_TEST

void t9026_wf2g_rxlnatank(struct wf2g_rxlnatank_result * wf2g_rxlnatank_result, bool manual, U8 channel)
{
    RG_TOP_A15_FIELD_T    rg_top_a15;
    RG_TOP_A29_FIELD_T    rg_top_a29;
    RG_SX_A48_FIELD_T    rg_sx_a48;
    RG_TX_A6_FIELD_T    rg_tx_a6;
    RG_TX_A11_FIELD_T    rg_tx_a11;
    RG_RX_A2_FIELD_T    rg_rx_a2;
    RG_RX_A90_FIELD_T    rg_rx_a90;
    RG_TOP_A23_FIELD_T    rg_top_a23;
    RG_XMIT_A3_FIELD_T    rg_xmit_a3;
    RG_XMIT_A4_FIELD_T    rg_xmit_a4;
    RG_XMIT_A56_FIELD_T    rg_xmit_a56;
    RG_RECV_A11_FIELD_T    rg_recv_a11;
    RG_ESTI_A14_FIELD_T    rg_esti_a14;
    RG_ESTI_A64_FIELD_T    rg_esti_a64;
    RG_ESTI_A65_FIELD_T    rg_esti_a65;
    RG_XMIT_A46_FIELD_T    rg_xmit_a46;
    RG_ESTI_A18_FIELD_T    rg_esti_a18;
    RG_RX_A8_FIELD_T    rg_rx_a8;
    RG_ESTI_A94_FIELD_T    rg_esti_a94;

    // Internal variables definitions
    bool    RG_TOP_A15_saved = False;
    RG_TOP_A15_FIELD_T    rg_top_a15_i;
    // Internal variables definitions
    bool    RG_TOP_A29_saved = False;
    RG_TOP_A29_FIELD_T    rg_top_a29_i;
    // Internal variables definitions
    bool    RG_SX_A48_saved = False;
    RG_SX_A48_FIELD_T    rg_sx_a48_i;
    // Internal variables definitions
    bool    RG_TX_A6_saved = False;
    RG_TX_A6_FIELD_T    rg_tx_a6_i;
    // Internal variables definitions
    bool    RG_TX_A11_saved = False;
    RG_TX_A11_FIELD_T    rg_tx_a11_i;
    // Internal variables definitions
    bool    RG_RX_A2_saved = False;
    RG_RX_A2_FIELD_T    rg_rx_a2_i;
    // Internal variables definitions
    bool    RG_RX_A90_saved = False;
    RG_RX_A90_FIELD_T    rg_rx_a90_i;
    // Internal variables definitions
    bool    RG_TOP_A23_saved = False;
    RG_TOP_A23_FIELD_T    rg_top_a23_i;
    // Internal variables definitions
    bool    RG_XMIT_A3_saved = False;
    RG_XMIT_A3_FIELD_T    rg_xmit_a3_i;
    // Internal variables definitions
    bool    RG_XMIT_A4_saved = False;
    RG_XMIT_A4_FIELD_T    rg_xmit_a4_i;
    // Internal variables definitions
    bool    RG_XMIT_A56_saved = False;
    RG_XMIT_A56_FIELD_T    rg_xmit_a56_i;
    // Internal variables definitions
    bool    RG_RECV_A11_saved = False;
    RG_RECV_A11_FIELD_T    rg_recv_a11_i;
    // Internal variables definitions
    bool    RG_ESTI_A14_saved = False;
    RG_ESTI_A14_FIELD_T    rg_esti_a14_i;
    // Internal variables definitions
    bool    RG_XMIT_A46_saved = False;
    RG_XMIT_A46_FIELD_T    rg_xmit_a46_i;

    struct rx_lna_output rx_lna_out;


    unsigned int code = 0;
    unsigned int code_out = 0;
    unsigned int pwr_out = 0;
    int max_i = 0;

    unsigned int rx_lna_ready = 0;
    unsigned int unlock_cnt = 0;
    unsigned int TIME_OUT_CNT = RF_TIME_OUT_CNT;
    unsigned int channel_idx = 0;

    CALI_MODE_T cali_mode = RXLNATANK;
    struct wf2g_rxdcoc_in wf2g_rxdcoc_in;
    struct wf2g_rxdcoc_result wf2g_rxdcoc_result;

    rg_top_a15.data = rf_read_register(RG_TOP_A15);
    if(!RG_TOP_A15_saved) {
        RG_TOP_A15_saved = True;
        rg_top_a15_i.b.rg_wf_adda_man_mode = rg_top_a15.b.rg_wf_adda_man_mode;
    }
    rg_top_a15.b.rg_wf_adda_man_mode = 0x1;
    rf_write_register(RG_TOP_A15, rg_top_a15.data);

    rg_top_a29.data = rf_read_register(RG_TOP_A29);
    if(!RG_TOP_A29_saved) {
        RG_TOP_A29_saved = True;
        rg_top_a29_i.b.RG_M6_WF_TRX_HF_LDO_RXB_EN = rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RXB_EN;
        rg_top_a29_i.b.RG_M6_WF_TRX_HF_LDO_RX1_EN = rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RX1_EN;
        rg_top_a29_i.b.RG_M6_WF_TRX_HF_LDO_RX2_EN = rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RX2_EN;
        rg_top_a29_i.b.RG_M6_WF_TRX_LF_LDO_RX_EN = rg_top_a29.b.RG_M6_WF_TRX_LF_LDO_RX_EN;
    }
    rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RXB_EN = 0x1;
    rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RX1_EN = 0x1;
    rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RX2_EN = 0x1;
    rg_top_a29.b.RG_M6_WF_TRX_LF_LDO_RX_EN = 0x1;
    rf_write_register(RG_TOP_A29, rg_top_a29.data);

    rg_sx_a48.data = rf_read_register(RG_SX_A48);
    if(!RG_SX_A48_saved) {
        RG_SX_A48_saved = True;
        rg_sx_a48_i.b.RG_M6_WF_SX_LOG2G_RXLO_EN = rg_sx_a48.b.RG_M6_WF_SX_LOG2G_RXLO_EN;
    }
    rg_sx_a48.b.RG_M6_WF_SX_LOG2G_RXLO_EN = 0x1;
    rf_write_register(RG_SX_A48, rg_sx_a48.data);

    rg_tx_a6.data = rf_read_register(RG_TX_A6);
    if(!RG_TX_A6_saved) {
        RG_TX_A6_saved = True;
        rg_tx_a6_i.b.RG_WF2G_TX_RXIQCAL_EN = rg_tx_a6.b.RG_WF2G_TX_RXIQCAL_EN;
        rg_tx_a6_i.b.RG_WF2G_TX_MAN_MODE = rg_tx_a6.b.RG_WF2G_TX_MAN_MODE;
    }
    rg_tx_a6.b.RG_WF2G_TX_RXIQCAL_EN = 0x1;
    rg_tx_a6.b.RG_WF2G_TX_MAN_MODE = 0x1;
    rf_write_register(RG_TX_A6, rg_tx_a6.data);

    rg_tx_a11.data = rf_read_register(RG_TX_A11);
    if(!RG_TX_A11_saved) {
        RG_TX_A11_saved = True;
        rg_tx_a11_i.b.RG_M6_WF2G_TX_PA_EN = rg_tx_a11.b.RG_M6_WF2G_TX_PA_EN;
    }
    rg_tx_a11.b.RG_M6_WF2G_TX_PA_EN = 0x0;
    rf_write_register(RG_TX_A11, rg_tx_a11.data);

    rg_rx_a2.data = rf_read_register(RG_RX_A2);
    if(!RG_RX_A2_saved) {
        RG_RX_A2_saved = True;
        rg_rx_a2_i.b.RG_WF_RX_BBBM_MAN_MODE = rg_rx_a2.b.RG_WF_RX_BBBM_MAN_MODE;
        rg_rx_a2_i.b.RG_WF_RX_GAIN_MAN = rg_rx_a2.b.RG_WF_RX_GAIN_MAN;
        rg_rx_a2_i.b.RG_WF_RX_GAIN_MAN_MODE = rg_rx_a2.b.RG_WF_RX_GAIN_MAN_MODE;
    }
    rg_rx_a2.b.RG_WF_RX_BBBM_MAN_MODE = 0x1;                         // Use default 20MHz BW setting
    rg_rx_a2.b.RG_WF_RX_GAIN_MAN = 0x21;
    rg_rx_a2.b.RG_WF_RX_GAIN_MAN_MODE = 0x1;
    rf_write_register(RG_RX_A2, rg_rx_a2.data);

    rg_rx_a90.data = rf_read_register(RG_RX_A90);
    if(!RG_RX_A90_saved) {
        RG_RX_A90_saved = True;
        rg_rx_a90_i.b.RG_M6_WF_RX_LNA_BIAS_EN = rg_rx_a90.b.RG_M6_WF_RX_LNA_BIAS_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_GM_BIAS_EN = rg_rx_a90.b.RG_M6_WF_RX_GM_BIAS_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_MXR_BIAS_EN = rg_rx_a90.b.RG_M6_WF_RX_MXR_BIAS_EN;
        rg_rx_a90_i.b.RG_M6_WF2G_RX_LNA_EN = rg_rx_a90.b.RG_M6_WF2G_RX_LNA_EN;
        rg_rx_a90_i.b.RG_M6_WF2G_RX_GM_EN = rg_rx_a90.b.RG_M6_WF2G_RX_GM_EN;
        rg_rx_a90_i.b.RG_M6_WF2G_RX_MXR_EN = rg_rx_a90.b.RG_M6_WF2G_RX_MXR_EN;
        rg_rx_a90_i.b.RG_M6_WF2G_RX_IRRCAL_EN = rg_rx_a90.b.RG_M6_WF2G_RX_IRRCAL_EN;
        rg_rx_a90_i.b.RG_M6_WF_RTX_ABB_RX_EN = rg_rx_a90.b.RG_M6_WF_RTX_ABB_RX_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_TIA_BIAS_EN = rg_rx_a90.b.RG_M6_WF_RX_TIA_BIAS_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_RSSIPGA_BIAS_EN = rg_rx_a90.b.RG_M6_WF_RX_RSSIPGA_BIAS_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_LPF_BIAS_EN = rg_rx_a90.b.RG_M6_WF_RX_LPF_BIAS_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_TIA_EN = rg_rx_a90.b.RG_M6_WF_RX_TIA_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_RSSIPGA_EN = rg_rx_a90.b.RG_M6_WF_RX_RSSIPGA_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_LPF_EN = rg_rx_a90.b.RG_M6_WF_RX_LPF_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_LPF_I_DCOC_EN = rg_rx_a90.b.RG_M6_WF_RX_LPF_I_DCOC_EN;
        rg_rx_a90_i.b.RG_M6_WF_RX_LPF_I_DCOC_EN = rg_rx_a90.b.RG_M6_WF_RX_LPF_I_DCOC_EN;
    }
    rg_rx_a90.b.RG_M6_WF_RX_LNA_BIAS_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_GM_BIAS_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_MXR_BIAS_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF2G_RX_LNA_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF2G_RX_GM_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF2G_RX_MXR_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF2G_RX_IRRCAL_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RTX_ABB_RX_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_TIA_BIAS_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_RSSIPGA_BIAS_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_BIAS_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_TIA_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_RSSIPGA_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_I_DCOC_EN = 0x1;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_I_DCOC_EN = 0x1;
    rf_write_register(RG_RX_A90, rg_rx_a90.data);

    rg_top_a23.data = rf_read_register(RG_TOP_A23);
    if(!RG_TOP_A23_saved) {
        RG_TOP_A23_saved = True;
        rg_top_a23_i.b.rg_m6_wf_radc_en = rg_top_a23.b.rg_m6_wf_radc_en;
        rg_top_a23_i.b.rg_m6_wf_wadc_enable_i = rg_top_a23.b.rg_m6_wf_wadc_enable_i;
        rg_top_a23_i.b.rg_m6_wf_wadc_enable_q = rg_top_a23.b.rg_m6_wf_wadc_enable_q;
    }
    rg_top_a23.b.rg_m6_wf_radc_en = 0x1;
    rg_top_a23.b.rg_m6_wf_wadc_enable_i = 0x1;
    rg_top_a23.b.rg_m6_wf_wadc_enable_q = 0x1;
    rf_write_register(RG_TOP_A23, rg_top_a23.data);

    wf2g_rxdcoc_in.bw_idx = 0;
    wf2g_rxdcoc_in.tia_idx = 0;
    wf2g_rxdcoc_in.lpf_idx = 1;
    wf2g_rxdcoc_in.wf_rx_gain = rg_rx_a2.b.RG_WF_RX_GAIN_MAN;
    wf2g_rxdcoc_in.manual_mode = 1;
    t9026_wf2g_rxdcoc(&wf2g_rxdcoc_in, &wf2g_rxdcoc_result);

     fi_ahb_write(PHY_TX_LPF_BYPASS, 0x1);

    rg_xmit_a3.data = fi_ahb_read(RG_XMIT_A3);
    if(!RG_XMIT_A3_saved) {
        RG_XMIT_A3_saved = True;
        rg_xmit_a3_i.b.rg_tg2_f_sel = rg_xmit_a3.b.rg_tg2_f_sel;
        rg_xmit_a3_i.b.rg_tg2_enable = rg_xmit_a3.b.rg_tg2_enable;
        rg_xmit_a3_i.b.rg_tg2_tone_man_en = rg_xmit_a3.b.rg_tg2_tone_man_en;
    }
    rg_xmit_a3.b.rg_tg2_f_sel = 0x66666;                     // new_set_reg(0xe40c, 0x90066666);//open tone2(1M)
    rg_xmit_a3.b.rg_tg2_enable = 0x1;
    rg_xmit_a3.b.rg_tg2_tone_man_en = 0x1;
    fi_ahb_write(RG_XMIT_A3, rg_xmit_a3.data);

    rg_xmit_a4.data = fi_ahb_read(RG_XMIT_A4);
    if(!RG_XMIT_A4_saved) {
        RG_XMIT_A4_saved = True;
        rg_xmit_a4_i.b.rg_tx_signal_sel = rg_xmit_a4.b.rg_tx_signal_sel;
    }
    rg_xmit_a4.b.rg_tx_signal_sel = 0x1;                         // new_set_reg(0xe410, 0x00000001);//1: select single tone
    fi_ahb_write(RG_XMIT_A4, rg_xmit_a4.data);

    rg_xmit_a56.data = fi_ahb_read(RG_XMIT_A56);
    if(!RG_XMIT_A56_saved) {
        RG_XMIT_A56_saved = True;
        rg_xmit_a56_i.b.rg_txdpd_comp_bypass = rg_xmit_a56.b.rg_txdpd_comp_bypass;
    }
    rg_xmit_a56.b.rg_txdpd_comp_bypass = 0x1;                         // new_set_reg(0xe4f0, 0x15008080);   //TX DPD bypass
    fi_ahb_write(RG_XMIT_A56, rg_xmit_a56.data);

    rg_recv_a11.data = fi_ahb_read(RG_RECV_A11);
    if(!RG_RECV_A11_saved) {
        RG_RECV_A11_saved = True;
        rg_recv_a11_i.b.rg_rxirr_bypass = rg_recv_a11.b.rg_rxirr_bypass;
    }
    rg_recv_a11.b.rg_rxirr_bypass = 0x1;                         // i2c_set_reg_fragment(0xe82c, 4, 4, 1); //RX IRR bypass
    fi_ahb_write(RG_RECV_A11, rg_recv_a11.data);

    rg_esti_a14.data = fi_ahb_read(RG_ESTI_A14);
    if(!RG_ESTI_A14_saved) {
        RG_ESTI_A14_saved = True;
        rg_esti_a14_i.b.rg_dc_rm_leaky_factor = rg_esti_a14.b.rg_dc_rm_leaky_factor;
    }
    rg_esti_a14.b.rg_dc_rm_leaky_factor = 0x0;
    fi_ahb_write(RG_ESTI_A14, rg_esti_a14.data);

    rg_esti_a64.data = fi_ahb_read(RG_ESTI_A64);
    rg_esti_a64.b.rg_adda_wait_count = 0x400;
    fi_ahb_write(RG_ESTI_A64, rg_esti_a64.data);

    rg_esti_a65.data = fi_ahb_read(RG_ESTI_A65);
    rg_esti_a65.b.rg_adda_calc_count = 0x400;
    fi_ahb_write(RG_ESTI_A65, rg_esti_a65.data);

    rg_xmit_a46.data = fi_ahb_read(RG_XMIT_A46);
    if(!RG_XMIT_A46_saved) {
        RG_XMIT_A46_saved = True;
        rg_xmit_a46_i.b.rg_txpwc_comp_man_sel = rg_xmit_a46.b.rg_txpwc_comp_man_sel;
        rg_xmit_a46_i.b.rg_txpwc_comp_man = rg_xmit_a46.b.rg_txpwc_comp_man;
    }
    rg_xmit_a46.b.rg_txpwc_comp_man_sel = 0x1;                         // i2c_set_reg_fragment(0xe4b8, 16, 16, 1);//set txpwctrl gain manual to 1
    rg_xmit_a46.b.rg_txpwc_comp_man = 0x10;                        // i2c_set_reg_fragment(0xe4b8, 31, 24, 0x10);//set txpwctrl gain
    fi_ahb_write(RG_XMIT_A46, rg_xmit_a46.data);

    rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
    rg_esti_a18.b.rg_lnatank_read_response_bypass = 0x0;
    rg_esti_a18.b.rg_lnatank_read_response = 0x0;
    fi_ahb_write(RG_ESTI_A18, rg_esti_a18.data);


    t9026_cali_mode_access(cali_mode);

    rg_rx_a8.data = rf_read_register(RG_RX_A8);
    rg_rx_a8.b.RG_WF2G_RX_LNA_TANK_SEL_MAN_MODE = manual;
    rf_write_register(RG_RX_A8, rg_rx_a8.data);

    for (code = 0; code < 16; code++)
    {
        if (manual == 0)
        {


            rg_rx_a8.data = rf_read_register(RG_RX_A8);
            rg_rx_a8.b.RG_WF2G_RX_LNA_TANK_SEL = code;                        // Band0 : 4900-5000MHz
            rf_write_register(RG_RX_A8, rg_rx_a8.data);

        }
        else
        {

            rg_rx_a8.data = rf_read_register(RG_RX_A8);
            rg_rx_a8.b.RG_WF2G_RX_LNA_TANK_SEL_MAN = code;                        //  Manual Tank SEL
            rf_write_register(RG_RX_A8, rg_rx_a8.data);

        } // if(manual == 0)

        rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
        rg_esti_a18.b.rg_lnatank_read_response = 0x1;                         // i2c_set_reg_fragment(0xec48, 31, 31, 1);//open soft response
        fi_ahb_write(RG_ESTI_A18, rg_esti_a18.data);

        rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
        rx_lna_ready = rg_esti_a18.b.ro_lnatank_esti_code_ready;
        while (unlock_cnt < TIME_OUT_CNT)
        {
            if (rx_lna_ready == 0)
            {

                rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
                rx_lna_ready = rg_esti_a18.b.ro_lnatank_esti_code_ready;
                unlock_cnt = unlock_cnt + 1;
                RF_DPRINTF(RF_DEBUG_INFO, "unlock_cnt:%d, RX LNA NOT READY...\n", unlock_cnt);
            }
            else
            {
                RF_DPRINTF(RF_DEBUG_INFO, "RX LNA READY\n");
                break;
            } // if(rx_lna_ready == 0)
        } // while(unlock_cnt < TIME_OUT_CNT)

        rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
        rg_esti_a18.b.rg_lnatank_read_response = 0x0;                         // i2c_set_reg_fragment(0xec48, 31, 31, 1);//open soft response
        fi_ahb_write(RG_ESTI_A18, rg_esti_a18.data);

        rg_esti_a94.data = fi_ahb_read(RG_ESTI_A94);
        pwr_out = rg_esti_a94.b.ro_lnatank_esti_pow;//<17.1>
        RF_DPRINTF(RF_DEBUG_RESULT, "unlock_cnt:%d, code = %d, RXLNA pwr_out = %d\n", unlock_cnt, code, pwr_out);
        if (max_i++ > 1024)
        {
            return;
        }
    } // for(code = 0; code < 16; code ++)
    unlock_cnt = 0;

    rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
    rx_lna_ready = rg_esti_a18.b.ro_lnatank_esti_code_ready;
    while (unlock_cnt < TIME_OUT_CNT)
    {
        if (rx_lna_ready == 0)
        {

            rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
            rx_lna_ready = rg_esti_a18.b.ro_lnatank_esti_code_ready;
            unlock_cnt = unlock_cnt + 1;
        }
        else
        {
            break;
        }  // if(rx_lna_ready == 0)
    }  // while(unlock_cnt < TIME_OUT_CNT)

    rg_esti_a18.data = fi_ahb_read(RG_ESTI_A18);
    code_out = rg_esti_a18.b.ro_lnatank_esti_code;

    if (manual == 0)   //write back calibration result
    {

        rg_rx_a8.data = rf_read_register(RG_RX_A8);
        rg_rx_a8.b.RG_WF2G_RX_LNA_TANK_SEL = code_out;                    // Band0 : 4900-5000MHz
        rf_write_register(RG_RX_A8, rg_rx_a8.data);

    }
    else
    {

        rg_rx_a8.data = rf_read_register(RG_RX_A8);
        rg_rx_a8.b.RG_WF2G_RX_LNA_TANK_SEL_MAN = code_out;                    //  Manual Tank SEL
        rf_write_register(RG_RX_A8, rg_rx_a8.data);

    } // if(manual == 0)

    wf2g_rxlnatank_result->cal_rx_lna.rx_lna_code = code_out;
    rx_lna_out.channel_idx = channel_idx;
    rx_lna_out.code_out = code_out; 
    RF_DPRINTF(RF_DEBUG_RESULT, "unlock_cnt:%d, chn_idx: %d, RXLNA code out %d\n", unlock_cnt, channel_idx, code_out);
    t9026_cali_mode_exit();

    // Revert the original value before calibration back
    rg_top_a15.data = rf_read_register(RG_TOP_A15);
    rg_top_a15.b.rg_wf_adda_man_mode = rg_top_a15_i.b.rg_wf_adda_man_mode;
    rf_write_register(RG_TOP_A15, rg_top_a15.data);
    // Revert the original value before calibration back
    rg_top_a29.data = rf_read_register(RG_TOP_A29);
    rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RXB_EN = rg_top_a29_i.b.RG_M6_WF_TRX_HF_LDO_RXB_EN;
    rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RX1_EN = rg_top_a29_i.b.RG_M6_WF_TRX_HF_LDO_RX1_EN;
    rg_top_a29.b.RG_M6_WF_TRX_HF_LDO_RX2_EN = rg_top_a29_i.b.RG_M6_WF_TRX_HF_LDO_RX2_EN;
    rg_top_a29.b.RG_M6_WF_TRX_LF_LDO_RX_EN = rg_top_a29_i.b.RG_M6_WF_TRX_LF_LDO_RX_EN;
    rf_write_register(RG_TOP_A29, rg_top_a29.data);
    // Revert the original value before calibration back
    rg_sx_a48.data = rf_read_register(RG_SX_A48);
    rg_sx_a48.b.RG_M6_WF_SX_LOG2G_RXLO_EN = rg_sx_a48_i.b.RG_M6_WF_SX_LOG2G_RXLO_EN;
    rf_write_register(RG_SX_A48, rg_sx_a48.data);
    // Revert the original value before calibration back
    rg_tx_a6.data = rf_read_register(RG_TX_A6);
    rg_tx_a6.b.RG_WF2G_TX_RXIQCAL_EN = rg_tx_a6_i.b.RG_WF2G_TX_RXIQCAL_EN;
    rg_tx_a6.b.RG_WF2G_TX_MAN_MODE = rg_tx_a6_i.b.RG_WF2G_TX_MAN_MODE;
    rf_write_register(RG_TX_A6, rg_tx_a6.data);
    // Revert the original value before calibration back
    rg_tx_a11.data = rf_read_register(RG_TX_A11);
    rg_tx_a11.b.RG_M6_WF2G_TX_PA_EN = rg_tx_a11_i.b.RG_M6_WF2G_TX_PA_EN;
    rf_write_register(RG_TX_A11, rg_tx_a11.data);
    // Revert the original value before calibration back
    rg_rx_a2.data = rf_read_register(RG_RX_A2);
    rg_rx_a2.b.RG_WF_RX_BBBM_MAN_MODE = rg_rx_a2_i.b.RG_WF_RX_BBBM_MAN_MODE;
    rg_rx_a2.b.RG_WF_RX_GAIN_MAN = rg_rx_a2_i.b.RG_WF_RX_GAIN_MAN;
    rg_rx_a2.b.RG_WF_RX_GAIN_MAN_MODE = rg_rx_a2_i.b.RG_WF_RX_GAIN_MAN_MODE;
    rf_write_register(RG_RX_A2, rg_rx_a2.data);
    // Revert the original value before calibration back
    rg_rx_a90.data = rf_read_register(RG_RX_A90);
    rg_rx_a90.b.RG_M6_WF_RX_LNA_BIAS_EN = rg_rx_a90_i.b.RG_M6_WF_RX_LNA_BIAS_EN;
    rg_rx_a90.b.RG_M6_WF_RX_GM_BIAS_EN = rg_rx_a90_i.b.RG_M6_WF_RX_GM_BIAS_EN;
    rg_rx_a90.b.RG_M6_WF_RX_MXR_BIAS_EN = rg_rx_a90_i.b.RG_M6_WF_RX_MXR_BIAS_EN;
    rg_rx_a90.b.RG_M6_WF2G_RX_LNA_EN = rg_rx_a90_i.b.RG_M6_WF2G_RX_LNA_EN;
    rg_rx_a90.b.RG_M6_WF2G_RX_GM_EN = rg_rx_a90_i.b.RG_M6_WF2G_RX_GM_EN;
    rg_rx_a90.b.RG_M6_WF2G_RX_MXR_EN = rg_rx_a90_i.b.RG_M6_WF2G_RX_MXR_EN;
    rg_rx_a90.b.RG_M6_WF2G_RX_IRRCAL_EN = rg_rx_a90_i.b.RG_M6_WF2G_RX_IRRCAL_EN;
    rg_rx_a90.b.RG_M6_WF_RTX_ABB_RX_EN = rg_rx_a90_i.b.RG_M6_WF_RTX_ABB_RX_EN;
    rg_rx_a90.b.RG_M6_WF_RX_TIA_BIAS_EN = rg_rx_a90_i.b.RG_M6_WF_RX_TIA_BIAS_EN;
    rg_rx_a90.b.RG_M6_WF_RX_RSSIPGA_BIAS_EN = rg_rx_a90_i.b.RG_M6_WF_RX_RSSIPGA_BIAS_EN;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_BIAS_EN = rg_rx_a90_i.b.RG_M6_WF_RX_LPF_BIAS_EN;
    rg_rx_a90.b.RG_M6_WF_RX_TIA_EN = rg_rx_a90_i.b.RG_M6_WF_RX_TIA_EN;
    rg_rx_a90.b.RG_M6_WF_RX_RSSIPGA_EN = rg_rx_a90_i.b.RG_M6_WF_RX_RSSIPGA_EN;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_EN = rg_rx_a90_i.b.RG_M6_WF_RX_LPF_EN;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_I_DCOC_EN = rg_rx_a90_i.b.RG_M6_WF_RX_LPF_I_DCOC_EN;
    rg_rx_a90.b.RG_M6_WF_RX_LPF_I_DCOC_EN = rg_rx_a90_i.b.RG_M6_WF_RX_LPF_I_DCOC_EN;
    rf_write_register(RG_RX_A90, rg_rx_a90.data);
    // Revert the original value before calibration back
    rg_top_a23.data = rf_read_register(RG_TOP_A23);
    rg_top_a23.b.rg_m6_wf_radc_en = rg_top_a23_i.b.rg_m6_wf_radc_en;
    rg_top_a23.b.rg_m6_wf_wadc_enable_i = rg_top_a23_i.b.rg_m6_wf_wadc_enable_i;
    rg_top_a23.b.rg_m6_wf_wadc_enable_q = rg_top_a23_i.b.rg_m6_wf_wadc_enable_q;
    rf_write_register(RG_TOP_A23, rg_top_a23.data);
    // Revert the original value before calibration back
    rg_xmit_a3.data = fi_ahb_read(RG_XMIT_A3);
    rg_xmit_a3.b.rg_tg2_f_sel = rg_xmit_a3_i.b.rg_tg2_f_sel;
    rg_xmit_a3.b.rg_tg2_enable = rg_xmit_a3_i.b.rg_tg2_enable;
    rg_xmit_a3.b.rg_tg2_tone_man_en = rg_xmit_a3_i.b.rg_tg2_tone_man_en;
    fi_ahb_write(RG_XMIT_A3, rg_xmit_a3.data);
    // Revert the original value before calibration back
    rg_xmit_a4.data = fi_ahb_read(RG_XMIT_A4);
    rg_xmit_a4.b.rg_tx_signal_sel = rg_xmit_a4_i.b.rg_tx_signal_sel;
    fi_ahb_write(RG_XMIT_A4, rg_xmit_a4.data);
    // Revert the original value before calibration back
    rg_xmit_a56.data = fi_ahb_read(RG_XMIT_A56);
    rg_xmit_a56.b.rg_txdpd_comp_bypass = rg_xmit_a56_i.b.rg_txdpd_comp_bypass;
    fi_ahb_write(RG_XMIT_A56, rg_xmit_a56.data);
    // Revert the original value before calibration back
    rg_recv_a11.data = fi_ahb_read(RG_RECV_A11);
    rg_recv_a11.b.rg_rxirr_bypass = rg_recv_a11_i.b.rg_rxirr_bypass;
    fi_ahb_write(RG_RECV_A11, rg_recv_a11.data);
    // Revert the original value before calibration back
    rg_esti_a14.data = fi_ahb_read(RG_ESTI_A14);
    rg_esti_a14.b.rg_dc_rm_leaky_factor = rg_esti_a14_i.b.rg_dc_rm_leaky_factor;
    fi_ahb_write(RG_ESTI_A14, rg_esti_a14.data);
    // Revert the original value before calibration back
    rg_xmit_a46.data = fi_ahb_read(RG_XMIT_A46);
    rg_xmit_a46.b.rg_txpwc_comp_man_sel = rg_xmit_a46_i.b.rg_txpwc_comp_man_sel;
    rg_xmit_a46.b.rg_txpwc_comp_man = rg_xmit_a46_i.b.rg_txpwc_comp_man;
    fi_ahb_write(RG_XMIT_A46, rg_xmit_a46.data);
    return;
}

#endif // RF_CALI_TEST