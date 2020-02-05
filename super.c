#include "kiofs.h"

static int iofs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode *root_inode;
    struct iofs_inode *root_iofs_inode;
    int ret = 0;

    sb->s_magic = (uint32_t)IOFS_MAGIC;
    sb->s_maxbytes = 1024; 
    sb->s_op = &iofs_sb_ops;

    root_iofs_inode = iofs_get_iofs_inode(sb, 1); 
    root_inode = new_inode(sb);
    if (!root_inode || !root_iofs_inode) {
      ret = -ENOMEM; 
      goto release;
    }
    iofs_fill_inode(sb, root_inode, root_iofs_inode, 1);
    inode_init_owner(root_inode, NULL, root_inode->i_mode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto release;
    }
release:
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
    return;
}
