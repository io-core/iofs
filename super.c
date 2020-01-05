#include "kiofs.h"

static int iofs_fill_super(struct super_block *sb, void *data, int silent) {
//    struct inode *root_inode;
//    struct iofs_inode *root_iofs_inode;
    struct buffer_head *bh;
    struct iofs_superblock *iofs_sb;
    int ret = -ENOSYS;

    bh = sb_bread(sb, IOFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);

    iofs_sb = (struct iofs_superblock *)bh->b_data;
    if (unlikely(iofs_sb->magic != IOFS_MAGIC)) {
        printk(KERN_ERR
               "The filesystem being mounted is not of type iofs. "
               "Magic number mismatch: %llu != %llu\n",
               iofs_sb->magic, (uint64_t)IOFS_MAGIC);
        goto release;
    }

/*
    if (unlikely(sb->s_blocksize != iofs_sb->blocksize)) {
        printk(KERN_ERR
               "iofs seem to be formatted with mismatching blocksize: %lu\n",
               sb->s_blocksize);
        goto release;
    }

    sb->s_magic = iofs_sb->magic;
    sb->s_fs_info = iofs_sb;
    sb->s_maxbytes = iofs_sb->blocksize;
    sb->s_op = &iofs_sb_ops;

    root_iofs_inode = iofs_get_iofs_inode(sb, IOFS_ROOTDIR_INODE_NO);
    root_inode = new_inode(sb);
    if (!root_inode || !root_iofs_inode) {
        ret = -ENOMEM;
        goto release;
    }
    iofs_fill_inode(sb, root_inode, root_iofs_inode);
    inode_init_owner(root_inode, NULL, root_inode->i_mode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto release;
    }

*/

release:
    brelse(bh);
    return ret;
}

struct dentry *iofs_mount(struct file_system_type *fs_type,
                             int flags, const char *dev_name,
                             void *data) {
    struct dentry *ret;
    ret = mount_bdev(fs_type, flags, dev_name, data, iofs_fill_super);

    if (unlikely(IS_ERR(ret))) {
        printk(KERN_ERR "Error mounting iofs.\n");
    } else {
        printk(KERN_INFO "iofs is succesfully mounted on: %s\n",
               dev_name);
    }

    return ret;
}

void iofs_kill_superblock(struct super_block *sb) {
    printk(KERN_INFO
           "iofs superblock is destroyed. Unmount succesful.\n");
    kill_block_super(sb);
}

void iofs_put_super(struct super_block *sb) {
    return;
}

void iofs_save_sb(struct super_block *sb) {
    struct buffer_head *bh;
    struct iofs_superblock *iofs_sb = IOFS_SB(sb);

    bh = sb_bread(sb, IOFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);

    bh->b_data = (char *)iofs_sb;
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}
