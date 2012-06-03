/****************************************************************************/
/*									ASFS									*/
/*				A file system with a completely unrelated name				*/
/*																			*/
/*					 Interface and handler implementation					*/
/****************************************************************************/

#ifndef __ASFS_H__
#define __ASFS_H__

#include <list>
#include <string>
#include "const.h"
#include "fs.h"

using namespace std;

class asfs {

	std::fstream				part;

	struct super_block			*sb_holder;
	struct dentry				*d_current;
	std::list<struct file *>	*f_table;
	std::list<struct dentry *>	*d_table;		/* of dubious importance, but hey, why not */

	public:

		asfs(string fname) { part.open(fname.c_str(), fstream::in | fstream::out | fstream::binary); }
		~asfs() { part.close(); }

		int asfs_read(char *);
		int asfs_write(char *);
		int asfs_lseek(loff_t);
		loff_t asfs_tellp();
		loff_t asfs_tellg();

		int asfs_open(struct dentry *, struct nameidata *, struct file *);
		int	asfs_readfd(struct file *, char *, size_t);
		int asfs_writefd(struct file *, char *, size_t);
		int asfs_lseekfd(struct file *, loff_t);
		int asfs_close(struct file *);

        int asfs_create(struct inode *, struct dentry *, int, struct nameidata *);
		int asfs_mkdir(struct inode *, struct dentry *, int);
		int asfs_symlink(struct inode *, struct dentry *, const char *);
		int asfs_rmdir(struct inode *, struct dentry *);
		int asfs_unlink(struct inode *, struct dentry *);
		int asfs_rename(struct inode *, struct dentry *, struct inode *, struct dentry *);

};

#endif
