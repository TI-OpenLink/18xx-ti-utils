/*
 * sdio_access.c
 *
 *  Created on: 8 ���� 2012
 *      Author: a0389772
 */

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


#include "public_types.h"
#include "log.h"

/*
 * Global Variables
 */
int irq_status_read_count=0;
char debugfs_filename[MAX_FILE_NAME];// /sys/kernel/debug/ieee80211/phy*/wlcore/mem

/*
 * Functions
 */
static void print_debugfs_cmd(char *type, int addr, int len, char * buf) {
	log_write(" Server: %s DEBUG FS\n", type);
	log_write("\tThe register off is 0x%X \n", addr);
	log_write("\tThe data length  is 0x%X \n", len);
	log_write("\tThe data value for register %d is 0x%x\n", 0, ((unsigned int *)buf)[0]);
}


int get_debugfs_sdio_file_status()
{
	struct stat st;
	if ( debugfs_filename==NULL)
		return ASI_FAIL;

	/* check if file exist */
	if(stat(debugfs_filename, &st) != 0) {
		error(" The SDIO access file is missing \n");
		return ASI_FAIL;
	}

	return ASI_SUCCESS;
}

int set_debugfs_sdio(char* file_name)
{
	struct stat st;
	if ( strlen(file_name)>=MAX_FILE_NAME || (file_name==NULL) )
		return ASI_FAIL;

	/* check if file exist */
	if(stat(file_name, &st) != 0) {
		log_write(" The file %s is NOT present\n", file_name);
		return ASI_FAIL;
	}

	/* update the file name parameter */
	memset(debugfs_filename, 0x0, MAX_FILE_NAME);
	memcpy(debugfs_filename, file_name, strlen(file_name));

	return ASI_SUCCESS;
}


int debugfs_write(int addr, int len, char * buf)
{
	int i=0;
	int fp=open(debugfs_filename, O_RDWR);
	print_debugfs_cmd("WRITING", addr, len, buf);
	lseek(fp, addr, SEEK_SET);
	write(fp, buf, len);
	close(fp);
	return ASI_SUCCESS;
}

int debugfs_read(int addr, int len, char * buf)
{
	int i=0;
	int fp=open(debugfs_filename, O_RDWR);
	lseek(fp, addr, SEEK_SET);
	read(fp, buf, len);
	if(!((addr==0x8050F8) && (*((unsigned int*)buf)==0x0)))
	{
		print_debugfs_cmd("READING", addr, len, buf);
	}
	else
	{
		/* RT^3 is polling status register each 100 ms. In majority of the cases the result is zero.
		 * This message segments that application is still up */
		irq_status_read_count++;
		if(irq_status_read_count%100==0x0)
			log_write(" Server: Number of background zero interrupt status result are: %d \n", irq_status_read_count);
	}
	close(fp);
	return ASI_SUCCESS;
}


