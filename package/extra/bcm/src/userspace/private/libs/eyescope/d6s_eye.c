/*
  <:copyright-BRCM:2022:proprietary:standard
  
     Copyright (c) 2022 Broadcom 
     All Rights Reserved
  
   This program is the proprietary software of Broadcom and/or its
   licensors, and may only be used, duplicated, modified or distributed pursuant
   to the terms and conditions of a separate, written license agreement executed
   between you and Broadcom (an "Authorized License").  Except as set forth in
   an Authorized License, Broadcom grants no license (express or implied), right
   to use, or waiver of any kind with respect to the Software, and Broadcom
   expressly reserves all rights in and to the Software and all intellectual
   property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
   NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
   BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
  
   Except as expressly set forth in the Authorized License,
  
   1. This program, including its structure, sequence and organization,
      constitutes the valuable trade secrets of Broadcom, and you shall use
      all reasonable efforts to protect the confidentiality thereof, and to
      use this information only in connection with your use of Broadcom
      integrated circuit products.
  
   2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
      AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
      WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
      RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
      ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
      FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
      COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
      TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
      PERFORMANCE OF THE SOFTWARE.
  
   3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
      ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
      INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
      WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
      IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
      OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
      SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
      SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
      LIMITED REMEDY.
  :>
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h> /* for uint32_t */
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "board.h"
#include "eyescope.h"


/* #define DEBUG_STEP */
/* #define DEBUG_EYE */

typedef enum
{
    upper_even,
    lower_even,
    merged_even,
    upper_odd,
    lower_odd,
    merged_odd,
    merged
} latch_selection;

typedef enum
{
    comp_even,
    comp_odd,
    comp_all
} latch_comp_mode;

typedef struct
{
    uint32_t rx_wordmode;
    uint32_t burst_sel;
    uint32_t ranging_en;
    uint32_t intrate;
    uint32_t eyecenter;
    uint32_t steps_per_fdrUI;
    uint32_t phase_steps_per_rotation;
    uint32_t fullphaseres;
    uint32_t steps_per_ui;
    uint32_t afe_step;
    uint32_t contour_depth;
    uint32_t caldepth;
    uint32_t totalbitsperword;
    uint32_t compbitsperword;
    uint32_t minbits;
    uint32_t numbers;
    uint32_t wordspercount;
    uint32_t error_thr;
    int max_step_amp; /* to be calculated with negative numbers */
    int centervoffset, maxvoffset, minvoffset;
    int centerphase;
    int current_phase;
    float osrate;
    float bit_oversample, bit_oversample_0;
    float span, span_offset;
    latch_selection latch;
} eye_globals;

typedef struct
{
    uint32_t addr;
    uint32_t mask;
} eye_scope_register;

typedef struct
{
    float ber;
    char *dot;
} eye_dot;

typedef struct
{
    uint32_t _dp_ef_comp_sel;
    uint32_t _dp_ef_search_error_thr;
    uint32_t _dp_ef_sampcnt_thr_15_0;
    uint32_t _dp_ef_sampcnt_thr_31_16;
    uint32_t _dp_ef_search_sel;
    uint32_t _dp_ef_ph_rotate_dis;
    uint32_t _dp_ef_rotate_step;
    uint32_t _dp_ef_bca_fom_ena;
    uint32_t _rx_disable_continual_dfe_mode;
    uint32_t _qskew_walk_done_byp;
    uint32_t _pinfo_rx_burst_enao;
    uint32_t _pinfo_rx_burst_ena;
    uint32_t _pinfo_rx_burst_selo;
    uint32_t _pinfo_rx_burst_sel;
    uint32_t _pinfo_rx_wordmodeo;
    uint32_t _pinfo_rx_wordmode;
    uint32_t _pinfo_rx_ranging_enao;
    uint32_t _pinfo_rx_ranging_ena;
} backup_eye_regs;

static int initialize_eye_scope_hardware(void);
static void send_eyescope_request(uint32_t hardware, uint32_t *received, uint32_t *granted);
static void backup_registers(void);
static void restore_registers(void);
static int mk_list(uint32_t sf, int *phase_start, int *phase_step, int *phase_end,
             int *voffset_start, int *voffset_step, int *voffset_end);
static int get_eye_bers(uint32_t contour_depth, int phase_start, int phase_step, int phase_end,
                  int voffset_start, int voffset_step, int voffset_end, float ***peye);
static int get_vert_bers(int voffset_start, int voffset_step, int voffset_end, uint32_t contour_depth, int startingphaseloc, float *ber);
static int get_vert_slice(int voffset_start, int voffset_step, int voffset_end, uint32_t contour_depth, uint16_t *err_slice, uint32_t *cnt_slice);
static void set_compare_mask(latch_comp_mode mode);
static void vlatch(int voffset);
static void phase_adjust(int phase);
static int get_ber(uint32_t contour_depth, uint32_t *error_count, uint32_t *bit_count);
static int get_counts(uint32_t contour_depth, uint32_t compbitsperword, uint32_t *error_count, uint32_t *bit_count);
static int set_resolution(uint32_t contour_depth, uint32_t compbitsperword);
static float my_floor(float f);
static float my_ceil(float f);
static void write_eye_reg(uint32_t eye_reg, uint32_t value);
static uint32_t read_eye_reg(uint32_t eye_reg);
static uint32_t mem_read32(uint32_t address);
static void mem_write32(uint32_t address, uint32_t data);

static eye_globals r;
static backup_eye_regs bak;
static int ioctl_fd;

