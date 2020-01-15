#include "kiofs.h"

DEFINE_MUTEX(iofs_sb_lock);
DEFINE_MUTEX(iofs_d_lock);

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
    .statfs = iofs_statfs,
};

const struct inode_operations iofs_inode_ops = {
    .create = iofs_create,
    .mkdir = iofs_mkdir,
    .lookup = iofs_lookup,
};

const struct file_operations iofs_dir_operations = {
    .owner		= THIS_MODULE,
    .llseek		= generic_file_llseek,
    .read		= generic_read_dir,
    .iterate_shared	= iofs_iterate_shared,
    .fsync		= generic_file_fsync,
};  

const struct file_operations iofs_file_operations = {
    .read = iofs_read,
    .write = iofs_write,
};

struct kmem_cache *iofs_inode_cache = NULL;


static struct inode *iofs_alloc_inode(struct super_block *sb)
{
	struct iofs_inode_info *ei;
	ei = kmem_cache_alloc(iofs_inode_cache, GFP_KERNEL);
	if (!ei)
		return NULL;
	return &ei->vfs_inode;
}

static void iofs_free_in_core_inode(struct inode *inode)
{
	kmem_cache_free(iofs_inode_cache, iofs_i(inode));
}

static void iofs_free_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);
        printk(KERN_INFO "freeing inode %lu\n", (unsigned long)inode->i_ino);
	kmem_cache_free(iofs_inode_cache, IOFS_INODE(inode));
}

void iofs_inode_free(struct inode *inode)
{
	call_rcu(&inode->i_rcu, iofs_free_callback);
}


static void iofs_inode_init_once(void *i)
{
	struct iofs_inode_info *inode = (struct iofs_inode_info *)i;

	inode_init_once(&inode->vfs_inode);
}


static int iofs_inode_cache_create(void)
{
	iofs_inode_cache = kmem_cache_create("iofs_inode_cache",
		sizeof(struct iofs_inode), 0,
		(SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), iofs_inode_init_once);
	if (iofs_inode_cache == NULL)
		return -ENOMEM;
	return 0;
}


static void iofs_inode_cache_destroy(void)
{
	rcu_barrier();
	kmem_cache_destroy(iofs_inode_cache);
	iofs_inode_cache = NULL;
}


static int __init iofs_init(void)
{
    int ret;

    ret = iofs_inode_cache_create();

    if (ret != 0) {
	printk(KERN_INFO "cannot create inode cache\n");
	return ret;
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
MODULE_AUTHOR("charlesap");
