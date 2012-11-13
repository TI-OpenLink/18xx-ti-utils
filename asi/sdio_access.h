/*
 * sdio_access.h
 *
 *  Created on: 8 ���� 2012
 *      Author: a0389772
 */

#ifndef SDIO_ACCESS_H_
#define SDIO_ACCESS_H_
/*
 * Function prototypes
 */
int get_debugfs_sdio_file_status();
int set_debugfs_sdio(char* file_name);
int debugfs_read(int addr, int len, char * buf);
int debugfs_write(int addr, int len, char * buf);


#endif /* SDIO_ACCESS_H_ */
