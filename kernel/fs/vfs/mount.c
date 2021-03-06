#include <kernel.h>
#include <memory.h>
#include <task.h>
#include <asm/system.h>
#include <dev.h>
#include <fs.h>
#include <ll.h>
struct inode *init_tmpfs();

int do_mount(struct inode *i, struct inode *p)
{
	if(current_task->uid)
		return -EACCES;
	if(i == current_task->root)
		return -EINVAL;
	if(!is_directory(i))
		return -ENOTDIR;
	if(!is_directory(p))
		return -EIO;
	mutex_on(&i->lock);
	if(i->mount) {
		mutex_off(&i->lock);
		return -EBUSY;
	}
	i->mount = (mount_pt_t *)kmalloc(sizeof(mount_pt_t));
	i->mount->root = p;
	i->mount->parent = i;
	p->mount_parent = i;
	mutex_off(&i->lock);
	struct mountlst *ent = (void *)kmalloc(sizeof(struct mountlst));
	ent->node = ll_insert(mountlist, ent);
	ent->i = p;
	return 0;
}

int mount(char *d, struct inode *p)
{
	if(!p)
		return -EINVAL;
	struct inode *i = get_idir(d, 0);
	if(!i)
		return -ENOENT;
	return do_mount(i, p);
}

int s_mount(char *name, int dev, u64 block, char *fsname, char *no)
{
	struct inode *i=0;
	if(!fsname || !*fsname)
		i=sb_check_all(dev, block, no);
	else
		i=sb_callback(fsname, dev, block, no);
	if(!i)
		return -EIO;
	i->dev = dev;
	int ret = mount(name, i);
	if(!ret) return 0;
	vfs_callback_unmount(i, i->sb_idx);
	iput(i);
	return ret;
}

int sys_mount2(char *node, char *to, char *name, char *opts, int flags)
{
	if(!to) return -EINVAL;
	if(name && (!node || *node == '*')) {
		if(!strcmp(name, "devfs")) {
			iremove_nofree(devfs_root);
			return mount(to, devfs_root);
		}
		if(!strcmp(name, "procfs")) {
			iremove_nofree(procfs_root);
			return mount(to, procfs_root);
		}
		if(!strcmp(name, "tmpfs"))
			return mount(to, init_tmpfs());
		return -EINVAL;
	}
	if(!node)
		return -EINVAL;
	struct inode *i=0;
	i = get_idir(node, 0);
	if(!i)
		return -ENOENT;
	int dev = i->dev;
	iput(i);
	return s_mount(to, dev, 0, name, node);
}

int sys_mount(char *node, char *to)
{
	return sys_mount2(node, to, 0, 0, 0);
}

int do_unmount(struct inode *i, int flags)
{
	if(!i || !i->mount)
		return -EINVAL;
	if(current_task->uid)
		return -EACCES;
	if(!is_directory(i))
		return -ENOTDIR;
	struct inode *m = i->mount->root;
	if(m->required || m->count>1)
		return -EBUSY;
	mount_pt_t *mt = i->mount;
	mutex_on(&i->lock);
	i->mount=0;
	lock_scheduler();
	int c = recur_total_refs(m);
	if(c && (!(flags&1) && !current_task->uid))
	{
		i->mount=mt;
		unlock_scheduler();
		mutex_off(&i->lock);
		return -EBUSY;
	}
	unlock_scheduler();
	vfs_callback_unmount(m, m->sb_idx);
	m->mount_parent=0;
	struct mountlst *lst = get_mount(m);
	ll_remove(mountlist, lst->node);
	kfree(lst);
	if(m != devfs_root && m != procfs_root)
		iremove_recur(m);
	kfree(mt);
	return 0;
}

int unmount(char *n, int flags)
{
	if(!n) return -EINVAL;
	struct inode *i=0;
	i = get_idir(n, 0);
	if(!i)
		return -ENOENT;
	if(!i->mount_parent)
		return -ENOENT;
	struct inode *got = i;
	i = i->mount_parent;
	int ret = do_unmount(i, flags);
	if(ret) iput(got);
	else iput(i);
	return ret;
}
