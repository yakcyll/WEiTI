/****************************************************************************/
/*									ASFS									*/
/*				A file system with a completely unrelated name				*/
/*																			*/
/*						   Constant values' header							*/
/****************************************************************************/

#ifndef __CONST_H__
#define __CONST_H__

typedef unsigned short		uid_t, mode_t, umode_t, nlink_t, uint16_t;
typedef unsigned int		size_t, uint32_t;
typedef int					ssize_t;
typedef unsigned long long	u64;
typedef	long				off_t;
typedef long long			loff_t;
//typedef int					(*filldir_t)(void *, const char *, int, loff_t, u64, unsigned);



#define BLOCK_SIZE			4096

#define	BLOCKS_PER_INODE	19
#define	BLOCKS_PER_BLOCK	1020 // (4096 - 16) / 4

#endif

