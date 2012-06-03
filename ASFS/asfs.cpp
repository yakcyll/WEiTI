/****************************************************************************/
/*									ASFS									*/
/*				A file system with a completely unrelated name				*/
/*																			*/
/*						 VFS methods' implementation						*/
/****************************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include "const.h"
#include "asfs.h"

using namespace std;



int asfs::asfs_read(char *buf) {

	if (buf == NULL)
		return -1;

	part.read(buf, BLOCK_SIZE);

	if(part.bad())
		return -1;

	return 0;

}

int asfs::asfs_write(char *buf) {

	if (buf == NULL)
		return -1;

	part.write(buf, BLOCK_SIZE);

	if(part.bad())
		return -1;

	return 0;

}

int asfs::asfs_lseek(loff_t pos) {

	if (sb_holder->s_bbasket_size < pos || pos == 0)
		return -1;

	part.seekg(pos*BLOCK_SIZE);
	part.seekp(pos*BLOCK_SIZE);

	return 0;

}

loff_t asfs::asfs_tellp() {

	return part.tellp()/BLOCK_SIZE;

}

loff_t asfs::asfs_tellg() {

	return part.tellg()/BLOCK_SIZE;

}


int asfs::asfs_open(struct dentry *dentry, struct nameidata *nid, struct file *file) {

	if (file != NULL || dentry == NULL || nid == NULL)
		return -1;

	struct inode			*d_inode = dentry->d_inode;
	char					*pageBuffer = new char[BLOCK_SIZE];
	char					*blocksBuffer = new char[BLOCK_SIZE];
	char					*inodeBuffer = new char[sizeof(struct inode)];
	int i = 0;

	char					*fileBuffer = new char[BLOCK_SIZE*d_inode->i_blockcount];
	loff_t					fileBufPos = 0;
	int						limit = BLOCKS_PER_INODE;
	loff_t					newPos = d_inode->i_otherblocks;

	for (int x = 0; x < d_inode->i_blockcount; x++) {
		if(x >= limit) {
			limit += BLOCKS_PER_BLOCK;
			asfs_lseek(newPos);
			asfs_read(blocksBuffer);
			newPos = ((struct blocks*)pageBuffer)->b_otherblocks;

		}

		if(x<BLOCKS_PER_INODE)
			asfs_lseek(d_inode->i_blocks[x]);
		else
			asfs_lseek(((struct blocks*)blocksBuffer)->b_blocks[(x-BLOCKS_PER_INODE)%BLOCKS_PER_BLOCK]);
		
		asfs_read(pageBuffer);
		memcpy((&fileBuffer[fileBufPos]), pageBuffer, BLOCK_SIZE);

	}

	for (i = 0; i < BLOCK_SIZE*d_inode->i_blockcount; i+=sizeof(struct dir_entry)) {
		if (((struct dir_entry*)(&fileBuffer[i])) != NULL && strcmp(((struct dir_entry*)(&fileBuffer[i]))->name, nid->name) == 0)
			break;
	}
	if (i >= BLOCK_SIZE*d_inode->i_blockcount)
		return -2;							/*	The Earth is about to implode, better hide	*/

	asfs_lseek(((struct dir_entry*)(&fileBuffer[i]))->de_inode);
	asfs_read(pageBuffer);
	for (int j = 0; j < BLOCK_SIZE/sizeof(struct inode); j++) {

		if (((struct inode *)&pageBuffer[j*sizeof(struct inode)])->i_id == ((struct dir_entry*)(&fileBuffer[i]))->inode ) {
			memcpy(inodeBuffer, ((void *)&pageBuffer[j*sizeof(struct inode)]), sizeof(struct inode));
			break;
		}
	
	}

	file = new struct file();

	file->f_node = (struct inode *)inodeBuffer;
	file->f_dentry = dentry;
	file->f_flags = file->f_node->i_flags;
	file->f_mode = file->f_node->i_mode;
	file->f_pos = 0;

	f_table->push_back(file);

	delete fileBuffer;
	delete blocksBuffer;
	delete pageBuffer;

	return 0;

}

