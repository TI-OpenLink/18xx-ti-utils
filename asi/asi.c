/*
 *
 * Description: ASI (Android Sdio Interconnect
 * Author: Arthur Tiv
 * Date: 08/2012
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "asi.h"
#include "log.h"
#include "device_mode.h"
#include "sdio_access.h"
#include "public_types.h"

struct wl18xx_host_cmd {
	__le32 cmd_type;
	__le32 reg_off;
	__le32 data_len;
} __attribute__((packed));

struct wl18xx_host_cmd_result {
	__le32 cmd_type;
	__le32 cmd_status;
	__le32 data_len;
} __attribute__((packed));

/*
 * Function prototypes
 */
int (*hst_cmd_func[2])(int addr, int len, char * buf) = {debugfs_read, debugfs_write};

/*
 * Global Variables
 */
char asi_version[] = "0.01";
static int socket_connected = 0;
char *cmd_type_log[__WL18XX_HOST_CMD_LAST+1]={"read", "write"};
static const char *argv0;
char *sock_data_buf;

/*
 * Functions
 */
static void usage_options(void)
{
	printf("Options:\n");
	printf("\t--log LOG DIRECTORY\tset out debug notification to the log file. \n");
	printf("\t--debug\t\tset out debug notification to print  screen\n");
	printf("\t--version\tshow version (%s)\n", asi_version);
}

static void usage()
{
	printf("Usage:\t%s [OPTION]... SDIO ACCESS FILE \n\n", argv0);
	usage_options();
}

static void version(void)
{
	printf("asi version %s\n", asi_version);
}

static void sigpipe_handler()
{
	error("sigpipe caught!\r\n");
	socket_connected = 0;
}

