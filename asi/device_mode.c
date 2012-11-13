/*
 * device_mode.c
 *
 *  Created on: 31 October  2012
 *      Author: Arthur Tiv
 */


/*
 * PLT utility for wireless chip supported by TI's driver wl12xx
 *
 * See README and COPYING for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "nl80211.h"
#include "device_mode.h"

#ifndef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */

static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}

static inline int __genl_ctrl_alloc_cache(struct nl_sock *h,
	struct nl_cache **cache)
{
	struct nl_cache *tmp = genl_ctrl_alloc_cache(h);
	if (!tmp)
		return -ENOMEM;
	*cache = tmp;
	return 0;
}
#define genl_ctrl_alloc_cache __genl_ctrl_alloc_cache
#endif /* CONFIG_LIBNL20 */


SECTION(plt);
SECTION(get);
SECTION(set);


static int plt_power_mode(struct nl80211_state *state, struct nl_cb *cb,
			  struct nl_msg *msg, int argc, char **argv)
{
	struct nlattr *key;
	unsigned int pmode;

	if (argc != 1) {
		fprintf(stderr, "%s> Missing arguments\n", __func__);
		return 2;
	}

	if (strcmp(argv[0], "on") == 0)
		pmode = PLT_ON;
	else if (strcmp(argv[0], "off") == 0)
		pmode = PLT_OFF;
	else if (strcmp(argv[0], "chip_awake") == 0)
		pmode = PLT_CHIP_AWAKE;
	else {
		fprintf(stderr, "%s> Invalid parameter\n", __func__);
		return 2;
	}

	key = nla_nest_start(msg, NL80211_ATTR_TESTDATA);
	if (!key) {
		fprintf(stderr, "%s> fail to nla_nest_start()\n", __func__);
		return 1;
	}

	NLA_PUT_U32(msg, WL1271_TM_ATTR_CMD_ID, WL1271_TM_CMD_SET_PLT_MODE);
	NLA_PUT_U32(msg, WL1271_TM_ATTR_PLT_MODE, pmode);

	nla_nest_end(msg, key);

	return 0;

nla_put_failure:
	fprintf(stderr, "%s> building message failed\n", __func__);
	return 2;
}

COMMAND(plt, power_mode, "<on|off>",
	NL80211_CMD_TESTMODE, 0, CIB_NETDEV, plt_power_mode,
	"Set PLT power mode\n");

static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
		fprintf(stderr, "Failed to allocate generic netlink cache.\n");
		err = -ENOMEM;
		goto out_handle_destroy;
	}

	state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
	if (!state->nl80211) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_cache_free;
	}

	return 0;

 out_cache_free:
	nl_cache_free(state->nl_cache);
 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	genl_family_put(state->nl80211);
	nl_cache_free(state->nl_cache);
	nl_socket_free(state->nl_sock);
}

static int cmd_size;

extern struct cmd __start___cmd;
extern struct cmd __stop___cmd;

#define for_each_cmd(_cmd)					\
	for (_cmd = &__start___cmd; _cmd < &__stop___cmd;		\
	     _cmd = (const struct cmd *)((char *)_cmd + cmd_size))


static void __usage_cmd(const struct cmd *cmd, char *indent, bool full)
{
	const char *start, *lend, *end;

	printf("%s", indent);

	switch (cmd->idby) {
	case CIB_NONE:
		break;
	case CIB_PHY:
		printf("phy <phyname> ");
		break;
	case CIB_NETDEV:
		printf("dev <devname> ");
		break;
	}
	if (cmd->parent && cmd->parent->name)
		printf("%s ", cmd->parent->name);
	printf("%s", cmd->name);
	if (cmd->args)
		printf(" %s", cmd->args);
	printf("\n");

	if (!full || !cmd->help)
		return;

	/* hack */
	if (strlen(indent))
		indent = "\t\t";
	else
		printf("\n");

	/* print line by line */
	start = cmd->help;
	end = strchr(start, '\0');
	do {
		lend = strchr(start, '\n');
		if (!lend)
			lend = end;
		printf("%s", indent);
		printf("%.*s\n", (int)(lend - start), start);
		start = lend + 1;
	} while (end != lend);

	printf("\n");
}

static void usage_options(void)
{
	printf("Options:\n");
	printf("\t--debug\t\tenable netlink debugging\n");
}

static const char *argv0;

static void usage(bool full)
{
	const struct cmd *section, *cmd;

	printf("Usage:\t%s [options] command\n", argv0);
	usage_options();
	printf("Commands:\n");
	for_each_cmd(section) {
		if (section->parent)
			continue;

		if (section->handler && !section->hidden)
			__usage_cmd(section, "\t", full);

		for_each_cmd(cmd) {
			if (section != cmd->parent)
				continue;
			if (!cmd->handler || cmd->hidden)
				continue;
			__usage_cmd(cmd, "\t", full);
		}
	}
}

