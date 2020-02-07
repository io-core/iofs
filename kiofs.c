#include "kiofs.h"

DEFINE_MUTEX(iofs_sb_lock);

struct file_system_type iofs_fs_type = {
    .owner = THIS_MODULE,
    .name = "iofs",
    .mount = iofs_mount,
    .kill_sb = iofs_kill_superblock,
    .fs_flags = FS_REQUIRES_DEV,
};

const struct super_operations iofs_sb_ops = {
    .destroy_inode = iofs_destroy_inode,
    .put_super = iofs_put_super,
};

const struct inode_operations iofs_inode_ops = {
    .create = iofs_create,
    .mkdir = iofs_mkdir,
    .lookup = iofs_lookup,
};

const struct file_operations iofs_dir_operations = {
    .owner = THIS_MODULE,
    .iterate_shared = iofs_iterate_shared,
};

const struct file_operations iofs_file_operations = {
    .read = iofs_read,
    .write = iofs_write,
};

struct kmem_cache *iofs_inode_cache = NULL;

static void iofs_inode_cache_destroy(void)
{
        rcu_barrier();
        kmem_cache_destroy(iofs_inode_cache);
        iofs_inode_cache = NULL;
}

static int __init iofs_init(void)
{
    int ret;

    iofs_inode_cache = kmem_cache_create("iofs_inode_cache",
                                         sizeof(struct iofs_inode),
                                         0,
                                         (SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD),
                                         NULL);
    if (!iofs_inode_cache) {
        return -ENOMEM;
    }

    ret = register_filesystem(&iofs_fs_type);
    if (likely(0 == ret)) {
        printk(KERN_INFO "Sucessfully registered iofs\n");
    } else {
        printk(KERN_ERR "Failed to register iofs. Error code: %d\n", ret);
    }

    return ret;
}

static void __exit iofs_exit(void)
{
    int ret;

    ret = unregister_filesystem(&iofs_fs_type);
    iofs_inode_cache_destroy();

//    kmem_cache_destroy(iofs_inode_cache);

    if (likely(ret == 0)) {
        printk(KERN_INFO "Sucessfully unregistered iofs\n");
    } else {
        printk(KERN_ERR "Failed to unregister iofs. Error code: %d\n",
               ret);
    }
}

module_init(iofs_init);
module_exit(iofs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("accelazh");