/* Function name: create_listen_socket */
static int create_listen_socket(int listen_port)
{
	struct sockaddr_in serv_addr, cli_addr;
	int sock_flags = 0;
	int listen_sock = INVALID_HANDLE;

	log_write(" Server: Create Socket...\n");
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_HANDLE)
	{
		error(" Could not opening socket...\r\n");
		return INVALID_HANDLE;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(listen_port);

	// Bind to local port
	log_write(" Server: Binding socket...\n");
	if (bind(listen_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		  error(" Could not start server...\r\n");
		  return INVALID_HANDLE;
	}

	log_write(" Server: Listen for Client...\n");
	if(listen(listen_sock,1) < 0){
		error(" Failed to listen socket...\n");
		close(listen_sock);
		return INVALID_HANDLE;
	}

	return listen_sock;
}

/*
 * Function Name: accept_client
 */
static int accept_client(int server_sock)
{
	int client_sock = INVALID_HANDLE;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	struct timeval tv;

	tv.tv_sec = KEEP_ALIVE_INTERVAL;
	tv.tv_usec = 0;

	clilen = sizeof(cli_addr);

	// Non-Blocking accept for remote client connections
	log_write(" Server: Accepting Connection...\n");
	client_sock = accept(server_sock, (struct sockaddr *) &cli_addr, &clilen);

	if(client_sock != INVALID_HANDLE) {
		log_write(" Server: Connection established\n");
		socket_connected = 1;
	}
	else {
		log_write(" Server Connection failed \n");
		socket_connected = 0;

	}
	return client_sock;
}

static int socket_handle(int listen_sock)
{
	int client_sock = INVALID_HANDLE;
	struct wl18xx_host_cmd hst_cmd;
	int n = 0;
	enum wl18xx_host_cmd_result_e cmd_rslt = WL18XX_HOST_CMD_SUCCESS;
	struct wl18xx_host_cmd_result hst_cmd_rslt;

	log_write("\n Server: ADB Client socket session started !!!\n");

	/* Check on the client connection status */
    if (listen_sock != INVALID_HANDLE)
    {
    	client_sock = accept_client(listen_sock);
    }
    else
    {
    	error(" Socket testing FAIL\n");
    	return ASI_FAIL;
    }

    if(listen_sock < 0)
    	return ASI_FAIL;


    while(socket_connected)	 {
    	n=recv(client_sock, (unsigned char *)&hst_cmd, sizeof(struct wl18xx_host_cmd), MSG_WAITALL);
    	if(n < 0) {
    		//TODO Arthur: Fix it
    		//error(" Server: Client disconnected with error is %s and id %d", strerror(errno), errno);
    		socket_connected = 0;
    	}
    	else if(n == 0) {
    		log_write(" Server: No Data Received, connection must be closed\n");
    		error(" Server: Client disconnected");
    		socket_connected = 0;
    	}
    	else if (n<sizeof(struct wl18xx_host_cmd)) {
    		cmd_rslt = WL18XX_HOST_CMD_FAILED_SHORT_COMMAND_LEN;
    		error(" Server: Short command length\n");
    	}
    	else {
    		if (hst_cmd.cmd_type > __WL18XX_HOST_CMD_LAST) {
    			log_write(" Server: Received host command type %d. This is invalid command type. \n", hst_cmd.cmd_type);
    			error(" Server: Wrong host command \n");
        		cmd_rslt = WL18XX_HOST_CMD_FAILED_INVALID_CMD_TYPE;
    		}
    		else {
    			/*  Allocating buffer for result command data */
    			// TODO Arthur: check boundary length min and max
    			if(hst_cmd.data_len<MIN_SDIO_BUF_LEN_BYTES || hst_cmd.data_len>MAX_SDIO_BUF_LEN_BYTES)
    			{
    				log_write(" Server: command data length parameter is %d. This less than %d or greater than %d bytes \n", \
    													hst_cmd.data_len, MIN_SDIO_BUF_LEN_BYTES, MAX_SDIO_BUF_LEN_BYTES);
    				error(" Server: invalid command data length parameter \n");
    	    		socket_connected = 0;
    	    		break;
    			}
    			else {
    				sock_data_buf=realloc(sock_data_buf, hst_cmd.data_len);
    				if(sock_data_buf==NULL) {
    					log_write(" Server: Buffer allocation failed\n");
    					socket_connected = 0;
    				}
    			}

    			if(hst_cmd.cmd_type == WL18XX_HOST_CMD_WRITE) {
					n=recv(client_sock, sock_data_buf, hst_cmd.data_len, MSG_WAITALL);
					if(n<hst_cmd.data_len) {
						log_write(" Server: received  %d bytes data, less expected %d \n", n,  hst_cmd.data_len);
						error(" Server: received  less data than expected \n");
						cmd_rslt = WL18XX_HOST_CMD_FAILED_SHORT_DATA_LEN;
						break;
					}
    			}
    			cmd_rslt = hst_cmd_func[hst_cmd.cmd_type](hst_cmd.reg_off, hst_cmd.data_len, sock_data_buf);
    		}
    	}

    	/* Send result to the host */
    	if(socket_connected)	 {
    		hst_cmd_rslt.cmd_type=hst_cmd.cmd_type;
    		hst_cmd_rslt.cmd_status=cmd_rslt;
    		hst_cmd_rslt.data_len=0;

    		if( (hst_cmd_rslt.cmd_type==WL18XX_HOST_CMD_READ) &&
    			(hst_cmd_rslt.cmd_status==WL18XX_HOST_CMD_SUCCESS) )
        		hst_cmd_rslt.data_len=hst_cmd.data_len;

    		/* Send command result back to the client */
    		send(client_sock, ((unsigned char*)&hst_cmd_rslt), sizeof(hst_cmd_rslt), 0);
    		/* Sending data result to the client */
    		send(client_sock, (void*)sock_data_buf, hst_cmd_rslt.data_len, 0);

    		/* If there was failure command parsing, disconnect */
    		if(hst_cmd_rslt.cmd_status!=WL18XX_HOST_CMD_SUCCESS)
    			socket_connected = 0;

    	}
    }

    log_write( " Server: socket close client session... \n");
	close(client_sock);
	log_write("\n Server: ADB Client socket session Finished!!!\n");
	return ASI_SUCCESS;
}

static void start_adb_socket()	{
	int listen_sock=INVALID_HANDLE;
	int sock_status=ASI_SUCCESS;

	system("adb devices");

	/* Insert port */
	listen_sock = create_listen_socket(TCP_SERVER_PORT);

	while( (listen_sock!=INVALID_HANDLE) && (sock_status == ASI_SUCCESS))
	{
		/* WLAN enable */
		chip_awake(2);

		/* Call  socket session  handling */
		sock_status=socket_handle(listen_sock);
		sleep (1);

		/* Disable WLAN */
		chip_awake(0);
		sleep (1);

	}

    log_write( " Server: socket close listen mode... n");
	close(listen_sock);
}


int main(int argc, char *argv[])	{
	int ret=0;
	argc--;
	argv0 = *argv++;
	int done=0;

	while(argc>0) {
		if (argc > 0 && strcmp(*argv, "--chip_awake") == 0) {
			/* WLAN enable */
			chip_awake(2);
			return ASI_SUCCESS;
		}
		else if (argc > 0 && strcmp(*argv, "--on") == 0) {
			/* WLAN enable */
			chip_awake(1);
			return ASI_SUCCESS;
		}
		else if (argc > 0 && strcmp(*argv, "--off") == 0) {
			/* WLAN enable */
			chip_awake(0);
			return ASI_SUCCESS;
		}
		else if (argc > 0 && strcmp(*argv, "--log") == 0) {
			argc--;
			argv++;
			ret=LOG_FILE_FAIL;
			if(argc>0) {
				ret=log_init(LOG_TO_FILE, *argv);
				argc--;
				argv++;
			}
			if (ret!=LOG_FILE_SUCCESS) {
				usage();
				return ASI_FAIL;
			}
		}
		else if (argc > 0 && strcmp(*argv, "--debug") == 0) {
			argc--;
			argv++;
			ret=log_init(LOG_PRNT_SCR, NULL);
			if (ret!=LOG_FILE_SUCCESS) {
				usage();
				return ASI_FAIL;
			}
		}
		else if (argc > 0 && ((strcmp(*argv, "--version") == 0) ||
				 (strcmp(*argv, "-v") == 0))) {
			version();
			return ASI_SUCCESS;
		}
		else if(argc==1){
			if (set_debugfs_sdio(*argv) !=ASI_SUCCESS) {
				usage();
				return ASI_FAIL;
			}
			argc--;
			argv++;
		}
		else {
			usage();
			return ASI_FAIL;
		}
	}


	if ( argc > 0 || get_debugfs_sdio_file_status() != ASI_SUCCESS) {
		usage();
		return ASI_FAIL;
	}

	start_adb_socket();
    return ASI_SUCCESS;
}
