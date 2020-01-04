#include "kiofs.h"

int iofs_iterate(struct file *filp, struct dir_context *dc) {

	return 0;
}
int iofs_iterate_shared(struct file *filp, struct dir_context *dc) {
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