int asfs::asfs_readfd(struct file *file, char *buf, size_t count) {

	if (file == NULL || buf == NULL || count <= 0)
		return -1;

	char					*pageBuffer = new char[BLOCK_SIZE];
	char					*blocksBuffer = new char[BLOCK_SIZE];
	int						pageCount, tempCount = count, remainder;
	int						blockFileNo;

	while (tempCount > 0) {
		loff_t blockNo = file->f_pos/BLOCK_SIZE;
		remainder = min(int(BLOCK_SIZE - file->f_pos%BLOCK_SIZE), tempCount);

		if(blockNo < BLOCKS_PER_INODE) {
			asfs_lseek(file->f_node->i_blocks[blockNo]);
		} else {
			blockFileNo = (blockNo-BLOCKS_PER_INODE)/BLOCKS_PER_BLOCK;
			asfs_lseek(file->f_node->i_otherblocks);
			asfs_read(pageBuffer);
			while(blockFileNo--) {
				asfs_lseek(((struct blocks*)pageBuffer)->b_otherblocks);
				asfs_read(pageBuffer);
			}
			asfs_lseek(((struct blocks*)pageBuffer)->b_blocks[(blockNo-BLOCKS_PER_INODE)%BLOCKS_PER_BLOCK]);
		}
		asfs_read(pageBuffer);

		memcpy((&buf[count-tempCount]), (void *)(&pageBuffer[file->f_pos%BLOCK_SIZE]), remainder);
		tempCount -= remainder;
		asfs_lseekfd(file, file->f_pos+remainder);

	}

	return (count - tempCount);

}

