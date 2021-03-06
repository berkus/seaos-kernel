#include <kernel.h>
#include <dev.h>
#include <fs.h>
#include <pipe.h>

pipe_t *create_pipe()
{
	pipe_t *pipe = (pipe_t *)kmalloc(sizeof(pipe_t));
	pipe->length = PIPE_SIZE;
	pipe->buffer = (char *)kmalloc(PIPE_SIZE+1);
	pipe->lock = create_mutex(0);
	return pipe;
}

static struct inode *create_anon_pipe()
{
	struct inode *node;
	/* create a 'fake' inode */
	node = (struct inode *)kmalloc(sizeof(struct inode));
	_strcpy(node->name, "~pipe~");
	node->uid = current_task->uid;
	node->gid = current_task->gid;
	node->mode = S_IFIFO | 0x1FF;
	node->count=2;
	node->f_count=2;
	create_mutex(&node->lock);
	
	pipe_t *pipe = create_pipe();
	pipe->count=2;
	pipe->wrcount=1;
	node->pipe = pipe;
	return node;
}

int sys_pipe(int *files)
{
	if(!files) return -EINVAL;
	struct file *f;
	struct inode *inode = create_anon_pipe();
	/* this is the reading descriptor */
	f = (struct file *)kmalloc(sizeof(struct file));
	f->inode = inode;
	f->flags = _FREAD;
	f->pos=0;
	f->count=1;
	int read = add_file_pointer((task_t *)current_task, f);
	/* this is the writing descriptor */
	f = (struct file *)kmalloc(sizeof(struct file));
	f->inode = inode;
	f->flags = _FREAD | _FWRITE;
	f->count=1;
	f->pos=0;
	int write = add_file_pointer((task_t *)current_task, f);
	files[0]=read;
	files[1]=write;
	return 0;
}

void free_pipe(struct inode *i)
{
	if(!i || !i->pipe) return;
	kfree((void *)i->pipe->buffer);
	destroy_mutex(i->pipe->lock);
	kfree(i->pipe);
	i->pipe=0;
}

__attribute__((optimize("O0"))) int read_pipe(struct inode *ino, char *buffer, size_t length)
{
	if(!ino || !buffer)
		return -EINVAL;
	pipe_t *pipe = ino->pipe;
	if(!pipe)
		return -EINVAL;
	unsigned len = length;
	int ret=0;
	size_t count=0;
	/* should we even try reading? (empty pipe with no writing processes=no) */
	if(!pipe->pending && pipe->count <= 1 && pipe->type != PIPE_NAMED)
		return count;
	/* block until we can get access */
	mutex_on(pipe->lock);
	while(!pipe->pending && (pipe->count > 1 && pipe->type != PIPE_NAMED 
			&& pipe->wrcount>0)) {
		mutex_off(pipe->lock);
		force_schedule();
		if(current_task->sigd)
			return -EINTR;
		mutex_on(pipe->lock);
	}
	ret = pipe->pending > len ? len : pipe->pending;
	/* note: this is a quick implementation of line-buffering that should
	 * work for most cases. There is currently no way to disable line
	 * buffering in pipes, but I don't care, because there shouldn't be a
	 * reason to. TODO maybe? */
	char *nl = strchr((char *)pipe->buffer+pipe->read_pos, '\n');
	if(nl && (nl-(pipe->buffer+pipe->read_pos)) < ret)
		ret = (nl-(pipe->buffer+pipe->read_pos))+1;
	memcpy((void *)(buffer + count), (void *)(pipe->buffer + pipe->read_pos), ret);
	memcpy((void *)pipe->buffer, (void *)(pipe->buffer + pipe->read_pos + ret), 
		PIPE_SIZE - (pipe->read_pos + ret));
	if(ret > 0) {
		pipe->pending -= ret;
		pipe->write_pos -= ret;
		len -= ret;
		count+=ret;
	}
	mutex_off(pipe->lock);
	return count;
}

__attribute__((optimize("O0"))) int write_pipe(struct inode *ino, char *buffer, size_t length)
{
	if(!ino || !buffer)
		return -EINVAL;
	pipe_t *pipe = ino->pipe;
	if(!pipe)
		return -EINVAL;
	mutex_on(pipe->lock);
	/* we're writing to a pipe with no reading process! */
	if(pipe->count <= 1 && pipe->type != PIPE_NAMED) {
		mutex_off(pipe->lock);
		return -EPIPE;
	}
	/* IO block until we can write to it */
	while((pipe->write_pos+length)>=PIPE_SIZE) {
		mutex_off(pipe->lock);
		wait_flag_except((unsigned *)&pipe->write_pos, pipe->write_pos);
		if(current_task->sigd)
			return -EINTR;
		mutex_on(pipe->lock);
	}
	
	/* this shouldn't happen, but lets be safe */
	if((pipe->write_pos+length)>=PIPE_SIZE)
	{
		printk(1, "[pipe]: warning - task %d failed to block for writing to pipe\n"
			, current_task->pid);
		mutex_off(pipe->lock);
		return -EPIPE;
	}
	memcpy((void *)(pipe->buffer + pipe->write_pos), buffer, length);
	pipe->length = ino->len;
	pipe->write_pos += length;
	pipe->pending += length;
	mutex_off(pipe->lock);
	return length;
}

int pipedev_select(struct inode *in, int rw)
{
	if(rw != READ)
		return 1;
	pipe_t *pipe = in->pipe;
	if(!pipe) return 1;
	if(!pipe->pending && (pipe->count > 1 && pipe->type != PIPE_NAMED 
			&& pipe->wrcount>0))
		return 0;
	return 1;
}