static eye_scope_register reg[] =
{
    {0x80110164 , 0x0020}, /* 00 - cfg_sgb_serdes_test_cap_eye_stop [05:05] */
    {0x8011A490 , 0x0007}, /* 01 - rxd_bsm_os_sel_rx1               [02:00] */
    {0x8011A490 , 0x0038}, /* 02 - rxd_bsm_os_sel_rx2               [05:03] */
    {0x8011A490 , 0x01C0}, /* 03 - rxd_bsm_os_sel_rx3               [08:06] */
    {0x80118A30 , 0x0006}, /* 04 - pinfo_rx_burst_sel               [02:01] */
    {0x801183A4 , 0x000F}, /* 05 - rx_vreg_vrset_g1                 [03:00] */
    {0x801183A4 , 0x00F0}, /* 06 - rx_vreg_vrset_g2                 [07:04] */
    {0x801183A4 , 0x0F00}, /* 07 - rx_vreg_vrset_g3                 [11:08] */
    {0x801183A4 , 0xF000}, /* 08 - rx_vreg_vrset_g4                 [15:12] */
    {0x801183A8 , 0x000F}, /* 09 - rx_vreg_vrset_g5                 [03:00] */
    {0x80118A2C , 0x8000}, /* 10 - pinfo_rx_ranging_ena             [15:15] */
    {0x80118E2C , 0x0C00}, /* 11 - dp_ef_uplow_sel                  [11:10] */
    {0x80118E04 , 0x00E0}, /* 12 - dp_ef_comp_sel                   [07:05] */
    {0x80118E84 , 0xFFFF}, /* 13 - dp_ef_errorcount                 [15:00] */
    {0x80118E7C , 0xFFFF}, /* 14 - dp_ef_samplecount_15_0           [15:00] */
    {0x80118E80 , 0xFFFF}, /* 15 - dp_ef_samplecount_31_16          [15:00] */
    {0x80118E00 , 0x0010}, /* 16 - dp_ef_run                        [04:04] */
    {0x80118E00 , 0x0001}, /* 17 - dp_ef_reset_n                    [00:00] */
    {0x80118E00 , 0x0040}, /* 18 - dp_ef_count_sel                  [06:06] */
    {0x80118E04 , 0x7C00}, /* 19 - dp_ef_prescaler                  [14:10] */
    {0x80118E2C , 0x03FF}, /* 20 - dp_ef_vwmargin                   [09:00] */
    {0x8011813C , 0x007F}, /* 21 - cdr_rskew_reg                    [06:00] */
    {0x80118124 , 0x0010}, /* 22 - cdr_skew_ena_reg                 [04:04] */
    {0x80118E68 , 0x07C0}, /* 23 - dp_ef_cdr_roam_sel               [10:06] */
    {0x80118E00 , 0x1000}, /* 24 - dp_ef_pitap_ena                  [12:12] */
    {0x80118E68 , 0x003F}, /* 25 - dp_ef_pitap                      [05:00] */
    {0x80118B24 , 0x2000}, /* 26 - pinfo_rx_burst_selo              [13:13] */
    {0x80118A10 , 0x000E}, /* 27 - pinfo_rx_wordmode                [03:01] */
    {0x80118B10 , 0x0002}, /* 28 - pinfo_rx_wordmodeo               [01:01] */
    {0x80118B24 , 0x0800}, /* 29 - pinfo_rx_ranging_enao            [11:11] */
    {0x80118A30 , 0x0001}, /* 30 - pinfo_rx_burst_ena               [00:00] */
    {0x80118B24 , 0x1000}, /* 31 - pinfo_rx_burst_enao              [12:12] */
    {0x80118E00 , 0x2000}, /* 32 - dp_ef_fixed_ptap_period          [13:13] */
    {0x80118E00 , 0x0800}, /* 33 - dp_ef_align_dis                  [11:11] */
    {0x80118E00 , 0x0400}, /* 34 - dp_ef_ph_rotate_dis              [10:10] */
    {0x80118E00 , 0x0200}, /* 35 - dp_ef_rotate_step                [09:09] */
    {0x80118E00 , 0x0100}, /* 36 - dp_ef_search_sel                 [08:08] */
    {0x80118E00 , 0x0080}, /* 37 - dp_ef_data_snap_ena              [07:07] */
    {0x80118E00 , 0x0020}, /* 38 - dp_ef_stop                       [05:05] */
    {0x80118E00 , 0x0008}, /* 39 - dp_ef_selpn                      [03:03] */
    {0x80118E00 , 0x0004}, /* 40 - dp_ef_search_reg_ena             [02:02] */
    {0x80118E00 , 0x0002}, /* 41 - dp_ef_ena                        [01:01] */
    {0x80118E04 , 0x0300}, /* 42 - dp_ef_rxdata_sel                 [09:08] */
    {0x80118E04 , 0x001F}, /* 43 - dp_ef_skew_sel                   [04:00] */
    {0x80118E08 , 0x01FF}, /* 44 - dp_ef_search_error_thr           [08:00] */
    {0x80118E14 , 0xFFFF}, /* 45 - dp_ef_bit_desel_15_0             [15:00] */
    {0x80118E18 , 0xFFFF}, /* 46 - dp_ef_bit_desel_31_16            [15:00] */
    {0x80118E1C , 0xFFFF}, /* 47 - dp_ef_sampcnt_thr_15_0           [15:00] */
    {0x80118E20 , 0xFFFF}, /* 48 - dp_ef_sampcnt_thr_31_16          [15:00] */
    {0x80118E6C , 0x0002}, /* 49 - dp_ef_eserrtrgr                  [01:01] */
    {0x80118E6C , 0x0001}, /* 50 - dp_ef_search_abort               [00:00] */
    {0x80118E6C , 0x0004}, /* 51 - dp_ef_search_fom_abort           [02:02] */
    {0x80118E6C , 0x0008}, /* 52 - dp_ef_search_rx_margin_abort     [03:03] */
    {0x80118E74 , 0x0001}, /* 53 - dp_ef_bca_fom_ena                [00:00] */
    {0x80118344 , 0x0002}, /* 54 - fom_eqfrz_byp                    [01:01] */
    {0x80118E70 , 0x0000}, /* 55 - dp_ef_h1ena_reg                  [00:00] */
    {0x801180C8 , 0x0100}, /* 56 - rx_disable_continual_dfe_mode    [08:08] */
    {0x80118344 , 0x0010}, /* 57 - qskew_walk_done_byp              [04:04] */
    {0x80118A14 , 0x0040}, /* 58 - pinfo_rx_eqfrz                   [06:06] */
    {0x80118B14 , 0x0004}, /* 59 - pinfo_rx_eqfrzo                  [02:02] */
    {0x801180C8 , 0x0200}, /* 60 - rx_use_pins_for_eqctrl           [09:09] */
    {0x80118008 , 0x00F0}, /* 61 - rxtx_cdr_testmux_select          [07:04] */
    {0x80118008 , 0xFF00}, /* 62 - rxtx_ctrl_testmux_select         [15:08] */
    {0x8011800C , 0x000F}, /* 63 - rxtx_pvt                         [03:00] */
    {0x801181E4 , 0x007F}, /* 64 - cdr_qskew_reg_g5_slow            [06:00] */
    {0x801181E0 , 0x007F}, /* 65 - cdr_qskew_reg_g5_fast            [06:00] */
    {0x801181E0 , 0x3F80}, /* 66 - cdr_qskew_reg_g5_nom             [13:07] */
    {0x80118130 , 0x007F}, /* 67 - cdr_qskew_reg_g4_slow            [06:00] */
    {0x8011812C , 0x007F}, /* 68 - cdr_qskew_reg_g4_fast            [06:00] */
    {0x8011812C , 0x3F80}, /* 69 - cdr_qskew_reg_g4_nom             [13:07] */
    {0x80118138 , 0x007F}, /* 70 - cdr_qskew_reg_g3_slow            [06:00] */
    {0x80118134 , 0x007F}, /* 71 - cdr_qskew_reg_g3_fast            [06:00] */
    {0x80118134 , 0x3F80}, /* 72 - cdr_qskew_reg_g3_nom             [13:07] */
    {0x80118128 , 0x007F}, /* 73 - cdr_qskew_reg_g2                 [06:00] */
    {0x80118128 , 0x3F80}, /* 74 - cdr_qskew_reg_g1                 [13:07] */
    {0x80118B4C , 0xFFFF}, /* 75 - pin_serdes_dig_testmux_status    [15:00] */
    {0x80118EBC , 0x0001}, /* 76 - dp_ef_search_hw_req              [00:00] */
    {0x80118EBC , 0x0002}, /* 77 - dp_ef_search_non_hw_req          [01:01] */
    {0x80118EBC , 0x0004}, /* 78 - dp_ef_search_gnt                 [02:02] */
};

#if defined(DEBUG_EYE)
#define DBG_EYE(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG_EYE(fmt, ...)
#endif

