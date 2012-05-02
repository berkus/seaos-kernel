#include <kernel.h>
#include <memory.h>
#include <task.h>
#include <fs.h>
struct inode *ramfs_root;
int rfs_read(struct inode *i, unsigned int off, unsigned int len, char *b);
struct inode *rfs_create(struct inode *p, char *name, unsigned int mode);
int rfs_write(struct inode *i, unsigned int off, unsigned int len, char *b);
int ramfs_sane(struct inode *i);

int ramfs_op_dummy()
{
	return 0;
}

int ramfs_unlink(struct inode *i)
{
	i->f_count=0;
	i->count=1;
	return 0;
}

struct inode_operations rfs_inode_ops = {
 rfs_read,
 rfs_write,
 (void *)ramfs_op_dummy,
 rfs_create,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
 (void *)ramfs_unlink,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
 (void *)ramfs_op_dummy,
};

struct inode *init_ramfs()
{
	struct inode *i = (struct inode *)kmalloc(sizeof(struct inode));
	i->mode = S_IFDIR | 0x1FF;
	create_mutex(&i->lock);
	strcpy(i->name, "rfs");
	ramfs_root = i;
	i->i_ops = &rfs_inode_ops;
	i->parent = i;
	return i;
}

struct inode *init_tmpfs()
{
	struct inode *i = (struct inode *)kmalloc(sizeof(struct inode));
	i->mode = S_IFDIR | 0x1FF;
	create_mutex(&i->lock);
	strcpy(i->name, "rfs");
	i->i_ops = &rfs_inode_ops;
	i->parent = i;
	return i;
}

int rfs_read(struct inode *i, unsigned int off, unsigned int len, char *b)
{
	int pl = len;
	if((unsigned)off >= i->len)
		return 0;
	if((unsigned)(off+len) >= i->len)
		len = i->len-off;
	if(!len)
		return 0;
	memcpy((void *)b, (void *)(i->start+off), len);
	return len;
}

int rfs_resize(struct inode *i, unsigned int s)
{
	if(s == i->len)
		return 0;
	int new = (int)kmalloc(s);
	int x=0;
	if(i->len > s)
	{
		memcpy((void *)new, (void *)i->start, s);
		x = -1;
	}
	else
	{
		memcpy((void *)new, (void *)i->start, i->len);
		x = 1;
	}
	kfree((void *)i->start);
	i->start = new;
	i->len = s;
	return x;
}

int rfs_write(struct inode *i, unsigned int off, unsigned int len, char *b)
{
	if(!len)
		return -EINVAL;
	if((unsigned)off > i->len)
		rfs_resize(i, len+off);
	if((unsigned)off+len > i->len)
		rfs_resize(i, len+off);
	memcpy((void *)(i->start+off), (void *)b, len);
	return len;
}

struct inode *rfs_create(struct inode *__p, char *name, unsigned int mode)
{
	struct inode *r, *p=__p;
	if(!__p)
		p = ramfs_root;
	if((r = (struct inode *)get_idir(name, p)))
	{
		return r;
	}
	struct inode *root_nodes;
	root_nodes = (struct inode *)kmalloc(sizeof(struct inode));
	strncpy(root_nodes->name, name, 128);
	root_nodes->uid = current_task->uid;
	root_nodes->gid = current_task->gid;
	root_nodes->len = 1;
	root_nodes->i_ops = &rfs_inode_ops;
	root_nodes->mode = mode | 0x1FF;
	root_nodes->start = (int)kmalloc(1);
	create_mutex(&root_nodes->lock);
	if(!__p) add_inode(p, root_nodes);
	return root_nodes;
}
