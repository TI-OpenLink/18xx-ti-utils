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

#include "calibrator.h"
#include "plt.h"
#include "wl18xx_plt.h"

SECTION(wl18xx_plt);

static int plt_wl18xx_tune_channel(struct nl80211_state *state, struct nl_cb *cb,
				   struct nl_msg *msg, int argc, char **argv)
{
	struct nlattr *key;
	struct wl18xx_cmd_channel_tune prms;

	if (argc != 3)
		return 1;

	prms.test.id	= WL18XX_TEST_CMD_CHANNEL_TUNE;
	prms.channel	= (__u8)atoi(argv[0]);
	prms.band	= (__u8)atoi(argv[1]);
	prms.bandwidth	= (__u8)atoi(argv[2]);

	key = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!key) {
		fprintf(stderr, "fail to nla_nest_start()\n");
		return 1;
	}

	NLA_PUT_U32(msg, WL1271_TM_ATTR_CMD_ID, WL1271_TM_CMD_TEST);
	NLA_PUT(msg, WL1271_TM_ATTR_DATA, sizeof(prms), &prms);

	nla_nest_end(msg, key);

	return 0;

nla_put_failure:
	fprintf(stderr, "%s> building message failed\n", __func__);
	return 2;
}

COMMAND(wl18xx_plt, tune_channel, "<channel> <band> <bandwidth>",
	NL80211_CMD_TESTMODE, 0, CIB_NETDEV, plt_wl18xx_tune_channel,
	"Set channel, band and bandwidth for PLT.\n");

static int plt_wl18xx_start_rx(struct nl80211_state *state, struct nl_cb *cb,
			       struct nl_msg *msg, int argc, char **argv)
{
	struct nlattr *key;
	struct wl18xx_cmd_start_rx prms;

	if (argc != 0)
		return 1;

	prms.test.id	= WL18XX_TEST_CMD_START_RX_SIMULATION;

	key = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!key) {
		fprintf(stderr, "fail to nla_nest_start()\n");
		return 1;
	}

	NLA_PUT_U32(msg, WL1271_TM_ATTR_CMD_ID, WL1271_TM_CMD_TEST);
	NLA_PUT(msg, WL1271_TM_ATTR_DATA, sizeof(prms), &prms);

	nla_nest_end(msg, key);

	return 0;

nla_put_failure:
	fprintf(stderr, "%s> building message failed\n", __func__);
	return 2;
}

COMMAND(wl18xx_plt, start_rx, "",
	NL80211_CMD_TESTMODE, 0, CIB_NETDEV, plt_wl18xx_start_rx,
	"Start gathering RX statistics for PLT.\n");

static int plt_wl18xx_stop_rx(struct nl80211_state *state, struct nl_cb *cb,
			      struct nl_msg *msg, int argc, char **argv)
{
	struct nlattr *key;
	struct wl18xx_cmd_stop_rx prms;

	if (argc != 0)
		return 1;

	prms.test.id	= WL18XX_TEST_CMD_STOP_RX_SIMULATION;

	key = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!key) {
		fprintf(stderr, "fail to nla_nest_start()\n");
		return 1;
	}

	NLA_PUT_U32(msg, WL1271_TM_ATTR_CMD_ID, WL1271_TM_CMD_TEST);
	NLA_PUT(msg, WL1271_TM_ATTR_DATA, sizeof(prms), &prms);

	nla_nest_end(msg, key);

	return 0;

nla_put_failure:
	fprintf(stderr, "%s> building message failed\n", __func__);
	return 2;
}

COMMAND(wl18xx_plt, stop_rx, "",
	NL80211_CMD_TESTMODE, 0, CIB_NETDEV, plt_wl18xx_stop_rx,
	"Stop gathering RX statistics for PLT.\n");


static int plt_wl18xx_display_rx_stats(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *td[WL1271_TM_ATTR_MAX + 1];
	struct wl18xx_cmd_rx_stats *prms;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_TESTDATA]) {
		fprintf(stderr, "no data!\n");
		return NL_SKIP;
	}

	nla_parse(td, WL1271_TM_ATTR_MAX, nla_data(tb[NL80211_ATTR_TESTDATA]),
		  nla_len(tb[NL80211_ATTR_TESTDATA]), NULL);

	prms = (struct wl18xx_cmd_rx_stats *) nla_data(td[WL1271_TM_ATTR_DATA]);

	printf("\nRX statistics (status %d)\n", prms->radio_status);
	printf("Total packets:\t%d\n", prms->total);
	printf("FCS errors:\t%d\n", prms->errors);
	printf("MAC mismatch:\t%d\n", prms->addr_mm);
	printf("Good packets:\t%d\n", prms->good);

	return NL_SKIP;
}

static int plt_wl18xx_get_rx_stats(struct nl80211_state *state, struct nl_cb *cb,
				   struct nl_msg *msg, int argc, char **argv)
{
	struct nlattr *key;
	struct wl18xx_cmd_rx_stats prms;

	if (argc != 0)
		return 1;

	prms.test.id = WL18XX_TEST_CMD_GET_RX_STATISTICS;

	printf("test.id = 0x%0x\n", prms.test.id);

	key = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!key) {
		fprintf(stderr, "fail to nla_nest_start()\n");
		return 1;
	}

	NLA_PUT_U32(msg, WL1271_TM_ATTR_CMD_ID, WL1271_TM_CMD_TEST);
	NLA_PUT(msg, WL1271_TM_ATTR_DATA, sizeof(prms), &prms);
	NLA_PUT_U8(msg, WL1271_TM_ATTR_ANSWER, 1);

	nla_nest_end(msg, key);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
		  plt_wl18xx_display_rx_stats, NULL);

	return 0;

nla_put_failure:
	fprintf(stderr, "%s> building message failed\n", __func__);
	return 2;
}

COMMAND(wl18xx_plt, get_rx_stats, "",
	NL80211_CMD_TESTMODE, 0, CIB_NETDEV, plt_wl18xx_get_rx_stats,
	"Retrieve RX statistics for PLT.\n");