#define NO_IDX 0x7fffffff
/*--- reg addresses ---------------------*/
#define cfg_sgb_serdes_test_cap_eye_stop 0
#define rxd_bsm_os_sel_rx1               1
#define rxd_bsm_os_sel_rx2               2
#define rxd_bsm_os_sel_rx3               3
#define pinfo_rx_burst_sel               4
#define rx_vreg_vrset_g1                 5
#define rx_vreg_vrset_g2                 6
#define rx_vreg_vrset_g3                 7
#define rx_vreg_vrset_g4                 8
#define rx_vreg_vrset_g5                 9
#define pinfo_rx_ranging_ena             10
#define dp_ef_uplow_sel                  11
#define dp_ef_comp_sel                   12
#define dp_ef_errorcount                 13
#define dp_ef_samplecount_15_0           14
#define dp_ef_samplecount_31_16          15
#define dp_ef_run                        16
#define dp_ef_reset_n                    17
#define dp_ef_count_sel                  18
#define dp_ef_prescaler                  19
#define dp_ef_vwmargin                   20
#define cdr_rskew_reg                    21
#define cdr_skew_ena_reg                 22
#define dp_ef_cdr_roam_sel               23
#define dp_ef_pitap_ena                  24
#define dp_ef_pitap                      25
#define pinfo_rx_burst_selo              26
#define pinfo_rx_wordmode                27
#define pinfo_rx_wordmodeo               28
#define pinfo_rx_ranging_enao            29
#define pinfo_rx_burst_ena               30
#define pinfo_rx_burst_enao              31
#define dp_ef_fixed_ptap_period          32
#define dp_ef_align_dis                  33
#define dp_ef_ph_rotate_dis              34
#define dp_ef_rotate_step                35
#define dp_ef_search_sel                 36
#define dp_ef_data_snap_ena              37
#define dp_ef_stop                       38
#define dp_ef_selpn                      39
#define dp_ef_search_reg_ena             40
#define dp_ef_ena                        41
#define dp_ef_rxdata_sel                 42
#define dp_ef_skew_sel                   43
#define dp_ef_search_error_thr           44
#define dp_ef_bit_desel_15_0             45
#define dp_ef_bit_desel_31_16            46
#define dp_ef_sampcnt_thr_15_0           47
#define dp_ef_sampcnt_thr_31_16          48
#define dp_ef_eserrtrgr                  49
#define dp_ef_search_abort               50
#define dp_ef_search_fom_abort           51
#define dp_ef_search_rx_margin_abort     52
#define dp_ef_bca_fom_ena                53
#define fom_eqfrz_byp                    54
#define dp_ef_h1ena_reg                  55
#define rx_disable_continual_dfe_mode    56
#define qskew_walk_done_byp              57
#define pinfo_rx_eqfrz                   58
#define pinfo_rx_eqfrzo                  59
#define rx_use_pins_for_eqctrl           60
#define rxtx_cdr_testmux_select          61
#define rxtx_ctrl_testmux_select         62
#define rxtx_pvt                         63
#define cdr_qskew_reg_g5_slow            64
#define cdr_qskew_reg_g5_fast            65
#define cdr_qskew_reg_g5_nom             66
#define cdr_qskew_reg_g4_slow            67
#define cdr_qskew_reg_g4_fast            68
#define cdr_qskew_reg_g4_nom             69
#define cdr_qskew_reg_g3_slow            70
#define cdr_qskew_reg_g3_fast            71
#define cdr_qskew_reg_g3_nom             72
#define cdr_qskew_reg_g2                 73
#define cdr_qskew_reg_g1                 74
#define pin_serdes_dig_testmux_status    75
#define dp_ef_search_hw_req              76
#define dp_ef_search_non_hw_req          77
#define dp_ef_search_gnt                 78

#define D6S_MIN(X,Y) ((X) < (Y)) ? (X) : (Y)
#define D6S_MAX(X,Y) ((X) > (Y)) ? (X) : (Y)

#define EYESCOPE_LOCK_FILE          "/var/eyescope_lock"

static pthread_mutex_t scan_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef EINTR
static int _file_lock(int fd, int op)
{
    int ret;

    do {
        ret = lockf(fd, op, 0);
    } while (ret < 0 && errno == EINTR);

    return ret;
}
#else
#define _file_lock(a,b) lockf(a,b,0)
#endif

static int file_lock(int fd)
{
    return _file_lock(fd, F_LOCK);
}

static int file_unlock(int fd)
{
    return _file_lock(fd, F_ULOCK);
}

static int eyescope_lock(void)
{
    int fd;
    int ret_file_lock = -1;
    int ret_mutex_lock = -1;

    if ((fd = open(EYESCOPE_LOCK_FILE, O_CREAT | O_RDWR, S_IRUSR|S_IWUSR)) == -1)
    {
        printf("failed to open lock file %s error %d", EYESCOPE_LOCK_FILE, errno);
        return -1;
    }

    ret_file_lock = file_lock(fd);
    if (ret_file_lock != 0)
        goto Error;

    ret_mutex_lock = pthread_mutex_lock(&scan_mutex);
    if (ret_mutex_lock != 0)
        goto Error;

    if (ret_file_lock == 0 && ret_mutex_lock == 0)
        return fd;

Error:
    if (ret_file_lock == 0)
        file_unlock(fd);
    if (ret_mutex_lock == 0)
        pthread_mutex_unlock(&scan_mutex);

    close(fd);
    return -1;
}

static int eyescope_unlock(int fd)
{
    int ret;
    
    ret = file_unlock(fd);
    pthread_mutex_unlock(&scan_mutex);
    close(fd);

    return ret;

}

int eyescope_free(eyescope_params_t *params)
{
    int i;

    if (params->eye == NULL)
        return 0;

    for (i = 0; i <= (params->phase_end - params->phase_start) / params->phase_step; i++)
    {
        if ((params->eye)[i] != NULL)
        {
            free((params->eye)[i]);
            (params->eye)[i] = NULL;
        }
    }

    free(params->eye);
    params->eye = NULL;
    return 0;
}

static int eyescope_calc(eyescope_params_t *params)
{
    int x, y, x0, y0;
    int left = 0, right = 0;
    int top = 0, bottom = 0;
    float ber0 =  1.0 / params->depth;

    params->x_size = (params->phase_end - params->phase_start) / params->phase_step + 1;
    params->y_size = (params->voffset_end - params->voffset_start) / params->voffset_step + 1;
    x0 = params->x_size / 2;
    y0 = params->y_size / 2;

    /* from left to right (phase) */
    for (x = 0; x < params->x_size ; x++)
    {
        if (left == 0 && (params->eye)[x][y0] < ber0)
            left = x;

        if ((params->eye)[x][y0] < ber0)
            right = x;
    }

    /* from bottom to top (voffset) */
    for (y = 0; y < params->y_size; y++)
    {
        if (bottom == 0 && (params->eye)[x0][y] < ber0)
            bottom = y;

        if ((params->eye)[x0][y] < ber0)
            top = y;
    }

    left = params->phase_start + left * params->phase_step;
    right = params->phase_start + right * params->phase_step;
    top = params->voffset_start + top * params->voffset_step;
    bottom = params->voffset_start + bottom * params->voffset_step;

    params->left = left / 32.0 / params->osrate;
    params->right = right / 32.0 / params->osrate;
    params->top = top;
    params->bottom = bottom;

    return 0;
}

int eyescope_scan(eyescope_params_t *params)
{
    uint32_t sf = 4;
    int ret = 0;
    int lock_fd;

    DBG_EYE("\n\n\nHello from eyescope_scan() !!!\n\n\n");

    lock_fd = eyescope_lock();
    if (lock_fd == -1)
    {
        fprintf(stderr, "\n\n\neyescope library is busy. try again later\n");
        return EXIT_FAILURE;
    }

    ioctl_fd = open(BOARD_DEVICE_NAME, O_RDWR);
    if (0 > ioctl_fd)
    {
        fprintf(stderr, "\n\n\nCan't open `%s`\n", BOARD_DEVICE_NAME);
        ret = -1;
        goto Exit;
    }

    r.latch = merged;
    r.numbers = r.compbitsperword = 1;
    r.fullphaseres = 0;
    r.phase_steps_per_rotation = r.fullphaseres ? 128 : 64;
    r.steps_per_fdrUI = r.phase_steps_per_rotation >> 1;
    r.centervoffset = 0;
    r.centerphase = 0;
    r.current_phase = 0;
    r.contour_depth = 100000000; /* 1e-8 */
    r.caldepth      = 100000000; /* 1e-8 */
    r.totalbitsperword = 32;
    r.minbits = 1000000; /* 1e6 */
    r.bit_oversample = 3.0;
    r.bit_oversample_0 = 3.0;
    r.maxvoffset = 511;
    r.minvoffset = -511;
    r.span = 1.2;
    r.span_offset = 0;
    r.rx_wordmode = 5;
    r.ranging_en = 0;
    r.error_thr = 16;

    if (params->depth)
    {
        r.contour_depth = params->depth;
        r.caldepth = params->depth;
    }

    ret = initialize_eye_scope_hardware();
    if (ret)
        goto Exit;

    ret = mk_list(sf, &params->phase_start, &params->phase_step, &params->phase_end,
        &params->voffset_start, &params->voffset_step, &params->voffset_end);
    if (ret)
        goto Exit;

    if (params->scan_type == EYESCOPE_SCAN_HORIZONTAL)
    {
        params->voffset_start = 0;
        params->voffset_step = 1;
        params->voffset_end = 0;
    }

    if (params->scan_type == EYESCOPE_SCAN_VERTICAL)
    {
        params->phase_start = 0;
        params->phase_step = 1;
        params->phase_end = 0;
    }

    params->osrate = r.osrate;

    ret = get_eye_bers(r.contour_depth, params->phase_start, params->phase_step, params->phase_end,
        params->voffset_start, params->voffset_step, params->voffset_end, &params->eye);
    if (ret)
        goto Exit;

    ret = eyescope_calc(params);

Exit:
    if (ret)
        eyescope_free(params);
    phase_adjust(r.centerphase);
    restore_registers();
    if (ioctl_fd >= 0)
        close(ioctl_fd);
    eyescope_unlock(lock_fd);

    return ret;
}