static void usage_cmd(const struct cmd *cmd)
{
	printf("\nUsage:\t%s [options] ", argv0);
	__usage_cmd(cmd, "", true);
	usage_options();
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;

	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;

	return NL_STOP;
}

static int __handle_cmd(struct nl80211_state *state, enum id_input idby,
			int argc, char **argv, const struct cmd **cmdout)
{
	const struct cmd *cmd, *match = NULL, *sectcmd;
	struct nl_cb *cb;
	struct nl_msg *msg;
	int devidx = 0;
	int err, o_argc;
	const char *command, *section;
	char *tmp, **o_argv;
	enum command_identify_by command_idby = CIB_NETDEV;

	if (argc <= 1)
		return 1;

	o_argc = argc;
	o_argv = argv;

	devidx = if_nametoindex(*argv);
	if (devidx == 0)
		devidx = -1;
	argc--;
	argv++;


	if (devidx < 0)
		return -errno;

	section = *argv;
	argc--;
	argv++;

	for_each_cmd(sectcmd) {
		if (sectcmd->parent)
			continue;
		/* ok ... bit of a hack for the dupe 'info' section */
		if (match && sectcmd->idby != command_idby)
			continue;

		if (strcmp(sectcmd->name, section) == 0)
			match = sectcmd;
	}

	sectcmd = match;
	match = NULL;
	if (!sectcmd)
		return 1;

	if (argc > 0) {
		command = *argv;

		for_each_cmd(cmd) {
			if (!cmd->handler)
				continue;
			if (cmd->parent != sectcmd)
				continue;
			if (cmd->idby != command_idby)
				continue;
			if (strcmp(cmd->name, command))
				continue;
			if (argc > 1 && !cmd->args)
				continue;
			match = cmd;
			break;
		}

		if (match) {
			argc--;
			argv++;
		}
	}


	if (match)
		cmd = match;
	else {
		/* Use the section itself, if possible. */
		cmd = sectcmd;
		if (argc && !cmd->args)
			return 1;
		if (cmd->idby != command_idby)
			return 1;
		if (!cmd->handler)
			return 1;
	}

	if (cmdout)
		*cmdout = cmd;

	if (!cmd->cmd) {
		argc = o_argc;
		argv = o_argv;
		return cmd->handler(state, NULL, NULL, argc, argv);
	}

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out_free_msg;
	}

	genlmsg_put(msg, 0, 0, genl_family_get_id(state->nl80211), 0,
		    cmd->nl_msg_flags, cmd->cmd, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx);

	err = cmd->handler(state, cb, msg, argc, argv);
	if (err) {
		fprintf(stderr, "failed to handle\n");
		goto out;
	}

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to autocomplete\n");
		goto out;
	}

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);

 out:
	nl_cb_put(cb);
 out_free_msg:
	nlmsg_free(msg);
	return err;

 nla_put_failure:
	fprintf(stderr, "building message failed\n");
	return 2;
}

int handle_cmd(struct nl80211_state *state, enum id_input idby,
	       int argc, char **argv)
{
	return __handle_cmd(state, idby, argc, argv, NULL);
}

int chip_awake(int mode)
{
	struct nl80211_state nlstate;
	int err;
	const struct cmd *cmd = NULL;
	int i=0;

	int params_num=4;
	char *chip_on[4]={ "wlan0",  "plt",  "power_mode", "on"};
	char *chip_off[4]={"wlan0",  "plt",  "power_mode", "off"};
	char *chip_chp_awake[4]={"wlan0",  "plt",  "power_mode","chip_awake" };
	char **chip_mode;

	if(mode==0)
		chip_mode=chip_off;
	else if(mode==1)
		chip_mode=chip_on;
	else if(mode==2)
		chip_mode=chip_chp_awake;
	else return 1;

	/* calculate command size including padding */
	cmd_size = abs((long)&__section_set - (long)&__section_get);

	/* strip off self */
	argv0 = *chip_mode;

	err = nl80211_init(&nlstate);
	if (err)
		return 1;

	enum id_input idby = II_NETDEV;

	err = __handle_cmd(&nlstate, idby, params_num, chip_mode, &cmd);

	if (err == 1) {
		if (cmd)
			usage_cmd(cmd);
		else
			usage(false);
	} else if (err < 0)
		fprintf(stderr, "command failed: %s (%d)\n",
			strerror(-err), err);

	nl80211_cleanup(&nlstate);

	return err;
}
