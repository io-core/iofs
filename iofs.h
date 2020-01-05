#ifndef __IOFS_H__
#define __IOFS_H__

#define BITS_IN_BYTE 8
#define IOFS_MAGIC 0x9b1ea38d  // [ 141 163 30 155 ] 8D A3 1E 9B -- Oberon Directory Mark
#define IOFS_DEFAULT_BLOCKSIZE 1024 // 4096
#define IOFS_DEFAULT_INODE_TABLE_SIZE 256 // 1024
#define IOFS_DEFAULT_DATA_BLOCK_TABLE_SIZE 256 // 1024
#define IOFS_FILENAME_MAXLEN 63

#define IOFS_DIRMARK 0x9B1EA38D
#define IOFS_HEADERMARK 0x9BA71D86



/* Define filesystem structures */

extern struct mutex iofs_sb_lock;

struct iofs_dir_record {
    char filename[IOFS_FILENAME_MAXLEN];
    uint64_t inode_no;
};

struct iofs_inode {
    mode_t mode;
    uint64_t inode_no;
    uint64_t data_block_no;

    // TODO struct timespec is defined kenrel space,
    // but mkfs-iofs.c is compiled in user space
    /*struct timespec atime;
    struct timespec mtime;
    struct timespec ctime;*/

    union {
        uint64_t file_size;
        uint64_t dir_children_count;
    };
};


struct  iofs_direntry {  // B-tree node
    char name[32];
    uint32_t  adr;       // sec no of file header
    uint32_t  p;         // sec no of descendant in directory
};

struct iofs_superblock {
    uint32_t magic;
    uint32_t m;
    uint32_t p0;         //sec no of left descendant in directory
    char fill[52];
    struct iofs_direntry e[24]; 
/*
    uint32_t version;
    uint64_t blocksize;

    uint64_t inode_table_size;
    uint64_t inode_count;

    uint64_t data_block_table_size;
    uint64_t data_block_count;
*/

};

static const uint64_t IOFS_SUPERBLOCK_BLOCK_NO = 0;
static const uint64_t IOFS_INODE_BITMAP_BLOCK_NO = 1;
static const uint64_t IOFS_DATA_BLOCK_BITMAP_BLOCK_NO = 2;
static const uint64_t IOFS_INODE_TABLE_START_BLOCK_NO = 3;

static const uint64_t IOFS_ROOTDIR_INODE_NO = 0;
// data block no is the absolute block number from start of device
// data block no offset is the relative block offset from start of data block table
static const uint64_t IOFS_ROOTDIR_DATA_BLOCK_NO_OFFSET = 0;

/* Helper functions */

static inline uint64_t IOFS_INODES_PER_BLOCK_HSB(
        struct iofs_superblock *iofs_sb) {
    return iofs_sb->blocksize / sizeof(struct iofs_inode);
}

static inline uint64_t IOFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(
        struct iofs_superblock *iofs_sb) {
    return IOFS_INODE_TABLE_START_BLOCK_NO
           + iofs_sb->inode_table_size / IOFS_INODES_PER_BLOCK_HSB(iofs_sb)
           + 1;
}

#endif /*__IOFS_H__*/