static int initialize_eye_scope_hardware(void)
{
    uint32_t burstrate, ranging_ena, os_sel, vrset, gshadow;
    uint32_t frzo_start, frzf_start, eqctrl_start;
    uint32_t cdr_testmux, ctrl_testmux, qskew, qskewa, qskewreg;
    uint32_t received=0, granted=0;
    int rxtx_pvt_val, qskew_signed;

    write_eye_reg(cfg_sgb_serdes_test_cap_eye_stop, 1);
    write_eye_reg(cfg_sgb_serdes_test_cap_eye_stop, 0);

    r.burst_sel = read_eye_reg(pinfo_rx_burst_sel);
    burstrate = r.burst_sel + 1;
    ranging_ena = read_eye_reg(pinfo_rx_ranging_ena);

    switch(burstrate)
    {
        case 1:
            os_sel = read_eye_reg(rxd_bsm_os_sel_rx1);
            gshadow = ranging_ena ? 4 : 5;
            vrset = ranging_ena ? read_eye_reg(rx_vreg_vrset_g4) : read_eye_reg(rx_vreg_vrset_g5);
            break;
        case 2:
            os_sel = read_eye_reg(rxd_bsm_os_sel_rx2);
            gshadow = ranging_ena ? 2 : 3;
            vrset = ranging_ena ? read_eye_reg(rx_vreg_vrset_g2) : read_eye_reg(rx_vreg_vrset_g3);
            break;
        case 3:
            os_sel = read_eye_reg(rxd_bsm_os_sel_rx3);
            gshadow = 1;
            vrset = read_eye_reg(rx_vreg_vrset_g1);
            break;
        default:
            fprintf(stderr, "\n\n\nwrong burstrate %d\n", burstrate);
            return (EXIT_FAILURE + 1);
    }
    r.afe_step = (vrset*15.0 + 890.0) / 1024;

    rxtx_pvt_val = read_eye_reg(rxtx_pvt);
    if (rxtx_pvt_val > 7)
        rxtx_pvt_val -= 16; /* 4-bit 2's complement */
    switch(gshadow)
    {
        case 5:
            if (rxtx_pvt_val < -4)
                qskewreg = cdr_qskew_reg_g5_slow;
            else if (rxtx_pvt_val > 3)
                qskewreg = cdr_qskew_reg_g5_fast;
            else
                qskewreg = cdr_qskew_reg_g5_nom;
            break;
        case 4:
            if (rxtx_pvt_val < -4)
                qskewreg = cdr_qskew_reg_g4_slow;
            else if (rxtx_pvt_val > 3)
                qskewreg = cdr_qskew_reg_g4_fast;
            else
                qskewreg = cdr_qskew_reg_g4_nom;
            break;
        case 3:
            if (rxtx_pvt_val < -4)
                qskewreg = cdr_qskew_reg_g3_slow;
            else if (rxtx_pvt_val > 3)
                qskewreg = cdr_qskew_reg_g3_fast;
            else
                qskewreg = cdr_qskew_reg_g3_nom;
            break;
        case 2:
            qskewreg = cdr_qskew_reg_g2;
            break;
        case 1:
            qskewreg = cdr_qskew_reg_g1;
            break;
        default:
            fprintf(stderr, "\n\n\nwrong gshadow %d\n", burstrate);
            return (EXIT_FAILURE + 1);
    }

    switch(os_sel)
    {
        case 0:
            r.osrate = 1.0;
            r.eyecenter = 0;
            r.max_step_amp = 31;
            break;
        case 1:
            r.osrate = 2.0;
            r.eyecenter = 0;
            r.max_step_amp = 127;
            break;
        case 3:
            r.osrate = 8.0;
            r.eyecenter = 0;
            r.max_step_amp = 239;
            break;
        case 4:
            r.osrate = 16.0;
            r.eyecenter = 0;
            r.max_step_amp = 319;
            break;
        case 5:
            r.osrate = 16.5;
            r.eyecenter = 1;
            r.max_step_amp = 319;
            break;
        case 6:
            r.osrate = 2.5;
            r.eyecenter = 1;
            r.max_step_amp = 111;
            break;
        default:
            fprintf(stderr, "\n\n\nwrong os_sel %d\n", os_sel);
            return (EXIT_FAILURE + 1);
    }

    r.intrate = (uint32_t)r.osrate;
    r.steps_per_ui = (uint32_t)(r.steps_per_fdrUI * r.osrate);

    if ((1 == r.osrate && (merged_even == r.latch || merged_odd == r.latch || merged == r.latch))
        || (merged == r.latch && (16.5 == r.osrate || 2.5 == r.osrate)))
        r.numbers = 2;
    else
        r.numbers = 1;

    backup_registers();

    write_eye_reg(pinfo_rx_burst_sel, r.burst_sel);
    write_eye_reg(pinfo_rx_burst_selo, 1);
    write_eye_reg(pinfo_rx_wordmode, r.rx_wordmode);
    write_eye_reg(pinfo_rx_wordmodeo, 1);
    write_eye_reg(pinfo_rx_ranging_ena, r.ranging_en);
    write_eye_reg(pinfo_rx_ranging_enao, 1);
    write_eye_reg(pinfo_rx_burst_ena, 0);
    write_eye_reg(pinfo_rx_burst_enao, 1);
    write_eye_reg(pinfo_rx_burst_ena, 1);

    write_eye_reg(dp_ef_pitap, 0);
    write_eye_reg(cdr_rskew_reg, 0);
    write_eye_reg(dp_ef_reset_n, 0); /* Apply a soft reset to initialize the EyeScope logic */
    /* Register DP_EYE_RW_0 - The entire register block could be written at once to save processing time.
       bits [15:14] are reserved and should be 0 */
    write_eye_reg(dp_ef_fixed_ptap_period, 1);
    write_eye_reg(dp_ef_pitap_ena, 0);
    write_eye_reg(dp_ef_align_dis, 1);
    write_eye_reg(dp_ef_ph_rotate_dis, 1);
    write_eye_reg(dp_ef_rotate_step, 0);
    write_eye_reg(dp_ef_search_sel, 0);
    write_eye_reg(dp_ef_data_snap_ena, 0);
    write_eye_reg(dp_ef_count_sel, 0);
    write_eye_reg(dp_ef_stop, 0);
    write_eye_reg(dp_ef_run, 0);
    write_eye_reg(dp_ef_selpn, 1);
    write_eye_reg(dp_ef_search_reg_ena, 0);
    write_eye_reg(dp_ef_ena, 0);
    write_eye_reg(dp_ef_reset_n, 1);
    /* Initialize the common firmware and hardware register settings */
    write_eye_reg(dp_ef_rxdata_sel, 0);
    write_eye_reg(dp_ef_skew_sel, 0);
    write_eye_reg(dp_ef_search_error_thr, r.error_thr); /* hardware eyescope only, 0 to 256 errors */
    write_eye_reg(dp_ef_bit_desel_15_0, 0);
    write_eye_reg(dp_ef_bit_desel_31_16, 0);
    write_eye_reg(dp_ef_sampcnt_thr_15_0, 0xFFFF);
    write_eye_reg(dp_ef_sampcnt_thr_31_16, 0xFFFF);
    write_eye_reg(dp_ef_eserrtrgr, 1);
    write_eye_reg(dp_ef_search_abort, 1);
    write_eye_reg(dp_ef_search_fom_abort, 1);
    write_eye_reg(dp_ef_search_rx_margin_abort, 1);
    write_eye_reg(dp_ef_uplow_sel, 3);
    write_eye_reg(dp_ef_bca_fom_ena, 0); /* 0 = normal operation (all bits qualified) 1 = D7S investigation, ELK TEST */
    write_eye_reg(dp_ef_cdr_roam_sel, 0); /* quadrant == 0 on init time */
    /* keep the eyescope state machine from freezing the dfe when it grants access */
    write_eye_reg(fom_eqfrz_byp, 1); /* rx_margin_eqfrz_byp */

    /* Registers specific to firmware eyescope */
    if (1 == r.osrate)
    {
        write_eye_reg(dp_ef_h1ena_reg, 1); /* 1 = normal operation (apply h1 to roam), 0 = do not apply h1 to roam */
        write_eye_reg(dp_ef_vwmargin, 0);
    }
    else
    {
        write_eye_reg(dp_ef_h1ena_reg, 0); /* h1 not unrolled so don't apply h1 */
        write_eye_reg(dp_ef_vwmargin, 0);
    }

    /* Freeze the DFE adaptation, if called for. The initial adapatation states will be recorded for
       restoration after the EyeScope measurements are complete (restore_EyeScope_Hardware function).
       if self.disable_periodic_adaptation:  <==== always true */
    write_eye_reg(rx_disable_continual_dfe_mode, 1);
    write_eye_reg(qskew_walk_done_byp, 1);

    /* if not self.eye_dfe_freeze: #freeze control is off so it it may be adapting.  Need to teporarily freeze to get correlated results */
    frzo_start = read_eye_reg(pinfo_rx_eqfrzo);
    frzf_start = read_eye_reg(pinfo_rx_eqfrz);                
    if (!frzf_start)
        write_eye_reg(pinfo_rx_eqfrz, 1);
    if (!frzo_start)
        write_eye_reg(pinfo_rx_eqfrzo, 1);
    eqctrl_start = read_eye_reg(rx_use_pins_for_eqctrl);
    if (!eqctrl_start)
        write_eye_reg(rx_use_pins_for_eqctrl, 1);


    cdr_testmux = read_eye_reg(rxtx_cdr_testmux_select);
    ctrl_testmux = read_eye_reg(rxtx_ctrl_testmux_select);
    write_eye_reg(rxtx_cdr_testmux_select, 0xA);
    write_eye_reg(rxtx_ctrl_testmux_select, 0xC);
    qskewa = (read_eye_reg(pin_serdes_dig_testmux_status) >> 7) & 0x7F;
    write_eye_reg(rxtx_cdr_testmux_select, cdr_testmux);
    write_eye_reg(rxtx_ctrl_testmux_select, ctrl_testmux);
    if (0 == (qskewa & 0x20))
        qskew = qskewa;
    else
        qskew = ((~qskewa) & 0x1F) | (qskewa & 0x60);
    if (63 < qskew)
        qskew_signed = qskew - 128;
    else
        qskew_signed = qskew; /* 7-bit twos complement */
    write_eye_reg(qskewreg, qskew_signed & 0x7F);

    if (!eqctrl_start)
        write_eye_reg(rx_use_pins_for_eqctrl, eqctrl_start);
    if (!frzo_start)
        write_eye_reg(pinfo_rx_eqfrzo, frzo_start);
    if (!frzf_start)
        write_eye_reg(pinfo_rx_eqfrz, frzf_start);

    if (upper_even == r.latch || lower_even == r.latch || merged_even == r.latch)
        set_compare_mask(comp_even);
    else if (upper_odd == r.latch || lower_odd == r.latch || merged_odd == r.latch)
        set_compare_mask(comp_odd);
    else
        set_compare_mask(comp_all);

    send_eyescope_request(0, &received, &granted);
    if (!received || !granted)
    {
        fprintf(stderr, "\n\n\nsend_eyescope_request failed! received=%d, granted=%d\n", received, granted);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void send_eyescope_request(uint32_t hardware, uint32_t *received, uint32_t *granted)
{
#define REQ_TIMEOUT_RTRY 20 /* 2[Sec] */

    uint32_t received_reg, to_cnt;

    *received = *granted = 0;

    if (hardware)
        received_reg = dp_ef_search_hw_req;
    else
        received_reg = dp_ef_search_non_hw_req;

    write_eye_reg(dp_ef_search_reg_ena, hardware);
    write_eye_reg(dp_ef_ena, !hardware);

    to_cnt = 0;
    while (!*received && to_cnt++ < REQ_TIMEOUT_RTRY)
    {
        *received = read_eye_reg(received_reg);
        usleep(100000);
    }

    if (*received)
    {
        to_cnt = 0;
        while (!*granted && to_cnt++ < REQ_TIMEOUT_RTRY)
        {
            *granted = read_eye_reg(dp_ef_search_gnt);
            usleep(100000);
        }
    }
}

static void backup_registers(void)
{
    bak._dp_ef_comp_sel = read_eye_reg(dp_ef_comp_sel);
    bak._dp_ef_search_error_thr = read_eye_reg(dp_ef_search_error_thr);
    bak._dp_ef_sampcnt_thr_15_0 = read_eye_reg(dp_ef_sampcnt_thr_15_0);
    bak._dp_ef_sampcnt_thr_31_16 = read_eye_reg(dp_ef_sampcnt_thr_31_16);
    bak._dp_ef_search_sel = read_eye_reg(dp_ef_search_sel);
    bak._dp_ef_ph_rotate_dis = read_eye_reg(dp_ef_ph_rotate_dis);
    bak._dp_ef_rotate_step = read_eye_reg(dp_ef_rotate_step);
    bak._dp_ef_bca_fom_ena = read_eye_reg(dp_ef_bca_fom_ena);
    bak._rx_disable_continual_dfe_mode = read_eye_reg(rx_disable_continual_dfe_mode);
    bak._qskew_walk_done_byp = read_eye_reg(qskew_walk_done_byp);
    bak._pinfo_rx_burst_enao = read_eye_reg(pinfo_rx_burst_enao);
    bak._pinfo_rx_burst_ena = read_eye_reg(pinfo_rx_burst_ena);
    bak._pinfo_rx_burst_selo = read_eye_reg(pinfo_rx_burst_selo);
    bak._pinfo_rx_burst_sel = read_eye_reg(pinfo_rx_burst_sel);
    bak._pinfo_rx_wordmodeo = read_eye_reg(pinfo_rx_wordmodeo);
    bak._pinfo_rx_wordmode = read_eye_reg(pinfo_rx_wordmode);
    bak._pinfo_rx_ranging_enao = read_eye_reg(pinfo_rx_ranging_enao);
    bak._pinfo_rx_ranging_ena = read_eye_reg(pinfo_rx_ranging_ena);
}

static void restore_registers(void)
{
    write_eye_reg(dp_ef_comp_sel, bak._dp_ef_comp_sel);
    write_eye_reg(dp_ef_search_error_thr, bak._dp_ef_search_error_thr);
    write_eye_reg(dp_ef_sampcnt_thr_15_0, bak._dp_ef_sampcnt_thr_15_0);
    write_eye_reg(dp_ef_sampcnt_thr_31_16, bak._dp_ef_sampcnt_thr_31_16);
    write_eye_reg(dp_ef_search_sel, bak._dp_ef_search_sel);
    write_eye_reg(dp_ef_ph_rotate_dis, bak._dp_ef_ph_rotate_dis);
    write_eye_reg(dp_ef_rotate_step, bak._dp_ef_rotate_step);
    write_eye_reg(dp_ef_bca_fom_ena, bak._dp_ef_bca_fom_ena);
    write_eye_reg(rx_disable_continual_dfe_mode, bak._rx_disable_continual_dfe_mode);
    write_eye_reg(qskew_walk_done_byp, bak._qskew_walk_done_byp);

    /* Restore the hardware into a neutral state. */
    write_eye_reg(dp_ef_search_reg_ena, 0);
    write_eye_reg(dp_ef_ena, 0);
    /* Clear all the status indicators. */
    write_eye_reg(dp_ef_eserrtrgr, 1);
    write_eye_reg(dp_ef_search_abort, 1);
    write_eye_reg(dp_ef_search_fom_abort, 1);
    write_eye_reg(dp_ef_search_rx_margin_abort, 1);

    /* restore burst enable and burst_sel force */
    write_eye_reg(pinfo_rx_burst_enao, bak._pinfo_rx_burst_enao);
    write_eye_reg(pinfo_rx_burst_ena, bak._pinfo_rx_burst_ena);
    write_eye_reg(pinfo_rx_burst_selo, bak._pinfo_rx_burst_selo);
    write_eye_reg(pinfo_rx_burst_sel, bak._pinfo_rx_burst_sel);
    write_eye_reg(pinfo_rx_wordmodeo, bak._pinfo_rx_wordmodeo);
    write_eye_reg(pinfo_rx_wordmode, bak._pinfo_rx_wordmode);
    write_eye_reg(pinfo_rx_ranging_enao, bak._pinfo_rx_ranging_enao);
    write_eye_reg(pinfo_rx_ranging_ena, bak._pinfo_rx_ranging_ena);
}

static int mk_list(uint32_t sf, int *phase_start, int *phase_step, int *phase_end,
             int *voffset_start, int *voffset_step, int *voffset_end)
{
    float min_range_ui, max_range_ui, sfh, tmp;
    
    switch(sf)
    {
        case 1 ... 2:
            sfh = r.fullphaseres ? 1 : ((1 == r.intrate) ? 1 : 0.5);
            *voffset_step = (1 == sf) ? 2 : 4;
            break;
        case 4:
            sfh = r.fullphaseres ? 2 : 1;
            *voffset_step = 7;
            break;
        case 8:
            sfh = r.fullphaseres ? 4 : 2;
            *voffset_step = 15;
            break;
        default:
            fprintf(stderr, "\n\n\nwrong sf %d\n", sf);
            return EXIT_FAILURE;
    }
    *phase_step = (int)(r.intrate*sfh);
    
    /* Create the roaming latch phase list now that it is assured that min and max are covered in the range
       and the centerphase is included in between the two. */
    min_range_ui = -(r.span / 2.0) + r.span_offset;
    tmp = min_range_ui * (float)r.steps_per_ui;
    *phase_start = my_floor(tmp) + r.centerphase;
    *phase_start = (*phase_start < -r.max_step_amp) ? *phase_step * (-r.max_step_amp / *phase_step) : *phase_start;

    max_range_ui = (r.span / 2.0) + r.span_offset;
    tmp = max_range_ui * (float)r.steps_per_ui;
    *phase_end = my_ceil(tmp) + r.centerphase;
    *phase_end = (*phase_end > r.max_step_amp) ? *phase_step * (r.max_step_amp / *phase_step) : *phase_end;

    *voffset_start = r.minvoffset + (r.maxvoffset % *voffset_step);
    *voffset_end = -*voffset_start;

    return EXIT_SUCCESS;
}

static int get_eye_bers(uint32_t contour_depth, int phase_start, int phase_step, int phase_end, int voffset_start, int voffset_step, int voffset_end, float ***eye)
{
    /* Example: for 1e-7 the contour_depth is 10000000 */
    int i, in_eye = NO_IDX;
    uint32_t error_count, bit_count;
    float *ber;
    int ret;

    vlatch(r.centervoffset);
    for (i=0; i >= phase_start; i-=phase_step)
    {
        phase_adjust(i);
        ret = get_ber(r.caldepth, &error_count, &bit_count);
        if (ret == EXIT_FAILURE)
            return EXIT_FAILURE;
        DBG_EYE("\n\ntest center voffset for phase %d error_count %d bit_count %d\n", i, error_count, bit_count);
        if (bit_count > error_count * 10000)
        {
            in_eye = i;
            DBG_EYE("Found low BER point (1e-4) at center voffset for phase %d error_count %d bit_count %d\n\n", i, error_count, bit_count);
            break;
        }
    }

    if (NO_IDX == in_eye)
        for (i=phase_step; i <= phase_end; i+=phase_step)
        {
            phase_adjust(i);
            ret = get_ber(r.caldepth, &error_count, &bit_count);
            if (ret == EXIT_FAILURE)
                return EXIT_FAILURE;
            DBG_EYE("test center voffset for phase %d error_count %d bit_count %d\n", i, error_count, bit_count);
            if (bit_count > error_count * 10000)
            {
                in_eye = i;
                DBG_EYE("Found low BER point (1e-4) at center voffset for phase %d error_count %d bit_count %d\n\n", i, error_count, bit_count);
                break;
            }
        }

    if (phase_start == phase_end)
        in_eye = 0;

    if (NO_IDX == in_eye)
    {
        fprintf(stderr, "\n\n\nCAN NOT find a low BER point (1e-4) at the center voffset !!!\n");
        return EXIT_FAILURE;
    }

    *eye = (float **)malloc(sizeof(float*) * ((phase_end - phase_start)/phase_step + 1));
    if (!*eye)
    {
        fprintf(stderr, "\n\n\nCan't alloc memory for the eye!\n");
        return EXIT_FAILURE;
    }

    memset(*eye, 0, sizeof(float*) * ((phase_end - phase_start)/phase_step + 1));

    DBG_EYE("\n=> Get vertical BERs for right side\n\n");
    for (i=in_eye; i <= phase_end; i+=phase_step)
    {
        ber = (float *)malloc(sizeof(float) * ((voffset_end - voffset_start)/voffset_step + 1));
        if (!ber)
            goto failure;
            
        ret = get_vert_bers(voffset_start, voffset_step, voffset_end, contour_depth, i, ber);
        if (ret == EXIT_FAILURE)
        {
            free(ber);
            goto failure;
        }
        (*eye)[(i - phase_start) / phase_step] = ber;
    }

    DBG_EYE("\n=> Get vertical BERs for left side\n\n");
    for (i=in_eye-phase_step; i >= phase_start; i-=phase_step)
    {
        ber = (float *)malloc(sizeof(float) * ((voffset_end - voffset_start)/voffset_step + 1));
        if (!ber)
            goto failure;
        
        ret = get_vert_bers(voffset_start, voffset_step, voffset_end, contour_depth, i, ber);
        if (ret == EXIT_FAILURE)
        {
            free(ber);
            goto failure;
        }
        (*eye)[(i - phase_start) / phase_step] = ber;
    }

    return EXIT_SUCCESS;

failure:
    return EXIT_FAILURE;
}

static void vlatch(int voffset)
{
    voffset = D6S_MAX(D6S_MIN(voffset, r.maxvoffset), r.minvoffset);
    if (voffset < 0)
        write_eye_reg(dp_ef_vwmargin, (-voffset) | 512);
    else
        write_eye_reg(dp_ef_vwmargin, voffset);
}

static void phase_adjust(int phase)
{
    int stepdir, i, rotation;
    uint32_t quadrant;

    if (phase == r.current_phase)
        return;
    
    stepdir = phase > r.current_phase ? 1 : -1;
    for (i = phase - r.current_phase ; i != 0 ; i -= stepdir)
    {
        r.current_phase += stepdir;
        rotation = (int)my_floor((float)r.current_phase / r.phase_steps_per_rotation);
        quadrant = rotation % 0x20;
        if (r.fullphaseres)
        {
            write_eye_reg(cdr_rskew_reg, r.current_phase & 0x7F);
            write_eye_reg(cdr_skew_ena_reg, 1);
            write_eye_reg(cdr_skew_ena_reg, 0);
            if ( (int)my_floor((float)r.current_phase / r.phase_steps_per_rotation) != (int)my_floor((float)(r.current_phase - stepdir) / r.phase_steps_per_rotation) )
            {
                write_eye_reg(dp_ef_cdr_roam_sel, quadrant);
                write_eye_reg(dp_ef_pitap_ena, 1);
                write_eye_reg(dp_ef_pitap_ena, 0);
            }
#if defined(DEBUG_STEP)
            printf("%d current_phase: %d desired_phase: %d\n", r.current_phase & 0x7F, r.current_phase, phase);
#endif
        }
        else
        {
            write_eye_reg(dp_ef_pitap, r.current_phase & 0x3F);
            if ( (int)my_floor((float)r.current_phase / r.phase_steps_per_rotation) != (int)my_floor((float)(r.current_phase - stepdir) / r.phase_steps_per_rotation) )
            {
                write_eye_reg(dp_ef_cdr_roam_sel, quadrant);
            }
            write_eye_reg(dp_ef_pitap_ena, 1);
            write_eye_reg(dp_ef_pitap_ena, 0);
#if defined(DEBUG_STEP)
            printf("%d current_phase: %d desired_phase: %d\n", r.current_phase & 0x3F, r.current_phase, phase);
#endif
        }
        usleep(10000);
    }
}

static int get_ber(uint32_t contour_depth, uint32_t *error_count, uint32_t *bit_count)
{
    *error_count = 0;
    int ret;

    if (contour_depth > r.caldepth)
    {
        ret = get_counts(r.caldepth, r.compbitsperword, error_count, bit_count);
        if (ret == EXIT_FAILURE)
            return EXIT_FAILURE;
    }
    if (!*error_count)
    {
        ret = get_counts(contour_depth, r.compbitsperword, error_count, bit_count);
        if (ret == EXIT_FAILURE)
            return EXIT_FAILURE;
    }
    
    if (!*bit_count)
    {
        printf("get_ber: error_count %d bit_count %d\n", *error_count, *bit_count);
        ret = get_counts(contour_depth, r.compbitsperword, error_count, bit_count);
        if (ret == EXIT_FAILURE)
            return EXIT_FAILURE;
        
        if (!*bit_count)
        {
            printf("get_ber: error_count %d bit_count %d\n", *error_count, *bit_count);
            fprintf(stderr, "\n\n\nCan't get bit count!\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int get_vert_bers(int voffset_start, int voffset_step, int voffset_end, uint32_t contour_depth, int startingphaseloc, float *ber)
{
    uint16_t *err_slice = NULL;
    uint32_t *cnt_slice = NULL;
    int i;
    int ret = 0;

    DBG_EYE("\n===> Get vertical BERs for phase %d\n", startingphaseloc);
    phase_adjust(startingphaseloc);

    /* step #1 */
    if (1 != r.osrate || lower_even == r.latch || lower_odd == r.latch)
        write_eye_reg(dp_ef_uplow_sel, 0);
    else
        write_eye_reg(dp_ef_uplow_sel, 3);

    if (upper_even == r.latch || lower_even == r.latch || merged_even == r.latch)
        set_compare_mask(comp_even);
    else if (upper_odd == r.latch || lower_odd == r.latch || merged_odd == r.latch)
        set_compare_mask(comp_odd);
    else if (16.5 != r.osrate && 2.5 != r.osrate)
        set_compare_mask(comp_all);
    else
        set_compare_mask(comp_even);

    err_slice = (uint16_t *)calloc((voffset_end - voffset_start)/voffset_step + 1, sizeof(uint16_t));
    cnt_slice = (uint32_t *)calloc((voffset_end - voffset_start)/voffset_step + 1, sizeof(uint32_t));
    if (!err_slice || !cnt_slice)
    {
        fprintf(stderr, "\n\n\nCan't alloc memory for a eye vertical slice!\n");
        goto failure;
    }
    ret = get_vert_slice(voffset_start, voffset_step, voffset_end, contour_depth, err_slice, cnt_slice);
    if (ret == EXIT_FAILURE)
        goto failure;

   
    for (i = 0; i < (voffset_end - voffset_start)/voffset_step + 1; i++)
    {
        ber[i] = 0;
        if (cnt_slice[i])
            ber[i] += (float)err_slice[i] / cnt_slice[i];
        DBG_EYE("%.3e, ", ber[i]);
    }
    DBG_EYE("[len %d]\n\n", i + 1);

    bzero(err_slice, ((voffset_end - voffset_start) / voffset_step + 1) * sizeof(uint16_t));
    bzero(cnt_slice, ((voffset_end - voffset_start) / voffset_step + 1) * sizeof(uint32_t));

    /* optional step #2 */
    if ((1 == r.osrate) && (merged_even == r.latch || merged_odd == r.latch || merged == r.latch))
    {
        write_eye_reg(dp_ef_uplow_sel, 0);
        ret = get_vert_slice(voffset_start, voffset_step, voffset_end, contour_depth, err_slice, cnt_slice);
        if (ret == EXIT_FAILURE)
            goto failure;
    }
    else if (merged == r.latch && (16.5 == r.osrate || 2.5 == r.osrate))
    {
        set_compare_mask(comp_odd);
        ret = get_vert_slice(voffset_start, voffset_step, voffset_end, contour_depth, err_slice, cnt_slice);
        if (ret == EXIT_FAILURE)
            goto failure;
    }

    /* calculate average */
    for (i = 0; i < (voffset_end - voffset_start)/voffset_step + 1; i++)
    {
        if (cnt_slice[i])
            ber[i] += (float)err_slice[i] / cnt_slice[i];
        ber[i] /= 2;
        DBG_EYE("%.3e, ", ber[i]);
    }
    DBG_EYE("[len %d]\n\n", i + 1);

    return EXIT_SUCCESS;

failure:
    if (NULL != err_slice)
        free(err_slice);

    if (NULL != cnt_slice)
        free(cnt_slice);

    return EXIT_FAILURE;
}

static int get_vert_slice(int voffset_start, int voffset_step, int voffset_end, uint32_t contour_depth, uint16_t *err_slice, uint32_t *cnt_slice)
{
    uint32_t error_count, bit_count;
    /* The HW error count is 16-bit */
    uint16_t *err_ptr;
    uint32_t *cnt_ptr;
    int vl1, vl2;
    int ret;

    DBG_EYE("\n=====> Get a vertical slice:\n");
    err_ptr = err_slice;
    cnt_ptr = cnt_slice;

    for (vl1 = voffset_start; vl1 <= voffset_end; vl1 += voffset_step)
    {
        vlatch(vl1);
        ret = get_ber(contour_depth, &error_count, &bit_count);
        if (ret == EXIT_FAILURE)
            goto failure;
        DBG_EYE(" =====>  BER for vlatch %d: ec %d bc %d\n", vl1, error_count, bit_count);
        *(err_ptr++) += (uint16_t)error_count;
        *(cnt_ptr++) += bit_count;
        if (!error_count)
        {
            DBG_EYE("===========>  Got no errors for vlatch1 %d\n\n", vl1);
            break;
        }
    }

    err_ptr = err_slice + (voffset_end - voffset_start)/voffset_step;
    cnt_ptr = cnt_slice + (voffset_end - voffset_start)/voffset_step;
    for (vl2 = voffset_end; vl2 > vl1; vl2 -= voffset_step)
    {
        vlatch(vl2);
        ret = get_ber(contour_depth, &error_count, &bit_count);
        if (ret == EXIT_FAILURE)
            goto failure;
        DBG_EYE(" =====>  BER for vlatch %d: ec %d bc %d\n", vl2, error_count, bit_count);
        *(err_ptr--) += (uint16_t)error_count;
        *(cnt_ptr--) += bit_count;
        if (!error_count)
        {
            DBG_EYE("===========>  Got no errors for vlatch2 %d\n\n", vl2);
            break;
        }
    }

    /* always return offset to center */
    vlatch(r.centervoffset);

    return EXIT_SUCCESS;

failure:
    return EXIT_FAILURE;
}

static void set_compare_mask(latch_comp_mode mode)
{
    if (comp_odd == mode)
        write_eye_reg(dp_ef_comp_sel, 0);
    else if (comp_even == mode)
        write_eye_reg(dp_ef_comp_sel, 1);
    else
        write_eye_reg(dp_ef_comp_sel, 3);

    if ((comp_odd == mode || comp_even == mode) && (1 == r.osrate))
        r.compbitsperword = r.totalbitsperword >> 2;
    else if (comp_all == mode && 1 != r.osrate)
        r.compbitsperword = r.totalbitsperword;
    else
        r.compbitsperword = r.totalbitsperword >> 1;
}

static int get_counts(uint32_t contour_depth, uint32_t compbitsperword, uint32_t *error_count, uint32_t *bit_count)
{
    uint32_t bit_count_min, bit_count_180, bit_count_33, bit_count_0, bit_count_max;
    uint32_t last_sample_count = 0xFFFFFFFF, sample_count = 0;
    uint32_t sample_count_low, sample_count_hi;
    int ret;

    *error_count = 0;
    *bit_count = 0;

    ret = set_resolution(contour_depth, compbitsperword);
    if (ret == EXIT_FAILURE)
        return EXIT_FAILURE;

    write_eye_reg(cfg_sgb_serdes_test_cap_eye_stop, 1);
    write_eye_reg(cfg_sgb_serdes_test_cap_eye_stop, 0);

    write_eye_reg(dp_ef_run,     0);
    write_eye_reg(dp_ef_reset_n, 0);
    write_eye_reg(dp_ef_reset_n, 1);
    write_eye_reg(dp_ef_run,     1);

    bit_count_min = D6S_MIN(r.minbits, (uint32_t)(r.bit_oversample * contour_depth));
    bit_count_180 = D6S_MAX(bit_count_min, contour_depth / 10000 * 180);
    bit_count_33 = D6S_MAX(bit_count_min, contour_depth / 100 * 33);
    bit_count_0 = (uint32_t)(r.bit_oversample_0 * contour_depth);
    bit_count_max = (uint32_t)(D6S_MAX(r.bit_oversample_0, r.bit_oversample) * contour_depth);
    
    bit_count_0 /= r.numbers;
    bit_count_max /= r.numbers;

    while ( last_sample_count != sample_count && *error_count < 0x65500 && \
            !((*error_count >= 800 && *error_count < 0x2500 && *bit_count > bit_count_min) || \
              (*error_count >= 180 && *error_count < 0x2500 && *bit_count > bit_count_180) || \
              (*error_count >= 33  && *error_count < 0x2500 && *bit_count > bit_count_33 ) || \
              (*error_count == 0                            && *bit_count > bit_count_0  ) || \
              (*bit_count > bit_count_max)) )
    {
        last_sample_count = sample_count;
        usleep(1);

        write_eye_reg(dp_ef_count_sel, 1);
        *error_count = read_eye_reg(dp_ef_errorcount);
        sample_count_low = read_eye_reg(dp_ef_samplecount_15_0);
        sample_count_hi = read_eye_reg(dp_ef_samplecount_31_16);
        write_eye_reg(dp_ef_count_sel, 0);
        sample_count = (sample_count_hi << 16) | sample_count_low;
        *bit_count = sample_count * r.wordspercount * compbitsperword;
    }

    if (!*bit_count)
    {
        *error_count = 0xFFFF;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int set_resolution(uint32_t contour_depth, uint32_t compbitsperword)
{
    float oversample;
    uint64_t desired_bits, desired_words;

    oversample = D6S_MAX(r.bit_oversample_0, r.bit_oversample);
    oversample /= r.numbers;
    desired_bits = 2.0 * oversample * contour_depth;

    desired_words = desired_bits / compbitsperword;
    if (desired_words < (uint64_t)0xFFFFFFFF)
        write_eye_reg(dp_ef_prescaler, 0);
    else
    {
        fprintf(stderr, "\n\n\nwrong desired_words 0x%016llX\n", (long long)desired_words);
        return EXIT_FAILURE;
    }
    r.wordspercount = 1;

    return EXIT_SUCCESS;
}

static float my_floor(float f)
{
    float tmp;

    tmp = (float)((int)f);
    if (tmp == f)
        return f;
    
    if (f < 0)
        return tmp - 1;
    
    return tmp;
}

static float my_ceil(float f)
{
    float tmp;

    tmp = (float)((int)f);
    if (tmp == f)
        return f;
    
    if (f > 0)
        return tmp + 1;
    
    return tmp;
}

static void write_eye_reg(uint32_t eye_reg, uint32_t value)
{
    uint32_t mult, data;
    mult = reg[eye_reg].mask & ~(reg[eye_reg].mask << 1);
    data = mem_read32(reg[eye_reg].addr);
    data &= ~reg[eye_reg].mask;
    data |= value * mult;
    mem_write32(reg[eye_reg].addr, data);
}

static uint32_t read_eye_reg(uint32_t eye_reg)
{
    uint32_t mult, value;

    mult = reg[eye_reg].mask & ~(reg[eye_reg].mask << 1);
    value = mem_read32(reg[eye_reg].addr);
    return (value & reg[eye_reg].mask) / mult;
}

static uint32_t mem_read32(uint32_t address)
{
    BOARD_MEMACCESS_IOCTL_PARMS parms;
    char buf[4];
    uint32_t value;

    parms.address = address;
    parms.size = 4;
    parms.space = BOARD_MEMACCESS_IOCTL_SPACE_REG;
    parms.count = 1;
    parms.buf = buf;
    parms.op = BOARD_MEMACCESS_IOCTL_OP_READ;
    ioctl(ioctl_fd, BOARD_IOCTL_MEM_ACCESS, &parms);
    value = *(uint32_t *)buf;

    DBG_EYE("Read 0x%08X from 0x%08X\n", value, address);
    
    return value;
}

static void mem_write32(uint32_t address, uint32_t data)
{
    BOARD_MEMACCESS_IOCTL_PARMS parms;
    char buf[4];

    parms.address = address;
    parms.size = 4;
    parms.space = BOARD_MEMACCESS_IOCTL_SPACE_REG;
    parms.count = 1;
    *(uint32_t *)buf = data;
    parms.buf = buf;
    parms.op = BOARD_MEMACCESS_IOCTL_OP_WRITE;
    ioctl(ioctl_fd, BOARD_IOCTL_MEM_ACCESS, &parms);

    DBG_EYE("Write 0x%08X to 0x%08X\n", data, address);
}
