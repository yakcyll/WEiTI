/****************************************************************************/
/*									ASFS									*/
/*				A file system with a completely unrelated name				*/
/*																			*/
/*							 Defining structures							*/
/****************************************************************************/

#ifndef __FS_H__
#define __FS_H__

#include <time.h>
#include "const.h"
#include "list.h"

struct super_block {
	loff_t				s_ibasket;
	short int			s_ibasket_count;
	short int			s_ibasket_max;

	loff_t				s_bbasket;
	int					s_bbasket_count;	/* taken blocks	(i.e. used space) */
	int					s_bbasket_size;		/* total number of blocks available for the partition (i.e. total space) */

	unsigned long		s_blocksize;
	unsigned char		s_rd_only;
	unsigned long		s_flags;
	struct dentry		*s_root;

};

struct inode {
	unsigned int		i_id;

	unsigned int		i_count; /* reference count */
	umode_t				i_mode;
	nlink_t				i_nlink;
	off_t				i_size;
	time_t				i_ctime;
	time_t				i_mtime;
	time_t				i_atime;
	unsigned int		i_flags;
	unsigned long		i_blockcount;
	unsigned long		i_blocks[BLOCKS_PER_INODE];
	loff_t				i_otherblocks;	/* address to a block stored on the disk */
	unsigned long		i_version;
};

struct blocks {
	loff_t				b_inode;
	unsigned long		b_offset;
	unsigned long		b_blockcount;
	unsigned long		b_blocks[BLOCKS_PER_BLOCK];
	loff_t				b_otherblocks;
};

struct dentry {
	int					d_count;
	unsigned int		d_flags;
	struct inode		*d_inode;	/* Where the name belongs to - NULL is negative */
	struct dentry		*d_parent;	/* parent directory */
	/* TODO: directory children list for whatever reason */
};

struct dir_entry {
	uint32_t			inode;			/* Inode number */
	loff_t				de_inode;		/* for convenience */
	uint16_t			rec_len;		/* Directory entry length */
	uint16_t			name_len;		/* Name length */
	char				name[256];		/* File name */
};

struct file {
	/* TODO: file descriptor table pointer */
	struct inode		*f_node;
	struct dentry		*f_dentry;
	unsigned int		f_flags;
	mode_t				f_mode;
	loff_t				f_pos;

	unsigned long		f_version;

	/* Should also implement uid/guid fields etc. in case any process
	   other than the console would be able to write to the file system. */
};

struct nameidata {
	struct dentry		nid_dest;
	uint16_t			name_len;		/* Name length */
	char				name[256];		/* File name */
	unsigned int		flags;

};

#endif

