#include "kiofs.h"

int iofs_iterate_shared(struct file *filp, struct dir_context *dc) {
/*
    struct inode *inode;

    struct super_block *sb;
    struct buffer_head *bh;
    struct iofs_inode *iofs_inode;
    struct iofs_dir_record *dir_record;
    uint64_t i;

    inode = filp->f_path.dentry->d_inode;

    sb = inode->i_sb;
    iofs_inode = IOFS_INODE(inode);

    printk(KERN_INFO "readdir: iofs_inode->inode_no=%llu iofs_inode->dir_children_count=%llu", iofs_inode->inode_no, iofs_inode->dir_children_count);
    if (dc->pos >= iofs_inode->dir_children_count)
                return 0;

    if (dc->pos == 0) {
	if (!dir_emit_dot(filp, dc))
		goto out;
	dc->pos = 1;
    }

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
        dir_emit(dc, dir_record->filename, strnlen(dir_record->filename,IOFS_FILENAME_MAXLEN), dir_record->inode_no, DT_UNKNOWN);
        dir_record++;
        dc->pos++;
    }

    brelse(bh);
out:
*/
    return 0;
}
