#ifndef __ASI_H
#define __ASI_H


#define TCP_SERVER_PORT	6100
#define INVALID_HANDLE -1
#define KEEP_ALIVE_INTERVAL 5 // in seconds
/*
 * Structures and enums
 */
enum wl18xx_host_cmd_type_e {
	WL18XX_HOST_CMD_READ 	= 0,
	WL18XX_HOST_CMD_WRITE	= 1,
	__WL18XX_HOST_CMD_LAST = WL18XX_HOST_CMD_WRITE,
	WL18XX_HOST_CMD_ERROR	= 0xffffffff,
};

enum wl18xx_host_cmd_result_e {
	WL18XX_HOST_CMD_SUCCESS 					= 0,
	WL18XX_HOST_CMD_FAILED_SHORT_COMMAND_LEN	= 1,
	WL18XX_HOST_CMD_FAILED_INVALID_CMD_TYPE		= 2,
	WL18XX_HOST_CMD_FAILED_SHORT_DATA_LEN		= 3,
	__WL18XX_HOST_CMD_RSLT_LAST 	= 0xffffffff,
};
#endif /* __ASI_H */
