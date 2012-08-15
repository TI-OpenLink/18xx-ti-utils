/*
 * This file is part of calibrator
 *
 * Copyright (C) 2011 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __WL18XX_PLT_H__
#define __WL18XX_PLT_H__

enum wl18xx_test_cmds {
	/*  0x1    */	WL18XX_TEST_CMD_PD_BUFFER_CAL = 0x1,
	/*  0x2    */	WL18XX_TEST_CMD_P2G_CAL,
	/*  0x3    */	WL18XX_TEST_CMD_RX_PLT_ENTER,
	/*  0x4    */	WL18XX_TEST_CMD_RX_PLT_CAL,
	/*  0x5    */	WL18XX_TEST_CMD_RX_PLT_EXIT,
	/*  0x6    */	WL18XX_TEST_CMD_RX_PLT_GET,
	/*  0x7    */	WL18XX_TEST_CMD_FCC,
	/*  0x8    */	WL18XX_TEST_CMD_TELEC,
	/*  0x9    */	WL18XX_TEST_CMD_STOP_TX,
	/*  0xa    */	WL18XX_TEST_CMD_PLT_TEMPLATE,
	/*  0xb    */	WL18XX_TEST_CMD_PLT_GAIN_ADJUST,
	/*  0xc    */	WL18XX_TEST_CMD_PLT_GAIN_GET,
	/*  0xd    */	WL18XX_TEST_CMD_CHANNEL_TUNE_OLD,
	/*  0xe    */	WL18XX_TEST_CMD_FREE_RUN_RSSI,
	/*  0xf    */	WL18XX_TEST_CMD_DEBUG,
	/*  0x10   */	WL18XX_TEST_CMD_CLPC_COMMANDS,
	/*  0x11   */	WL18XX_TEST_CMD_RESERVED,
	/*  0x11   */	WL18XX_TEST_CMD_RX_STAT_STOP,
	/*  0x12   */	WL18XX_TEST_CMD_RX_STAT_START,
	/*  0x13   */	WL18XX_TEST_CMD_RX_STAT_RESET,
	/*  0x14   */	WL18XX_TEST_CMD_RX_STAT_GET,
	/*  0x15   */	WL18XX_TEST_CMD_LOOPBACK_START,
	/*  0x16   */	WL18XX_TEST_CMD_LOOPBACK_STOP,
	/*  0x17   */	WL18XX_TEST_CMD_GET_FW_VERSIONS,
	/*  0x18   */	WL18XX_TEST_CMD_INI_FILE_RADIO_PARAM,
	/*  0x19   */	WL18XX_TEST_CMD_RUN_CALIBRATION_TYPE,
	/*  0x1a   */	WL18XX_TEST_CMD_TX_GAIN_ADJUST,
	/*  0x1b   */	WL18XX_TEST_CMD_UPDATE_PD_BUFFER_ERRORS,
	/*  0x1c   */	WL18XX_TEST_CMD_UPDATE_PD_REFERENCE_POINT,
	/*  0x1d   */	WL18XX_TEST_CMD_INI_FILE_GENERAL_PARAM,
	/*  0x1e   */	WL18XX_TEST_CMD_SET_EFUSE,
	/*  0x1f   */	WL18XX_TEST_CMD_GET_EFUSE,
	/*  0x20   */	WL18XX_TEST_CMD_TEST_TONE,
	/*  0x21   */	WL18XX_TEST_CMD_POWER_MODE,
	/*  0x22   */	WL18XX_TEST_CMD_SMART_REFLEX,
	/*  0x23   */	WL18XX_TEST_CMD_CHANNEL_RESPONSE,
	/*  0x24   */	WL18XX_TEST_CMD_DCO_ITRIM_FEATURE,
	/*  0x25   */	WL18XX_TEST_CMD_START_TX_SIMULATION,
	/*  0x26   */	WL18XX_TEST_CMD_STOP_TX_SIMULATION,
	/*  0x27   */	WL18XX_TEST_CMD_START_RX_SIMULATION,
	/*  0x28   */	WL18XX_TEST_CMD_STOP_RX_SIMULATION,
	/*  0x29   */	WL18XX_TEST_CMD_GET_RX_STATISTICS,
	/*  0x2a   */	WL18XX_TEST_CMD_SET_NVS_VERSION,
	/*  0x2b   */	WL18XX_TEST_CMD_CHANNEL_TUNE,
	/*  0x2c   */	WL18XX_TEST_CMD_TX_POWER,
	/*  0x2d    */	WL18XX_TEST_CMD_SET_ANTENNA_MODE_24G,
	/*  0x2e    */  WL18XX_TEST_CMD_GET_CALIB_RESULT,
	/*  0x2f    */  WL18XX_TEST_CMD_SET_ANTENNA_MODE_5G,
};

struct wl18xx_cmd_channel_tune {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;

	__le16	radio_status;
	__u8	channel;
	__u8	band;
	__u8	bandwidth;
	__u8	padding[3];
} __attribute__((packed));

struct wl18xx_cmd_start_rx {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;
} __attribute__((packed));

struct wl18xx_cmd_stop_rx {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;
} __attribute__((packed));

struct wl18xx_cmd_rx_stats {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;

	__le32 radio_status;

	__le32 total;
	__le32 errors;
	__le32 addr_mm;
	__le32 good;
} __attribute__((packed));

struct wl18xx_cmd_start_tx {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;

	__le32 radio_status;

	__le32 delay;
	__le32 rate;
	__le32 size;
	__le32 mode;
	__le32 data_type;
	__le32 gi;
	__le32 options1;
	__le32 options2;
	__u8   src_addr[MAC_ADDR_LEN];
	__u8   dst_addr[MAC_ADDR_LEN];
	__le32 bandwidth;
	__le32 padding;
} __attribute__((packed));

struct wl18xx_cmd_stop_tx {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;
} __attribute__((packed));

struct wl18xx_cmd_set_tx_power {
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;

	__le32 radio_status;

	__le32 mac_des_pwr;
	__le32 mac_lvl_idx;
	__le32 freq_band;
	__le32 freq_prim_chan_num;
	__le32 freq_2nd_chan_idx;
	__le32 mac_ant_select;
	__le32 mac_non_srv;
	__le32 mac_chan_lim_dis;
	__le32 mac_fem_lim_dis;
	__le32 mac_gain_calc_mode;
	__le32 mac_analog_gain_control_idx;
	__le32 mac_post_dpd_gain;
} __attribute__((packed));

struct wl18xx_cmd_set_antenna_mode_24G { /* TEST_CMD_SET_ANTENNA_MODE24G */
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;

	__le32 radio_status;

	__u8   mac_prim_rx_chain;
	__u8   mac_prim_tx_chain;
	__u8   mac_rx_chain1_en;
	__u8   mac_rx_chain2_en;
	__u8   mac_tx_chain1_en;
	__u8   mac_tx_chain2_en;
	__u8   res1;
	__u8   res2;
} __attribute__((packed));

struct wl18xx_cmd_set_antenna_mode_5G { /* TEST_CMD_SET_ANTENNA_MODE5G */
	struct wl1271_cmd_header header;
	struct wl1271_cmd_test_header test;

	__le32 radio_status;

	__u8   mac_prim_rx_chain;
	__u8   mac_rx_chain1_en;
	__u8   mac_rx_chain2_en;
	__u8   mac_tx_chain1_en;
} __attribute__((packed));

#endif /* __WL18XX_PLT_H__ */
