#include "kiofs.h"
#include <linux/vfs.h>

int iofs_iterate(struct file *filp, struct dir_context *dc) {

	return 0;
}
int iofs_iterate_shared(struct file *filp, struct dir_context *dc) {
	return 0;
}

int iofs_statfs(struct dentry *dentry, struct kstatfs *buf) {
	struct super_block *sb = dentry->d_sb;
	u64 id = 0;

	if (sb->s_bdev)
		id = huge_encode_dev(sb->s_bdev->bd_dev);
	else if (sb->s_dev)
		id = huge_encode_dev(sb->s_dev);


	buf->f_type = IOFS_MAGIC;
	buf->f_bsize = IOFS_DEFAULT_BLOCKSIZE;
	buf->f_blocks = 0; //CRAMFS_SB(sb)->blocks;
	buf->f_bfree = 0;
	buf->f_bavail = 0;
	buf->f_files = 0;
	buf->f_ffree = 0;
	buf->f_fsid.val[0] = (u32)id;
	buf->f_fsid.val[1] = (u32)(id >> 32);
	buf->f_namelen = IOFS_FILENAME_MAXLEN;

	return 0;
}

int iofs_readdir(struct file *filp, void *dirent, filldir_t filldir) {
    loff_t pos;
    struct inode *inode;
    struct super_block *sb;
    struct buffer_head *bh;
    struct iofs_inode *iofs_inode;
    struct iofs_dir_record *dir_record;
    uint64_t i;

    pos = filp->f_pos;
    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    iofs_inode = IOFS_INODE(inode);

    if (pos) {
        // TODO @Sankar: we use a hack of reading pos to figure if we have filled in data.
        return 0;
    }

    printk(KERN_INFO "readdir: iofs_inode->inode_no=%llu", iofs_inode->inode_no);

    if (unlikely(!S_ISDIR(iofs_inode->mode))) {
        printk(KERN_ERR
               "Inode %llu of dentry %s is not a directory\n",
               iofs_inode->inode_no,
               filp->f_path.dentry->d_name.name);
        return -ENOTDIR;
    }

    bh = sb_bread(sb, iofs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct iofs_dir_record *)bh->b_data;
    for (i = 0; i < iofs_inode->dir_children_count; i++) {
        filldir(dirent, dir_record->filename, IOFS_FILENAME_MAXLEN, pos,
                dir_record->inode_no, DT_UNKNOWN);
        filp->f_pos += sizeof(struct iofs_dir_record);
        pos += sizeof(struct iofs_dir_record);
        dir_record++;
    }
    brelse(bh);

    return 0;
}
