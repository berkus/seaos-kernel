#ifndef H_EXT2
#define H_EXT2
#include <fs.h>
#include <cache.h>
#include <sys/stat.h>
#include <asm/bitops.h>
#include <task.h>
#include <time.h>
#define uint64_t asdasdasd
#include "sb.h"

typedef struct e2_vol_data {
	int flag;
	unsigned long long dev, block;
	char node[16];
	ext2_superblock_t *sb;
	unsigned block_prev_alloc;
	struct inode *root;
	char read_only;
	mutex_t *m_node, *m_block, *m_inode, ac_lock;
	cache_t *cache;
	struct e2_vol_data *next, *prev;
} ext2_fs_t;

typedef char ext2_inode_type_t;

typedef struct ext2_inode {
	unsigned short mode;
	unsigned short uid;
	unsigned size;
	unsigned access_time;
	unsigned change_time;
	unsigned modification_time;
	unsigned deletion_time;
	unsigned short gid;
	unsigned short link_count;
	unsigned sector_count;
	unsigned flags;
	unsigned os1;
	unsigned blocks[15];
	unsigned gen_num;
	unsigned file_acl;
	unsigned size_up;
	unsigned char pad[20];
	unsigned number;
	ext2_fs_t *fs;
} __attribute__((packed)) ext2_inode_t;

#define IF_SECDEL 0x1
#define IF_KEEP 0x2
#define IF_COMPRESS 0x4
#define IF_SYNC 0x8
#define IF_IMMU 0x10
#define IF_APPEND 0x20
#define IF_NODUMP 0x40
#define IF_NOUPLAT 0x80
#define IF_HASH 10000
#define IF_AFSDIR 0x20000
#define IF_JOURN 0x40000

typedef struct ext2_dirent {
	unsigned inode;
	unsigned short record_len;
	unsigned char name_len;
	unsigned char type;
	unsigned char name[];
} __attribute__((packed)) ext2_dirent_t;

#define EXT2_INODE_MODE_DIR 0x4000
#define EXT2_INODE_IS_DIR(inode) ((inode)->mode & EXT2_INODE_MODE_DIR)
#define ext2_inode_to_internal(x, n) (n-1)
#define ext2_inode_to_external(x, n) (n+1)
#define ext2_inode_from_internal(x, n) (n+1)

#define DET_UNKNOWN 0
#define DET_REG 1
#define DET_DIR 2
#define DET_CHAR 3
#define DET_BLOCK 4
#define DET_FIFO 5
#define DET_SOCK 6
#define DET_SLINK 7

typedef struct ext2_blockgroup {
	/// Nummer des Blocks mit der Block-Allokationsbitmap
	uint32_t block_bitmap;
	
	/// Nummer des Blocks mit der Inode-Allokationsbitmap
	uint32_t inode_bitmap;
	
	/// Nummer des Blocks mit der Inode-Tabelle
	uint32_t inode_table;
	
	/// Anzahl der freien Blöcke
	uint16_t free_blocks;
	
	/// Anzahl der freien Inodes
	uint16_t free_inodes;
	
	/// Anzahl der Verzeichnisse
	uint16_t used_directories;
	uint16_t padding;
	
	/// Reserviert
	uint32_t reserved[3];
} __attribute__((packed)) ext2_blockgroup_t;

extern struct inode_operations e2fs_inode_ops;
int ext2_write_block(ext2_fs_t *fs, u64 block, unsigned char *buf);
int ext2_read_block(ext2_fs_t *fs, u64 block, unsigned char *buf);

int ext2_inode_readblk(ext2_inode_t* inode, uint32_t block, void* buf,
		       size_t count);
int ext2_inode_writeblk(ext2_inode_t* inode, uint32_t block, void* buf);
int ext2_inode_readdata(
	ext2_inode_t* inode, uint32_t start, size_t len, unsigned char* buf);
int ext2_inode_writedata(
	ext2_inode_t* inode, uint32_t start, size_t len, const unsigned char* buf);
int ext2_inode_truncate(ext2_inode_t* inode, uint32_t size, int);
int ext2_bg_read(ext2_fs_t* fs, int group_nr, ext2_blockgroup_t* bg);
int ext2_bg_update(ext2_fs_t* fs, int group_nr, ext2_blockgroup_t* bg);
int ext2_inode_read(ext2_fs_t* fs, uint32_t inode_nr, ext2_inode_t* inode);
int ext2_inode_update(ext2_inode_t* inode);
struct inode *create_sea_inode(ext2_inode_t *in, char *name);
ext2_fs_t *get_fs(int v);
int ext2_inode_type(ext2_inode_t *in);
int ext2_inode_free(ext2_inode_t* inode);
void ext2_inode_release(ext2_inode_t* inode);
int ext2_inode_alloc(ext2_fs_t* fs, ext2_inode_t* inode);
int ext2_sb_update(ext2_fs_t *fs, ext2_superblock_t *sb);
int ext2_write_off(ext2_fs_t *fs, off_t off, unsigned char *buf, size_t len);
int ext2_read_off(ext2_fs_t *fs, off_t off, unsigned char *buf, size_t len);
int ext2_dir_change_type(ext2_inode_t* inode, char *name,
			       unsigned new_type);


int ext2_dir_unlink(ext2_inode_t* dir, const char* name, int);
int ext2_dir_create(ext2_inode_t* parent, const char* name, ext2_inode_t* newi);
int ext2_dir_link(ext2_inode_t* dir, ext2_inode_t* inode, const char* name);
int ext2_dir_get(ext2_inode_t* inode, char* name, ext2_dirent_t*);
int ext2_dir_getnum(ext2_inode_t* inode, unsigned number, char *);



#endif
