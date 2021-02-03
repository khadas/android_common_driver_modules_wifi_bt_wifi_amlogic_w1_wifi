#include "rf_calibration.h"

#ifdef RF_CALI_TEST

#if 1
void t9026_wf2g_txiqdc_check(U8 tx_gain)
{
    int ref_power_5M = 0;
    int ref_power_10M = 0;
    int cur_power_5M = 0;
    int cur_power_10M = 0;
    U8 tx_gain_list[2] = {0x3, 0x7};
    int ref_table[2][2];// = {{-36,-46}, {-27,-38}};
    U8 i = 0;
    int ret = 0;

    ref_table[0][0] = -34;
    ref_table[0][1] = -46;
    ref_table[1][0] = -27;
    ref_table[1][1] = -38;

    cur_power_5M = t9026_get_spectrum_snr_pwr(-5, 0);
    cur_power_10M = t9026_get_spectrum_snr_pwr(-10, 0);
    RF_DPRINTF(RF_DEBUG_INFO, "cur_power_5M = %.2d, cur_power_10M = %.2d\n", cur_power_5M, cur_power_10M);

    for (i = 0;i < 2; i++)
    {
        if (tx_gain_list[i] == tx_gain)
        {
            ref_power_5M = ref_table[i][0];
            ref_power_10M = ref_table[i][1];
            break;
        }
        else
        {
            continue;
        }
    }

    if ((cur_power_5M <= ref_power_5M) && (cur_power_10M <= ref_power_10M))
    {
        RF_DPRINTF(RF_DEBUG_RESULT_CHECK, "WF2G check TX DC&IRR calib pass.\n");
    }
    else
    {
        RF_DPRINTF(RF_DEBUG_RESULT_CHECK, "WF2G check TX DC&IRR calib fail.\n");
        ret = 1;
    }
    return;
}
#else
void t9026_wf2g_txiqdc_check(void)
{
    int ref_power_5M;
    int ref_power_10M;
    int cur_power_5M;
    int cur_power_10M;
    RG_XMIT_A45_FIELD_T    rg_xmit_a45;
    RG_XMIT_A46_FIELD_T    rg_xmit_a46;

    // Internal variables definitions
    bool    RG_XMIT_A45_saved = False;
    RG_XMIT_A45_FIELD_T    rg_xmit_a45_i;
    // Internal variables definitions
    bool    RG_XMIT_A46_saved = False;
    RG_XMIT_A46_FIELD_T    rg_xmit_a46_i;

    RF_DPRINTF(RF_DEBUG_INFO, "WF2G Check TX DC&IRR calib Start........\n");

    cur_power_5M = t9026_get_spectrum_snr_pwr(-5, 0);
    cur_power_10M = t9026_get_spectrum_snr_pwr(-10, 0);
    RF_DPRINTF(RF_DEBUG_INFO, "cur_power_5M = %.2d, cur_power_10M = %.2d\n", cur_power_5M, cur_power_10M);

    rg_xmit_a45.data = fi_ahb_read(RG_XMIT_A45);
    if (!RG_XMIT_A45_saved) {
        RG_XMIT_A45_saved = True;
        rg_xmit_a45_i.b.rg_man_tx_pwrlvl = rg_xmit_a45.b.rg_man_tx_pwrlvl;
        rg_xmit_a45_i.b.rg_tx_pwr_sel_man_en = rg_xmit_a45.b.rg_tx_pwr_sel_man_en;
    }
    rg_xmit_a45.b.rg_man_tx_pwrlvl = 0x80f90b;
    rg_xmit_a45.b.rg_tx_pwr_sel_man_en = 0x1;
    fi_ahb_write(RG_XMIT_A45, rg_xmit_a45.data);

    rg_xmit_a46.data = fi_ahb_read(RG_XMIT_A46);
    if (!RG_XMIT_A46_saved) {
        RG_XMIT_A46_saved = True;
        rg_xmit_a46_i.b.rg_man_txirr_comp_theta = rg_xmit_a46.b.rg_man_txirr_comp_theta;
        rg_xmit_a46_i.b.rg_man_txirr_comp_alpha = rg_xmit_a46.b.rg_man_txirr_comp_alpha;
    }
    rg_xmit_a46.b.rg_man_txirr_comp_theta = 0x64;
    rg_xmit_a46.b.rg_man_txirr_comp_alpha = 0xf1;
    fi_ahb_write(RG_XMIT_A46, rg_xmit_a46.data);

    ref_power_5M = t9026_get_spectrum_snr_pwr(-5, 0);
    ref_power_10M = t9026_get_spectrum_snr_pwr(-10, 0);
    RF_DPRINTF(RF_DEBUG_INFO, "ref_power_5M = %.2d, ref_power_10M = %.2d\n", ref_power_5M, ref_power_10M);

    if ((cur_power_5M <= ref_power_5M) && (cur_power_10M <= ref_power_10M))
    {
        RF_DPRINTF(RF_DEBUG_RESULT_CHECK, "WF2G check TX DC&IRR calib pass.\n");
    } else {
        RF_DPRINTF(RF_DEBUG_RESULT_CHECK, "WF2G check TX DC&IRR calib fail.\n");
    }

    // Revert the original value before calibration back
    rg_xmit_a45.data = fi_ahb_read(RG_XMIT_A45);
    rg_xmit_a45.b.rg_man_tx_pwrlvl = rg_xmit_a45_i.b.rg_man_tx_pwrlvl;
    rg_xmit_a45.b.rg_tx_pwr_sel_man_en = rg_xmit_a45_i.b.rg_tx_pwr_sel_man_en;
    fi_ahb_write(RG_XMIT_A45, rg_xmit_a45.data);
    // Revert the original value before calibration back
    rg_xmit_a46.data = fi_ahb_read(RG_XMIT_A46);
    rg_xmit_a46.b.rg_man_txirr_comp_theta = rg_xmit_a46_i.b.rg_man_txirr_comp_theta;
    rg_xmit_a46.b.rg_man_txirr_comp_alpha = rg_xmit_a46_i.b.rg_man_txirr_comp_alpha;
    fi_ahb_write(RG_XMIT_A46, rg_xmit_a46.data);
    RF_DPRINTF(RF_DEBUG_INFO, "WF2G check TX DC&IRR calib finish \n");
    return;
}
#endif
#endif // RF_CALI_TEST