int asfs::asfs_writefd(struct file *file, char *buf, size_t count) {

	if (file == NULL || buf == NULL || count <= 0)
		return -1;

	int						allocPagesNo = 0;
	int						x, y;
	int						pageCount, tempCount = count, remainder;
	int						blockFileNo;
	struct blocks			*blockBuffer;
	vector<loff_t>			allocPagesNos;
	char					*pageBuffer = new char[BLOCK_SIZE];
	char					*bitmapBuf = new char[int(((sb_holder->s_bbasket_size/8)+BLOCK_SIZE-1)/BLOCK_SIZE)*BLOCK_SIZE];

	if ((file->f_pos+count)/BLOCK_SIZE > file->f_node->i_blockcount) {
		/*	Let there be dragons.	*/

		allocPagesNo = (count - (BLOCK_SIZE - (file->f_pos % BLOCK_SIZE))) / BLOCK_SIZE;

		if (file->f_node->i_blockcount+allocPagesNo > BLOCKS_PER_INODE) {
			allocPagesNo += 1;
			if ((x = (file->f_node->i_blockcount+allocPagesNo-BLOCKS_PER_INODE)/BLOCK_SIZE - (file->f_node->i_blockcount-BLOCKS_PER_INODE)/BLOCK_SIZE) > 0)
				allocPagesNo += x;
		}

		asfs_lseek(1 + (sb_holder->s_ibasket_max)/(BLOCK_SIZE/sizeof(struct inode)));
		for (int i = 0; i < ((sb_holder->s_bbasket_size/8)+BLOCK_SIZE-1)/BLOCK_SIZE; i++) asfs_read(&bitmapBuf[i*BLOCK_SIZE]);

		while (allocPagesNo > 0) {
			for (int i = 0; i < sb_holder->s_bbasket_size; i++) {
				for (int j = 0; j < 8; j++) {
					if((((int)bitmapBuf[i]) >> j) & 1 == 0) { bitmapBuf[i] += (1 << j); allocPagesNos.push_back(i*8+(8-j)); }
				}
			}
		}

		asfs_lseek(1 + (sb_holder->s_ibasket_max)/(BLOCK_SIZE/sizeof(struct inode)));
		for (int i = 0; i < ((sb_holder->s_bbasket_size/8)+BLOCK_SIZE-1)/BLOCK_SIZE; i++) asfs_write(&bitmapBuf[i*BLOCK_SIZE]);

		/*	Allocation's done, now let's keep the syntax intact	*/

		while (allocPagesNo > 0) {
			if (file->f_node->i_blockcount < BLOCKS_PER_INODE) {
				file->f_node->i_blocks[file->f_node->i_blockcount] = allocPagesNos.at(0); 
				allocPagesNos.erase(allocPagesNos.begin());
			}
			else {
				if (asfs_lseek(file->f_node->i_otherblocks) == -1) {
					blockBuffer = new struct blocks();
					blockBuffer->b_blockcount = 1;
					blockBuffer->b_inode = file->f_node->i_id;
					blockBuffer->b_offset = 0;
					blockBuffer->b_otherblocks = NULL;
					blockBuffer->b_blocks[0] = allocPagesNos.at(0);
					allocPagesNos.erase(allocPagesNos.begin());
					file->f_node->i_otherblocks = allocPagesNos.at(0);
					asfs_lseek(allocPagesNos.at(0));
					asfs_write((char*)blockBuffer);
					allocPagesNos.erase(allocPagesNos.begin());
					allocPagesNo--;
				}
				else {
					asfs_read(pageBuffer);
					while (((struct blocks *)pageBuffer)->b_otherblocks != NULL) {
						asfs_lseek(((struct blocks *)pageBuffer)->b_otherblocks);
						asfs_read(pageBuffer);
					}
					if ((((struct blocks *)pageBuffer)->b_blockcount)%BLOCKS_PER_BLOCK == 0) {
						blockBuffer = new struct blocks();
						blockBuffer->b_blockcount = 1;
						blockBuffer->b_inode = file->f_node->i_id;
						blockBuffer->b_offset = ((struct blocks *)pageBuffer)->b_offset + 1;
						blockBuffer->b_otherblocks = NULL;
						blockBuffer->b_blocks[0] = allocPagesNos.at(0);
						allocPagesNos.erase(allocPagesNos.begin());
						((struct blocks *)pageBuffer)->b_otherblocks = allocPagesNos.at(0);
						asfs_lseek(asfs_tellg() - 1);
						asfs_write(pageBuffer);
						asfs_lseek(allocPagesNos.at(0));
						asfs_write((char*)blockBuffer);
						allocPagesNos.erase(allocPagesNos.begin());
						allocPagesNo--;
					}
					else {
						((struct blocks *)pageBuffer)->b_blocks[((struct blocks *)pageBuffer)->b_blockcount] = allocPagesNos.at(0);
						((struct blocks *)pageBuffer)->b_blockcount++;
						asfs_lseek(asfs_tellg() - 1);
						asfs_write(pageBuffer);
					}
				}
			}
			file->f_node->i_blockcount++;
		}

	}

	/*	Please, never again	*/

	while (tempCount > 0) {
		loff_t blockNo = file->f_pos/BLOCK_SIZE;
		remainder = min(int(BLOCK_SIZE - file->f_pos%BLOCK_SIZE), tempCount);

		if(blockNo < BLOCKS_PER_INODE) {
			asfs_lseek(file->f_node->i_blocks[blockNo]);
		} else {
			blockFileNo = (blockNo-BLOCKS_PER_INODE)/BLOCKS_PER_BLOCK;
			asfs_lseek(file->f_node->i_otherblocks);
			asfs_read(pageBuffer);
			while(blockFileNo--) {
				asfs_lseek(((struct blocks*)pageBuffer)->b_otherblocks);
				asfs_read(pageBuffer);
			}
			asfs_lseek(((struct blocks*)pageBuffer)->b_blocks[(blockNo-BLOCKS_PER_INODE)%BLOCKS_PER_BLOCK]);
		}
		asfs_read(pageBuffer);

		memcpy((void *)(&pageBuffer[file->f_pos%BLOCK_SIZE]), (&buf[count-tempCount]), remainder);
		tempCount -= remainder;
		asfs_lseekfd(file, file->f_pos+remainder);

		asfs_lseek(asfs_tellg() - 1);
		asfs_write(pageBuffer);

	}

	return (count - tempCount);

}

int asfs::asfs_lseekfd(struct file *file, loff_t pos) {

	if (file == NULL || pos < 0 || pos > file->f_node->i_size)
		return -1;

	file->f_pos = pos;

	return 0;

}

int asfs::asfs_close(struct file *file) {

	if (file == NULL)
		return -1;

	delete file->f_node;
	f_table->remove(file);

	return 0;

}






int asfs::asfs_create(struct inode *, struct dentry *, int, struct nameidata *) {

}

int asfs::asfs_mkdir(struct inode *, struct dentry *, int) {

}

int asfs::asfs_symlink(struct inode *, struct dentry *, const char *) {

}

int asfs::asfs_rmdir(struct inode *, struct dentry *) {

}

int asfs::asfs_unlink(struct inode *, struct dentry *) {

}

int asfs::asfs_rename(struct inode *, struct dentry *, struct inode *, struct dentry *) {

}
