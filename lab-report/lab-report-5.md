# Lab Report 5

519021910685

Grizzy@sjtu.edu.cn

*注：本次lab没有思考题，因此叙述较为简单*

## 1

提供了mknod和namex的实现

mknod根据mkdir判断创建regular file还是directory，需要注意的是传入参数的len指的是name length

namex为一个递归函数，递归地解析路径名并且更新dirat和name

```c
static int tfs_mknod(struct inode *dir, const char *name, size_t len, int mkdir)
{
	struct inode *inode;
	struct dentry *dent;

	BUG_ON(!name);

	if (len == 0) {
		WARN("mknod with len of 0");
		return -ENOENT;
	}
	/* LAB 5 TODO BEGIN */
	if(mkdir==true) {
		inode = new_dir();
	}
	else {
		inode = new_reg();
	}
	inode->size = 0;
	dent = new_dent(inode, name, len);
	htable_add(&(dir->dentries), dent->name.hash, &(dent->node));

	/* LAB 5 TODO END */

	return 0;
}


int tfs_namex(struct inode **dirat, const char **name, int mkdir_p)
{
	BUG_ON(dirat == NULL);
	BUG_ON(name == NULL);
	BUG_ON(*name == NULL);

	char buff[MAX_FILENAME_LEN + 1];
	int i;
	struct dentry *dent;
	int err;

	if (**name == '/') {
		*dirat = tmpfs_root;
		// make sure `name` starts with actual name
		while (**name && **name == '/')
			++(*name);
	} else {
		BUG_ON(*dirat == NULL);
		BUG_ON((*dirat)->type != FS_DIR);
	}

	// make sure a child name exists
	if (!**name)
		return -EINVAL;

	// `tfs_lookup` and `tfs_mkdir` are useful here

	/* LAB 5 TODO BEGIN */

	char *tmp = *name;
	i = 0;
	while(tmp[i]!='\0' && tmp[i]!='/')
		i++;
	strncpy(buff, tmp, i);
	buff[i] = '\0';
	dent = tfs_lookup(*dirat, buff, i);
	// whether the dentry exists or not?
	if(dent==NULL) {
		// create intermediate directory entry
		if(mkdir_p==true && tmp[i]=='/') {
			tfs_mkdir(*dirat, buff, i);
			dent = tfs_lookup(*dirat, buff, i);
		}
		else {
			return -ENOENT;
		}
	}
	// the named dentry is found or and intermediate dentry is created
	// the last component and it does not end with '/'
	if(tmp[i]=='\0') {
		return 0;
	}
	// the last component and it ends with '/'
	else if(tmp[i]=='/'&&tmp[i+1]=='\0') {
		*dirat = dent->inode;
		*name = tmp + i + 1;
		return 0;
	}
	// intermediate path but not a dentry
	else if(dent->inode->type!=FS_DIR) {
		return -ENOTDIR;
	}
	// intermediate dentry
	else{
		*dirat = dent->inode;
		*name = tmp + i + 1;
		return tfs_namex(dirat, name, mkdir_p);
	}

	/* LAB 5 TODO END */

	/* we will never reach here? */
	return 0;
}
```

## 2

tfs_file_read和tfs_file_write的实现

需要注意的是，都要判断一下文件的大小，分配空间对文件大小进行动态的调整，或者是读入少于size的数据

```c
ssize_t tfs_file_read(struct inode * inode, off_t offset, char *buff,
		      size_t size)
{
	BUG_ON(inode->type != FS_REG);
	BUG_ON(offset > inode->size);

	u64 page_no, page_off;
	u64 cur_off = offset;
	size_t to_read;
	void *page;

	/* LAB 5 TODO BEGIN */
	u64 page_current_count = (inode->size + PAGE_SIZE - 1) / PAGE_SIZE;
	u64 page_end_count  = (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
	page_no = offset / PAGE_SIZE;
	page_off = offset % PAGE_SIZE;
	size_t have_read = 0;
	size_t part;
	if(offset >= inode->size) {
		to_read = 0;
	} else if(offset + size > inode->size) {
		to_read = inode->size - offset;
	} else {
		to_read = size;
	}

	for(u64 idx = page_no;idx < page_end_count;++idx) {
		page = radix_get(&inode->data, idx);
		if(idx==page_no) {
			part = page_off + to_read > PAGE_SIZE ? PAGE_SIZE - page_off : to_read;
			memcpy(buff + have_read, page + page_off, part);
			have_read += part;
		}
		else {
			part = to_read - have_read > PAGE_SIZE ? PAGE_SIZE : to_read - have_read;
			memcpy(buff + have_read, page, part);
			have_read += part;
		}
	}
	/* LAB 5 TODO END */

	return to_read;
}

ssize_t tfs_file_write(struct inode * inode, off_t offset, const char *data,
		       size_t size)
{
	BUG_ON(inode->type != FS_REG);
	BUG_ON(offset > inode->size);

	u64 page_no, page_off;
	u64 cur_off = offset;
	size_t to_write;
	void *page;

	/* LAB 5 TODO BEGIN */
	u64 page_current_count = (inode->size + PAGE_SIZE - 1) / PAGE_SIZE;
	u64 page_end_count  = (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
	inode->size = inode->size > offset + size ? inode->size : offset+size;
	page_no = offset / PAGE_SIZE;
	page_off = offset % PAGE_SIZE;
	size_t written = 0;
	if(page_end_count > page_current_count) {
		for(u64 idx = page_current_count;idx < page_end_count;idx++) {
			page = calloc(1, PAGE_SIZE);
			radix_add(&inode->data, idx, page);
		}
	}
	for(u64 idx = page_no;idx < page_end_count;++idx) {
		page = radix_get(&inode->data, idx);
		if(idx==page_no) {
			to_write = page_off+size > PAGE_SIZE ? PAGE_SIZE - page_off : size;
			memcpy(page + page_off, data + written, to_write);
			written += to_write;
		}
		else {
			to_write = size-written > PAGE_SIZE ? PAGE_SIZE : size-written;
			memcpy(page, data + written, to_write);
			written += to_write;
		}
	}

	/* LAB 5 TODO END */

	return size;
}
```

## 3

tfs_load_image的实现

```c
int tfs_load_image(const char *start)
{
	struct cpio_file *f;
	struct inode *dirat;
	struct dentry *dent;
	const char *leaf;
	size_t len;
	int err;
	ssize_t write_count;

	BUG_ON(start == NULL);

	cpio_init_g_files();
	cpio_extract(start, "/");

	for (f = g_files.head.next; f; f = f->next) {
	/* LAB 5 TODO BEGIN */
	dirat = tmpfs_root;
	leaf = f->name;
	len = 0;
	int err = tfs_namex(&dirat, &leaf, 1);
	while(leaf[len]!='\0')
		len++;
	if(len==1 && leaf[0]=='.') {
		continue;
	}
	if(err == 0) {
		if(len==0) {
			// the last component ends with '/' and it is a directory
			continue;
		} else {
			// the last component should not be a directory
			dent = tfs_lookup(dirat, leaf, len);
			BUG_ON(dent==NULL);
			if(dent->inode->type==FS_DIR) return -EISDIR;
			size_t written = tfs_file_write(dent->inode, 0, f->data, f->header.c_filesize);
		}
	} else if(err == -ENOENT) {
		// the last component, which is a file, is missing
		tfs_creat(dirat, leaf, len);
		dent = tfs_lookup(dirat, leaf, len);
		size_t written = tfs_file_write(dent->inode, 0, f->data, f->header.c_filesize);
	} else {
		BUG_ON(err);
	}

	/* LAB 5 TODO END */
	}

	return 0;
}
```

## 4

只需要根据已经实现的接口进行调用，即可完成本部分内容。

```c
int fs_creat(const char *path)
{
	struct inode *dirat = NULL;
	const char *leaf = path;
	int err;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	/* LAB 5 TODO BEGIN */
	err = tfs_namex(&dirat, &leaf, 1);
	if(err != -ENOENT) return -EEXIST;
	err = tfs_creat(dirat, leaf, strlen(leaf));
	/* LAB 5 TODO END */
	return 0;

}

int tmpfs_unlink(const char *path, int flags)
{
	struct inode *dirat = NULL;
	const char *leaf = path;
	int err;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	/* LAB 5 TODO BEGIN */
	err = tfs_namex(&dirat, &leaf, 1);
	if(err) return err;
	size_t len = 0;
	while(leaf[len]!='\0') len++;
	tfs_remove(dirat, leaf, len);
	/* LAB 5 TODO END */
	return err;
}

int tmpfs_mkdir(const char *path, mode_t mode)
{
	struct inode *dirat = NULL;
	const char *leaf = path;
	int err;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	/* LAB 5 TODO BEGIN */
	err = tfs_namex(&dirat, &leaf, 1);
	if(err!=-ENOENT) return -EEXIST;
	size_t len = 0;
	while(leaf[len]!='\0') len++;
	err = tfs_mkdir(dirat, leaf, len);
	/* LAB 5 TODO END */
	return err;
}
```

## 5

调用chcore提供的系统调用完成getch函数

readline函数需要判读当前输入的是什么字符，并且将其实时呈现在console上。如果是tab则需要进行额外的自动补全，输入回车表示命令输入完成，执行对应的命令

```c
char getch()
{
	int c;
	/* LAB 5 TODO BEGIN */
	c = getc();
	/* LAB 5 TODO END */

	return (char) c;
}

char *readline(const char *prompt)
{
	static char buf[BUFLEN];

	int i = 0, j = 0;
	signed char c = 0;
	int ret = 0;
	char complement[BUFLEN];
	int complement_time = 0;

	if (prompt != NULL) {
		printf("%s", prompt);
	}

	while (1) {
    __chcore_sys_yield();
		c = getch();

	/* LAB 5 TODO BEGIN */
	/* Fill buf and handle tabs with do_complement(). */
	if(c=='\r' || c=='\n') {
		putc('\n');
		break;
	} else if(c=='\t') {
		putc(' ');
		complement_time++;
		do_complement(buf, complement, complement_time);
	} else {
		putc(c);
		buf[i] = c;
		i++;
	}

	/* LAB 5 TODO END */
	}

	return buf;
}
```

## 6

实现几个命令对应的函数，只需要构造相应的ipc_msg，进行ipc即可完成本部分内容

```c
void print_file_content(char* path) 
{

	/* LAB 5 TODO BEGIN */
	int fd = alloc_fd();
	int ret;
	struct ipc_msg *ipc_msg = ipc_create_msg(fs_ipc_struct_for_shell, sizeof(struct fs_request), 0);
	chcore_assert(ipc_msg);
	struct fs_request *fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_OPEN;
	fr->open.new_fd = fd;
	strcpy(fr->open.pathname, path);
	// the `flags` and `mode` fields are useless
	// fr->open.flags = 0;
	// fr->open.mode = 0;
	ret = ipc_call(fs_ipc_struct_for_shell, ipc_msg);
	ipc_destroy_msg(fs_ipc_struct_for_shell, ipc_msg);

	char file_buf[BUFLEN];
	ret = readfile(fd, file_buf, BUFLEN);
	printf("%s", file_buf);

	ipc_msg = ipc_create_msg(fs_ipc_struct_for_shell, sizeof(struct fs_request), 0);
	chcore_assert(ipc_msg);
	fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_CLOSE;
	fr->close.fd = fd;
	ret = ipc_call(fs_ipc_struct_for_shell, ipc_msg);
	ipc_destroy_msg(fs_ipc_struct_for_shell, ipc_msg);

	/* LAB 5 TODO END */

}

void fs_scan(char *path)
{

	/* LAB 5 TODO BEGIN */
	int fd = alloc_fd();
	int ret;
	struct ipc_msg *ipc_msg = ipc_create_msg(fs_ipc_struct_for_shell, sizeof(struct fs_request), 0);
	chcore_assert(ipc_msg);
	struct fs_request *fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_OPEN;
	fr->open.new_fd = fd;
	strcpy(fr->open.pathname, path);
	// the `flags` and `mode` fields are useless
	// fr->open.flags = 0;
	// fr->open.mode = 0;
	ret = ipc_call(fs_ipc_struct_for_shell, ipc_msg);
	ipc_destroy_msg(fs_ipc_struct_for_shell, ipc_msg);

	char name[BUFLEN];
	char scan_buf[BUFLEN];
	char tmp[BUFLEN];
	int offset;
	struct dirent *p;
	ret = getdents(fd, scan_buf, BUFLEN);
	int filled = 0;
	int len = 0;
	for (offset = 0; offset < ret; offset += p->d_reclen) {
		p = (struct dirent *)(scan_buf + offset);
		get_dent_name(p, tmp);
		printf("%s ", tmp);
	}

	ipc_msg = ipc_create_msg(fs_ipc_struct_for_shell, sizeof(struct fs_request), 0);
	chcore_assert(ipc_msg);
	fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_CLOSE;
	fr->close.fd = fd;
	ret = ipc_call(fs_ipc_struct_for_shell, ipc_msg);
	ipc_destroy_msg(fs_ipc_struct_for_shell, ipc_msg);

	/* LAB 5 TODO END */
}
```

## 7

do_complement和run_cmd函数

```c
int run_cmd(char *cmdline)
{
	int cap = 0;
	/* Hint: Function chcore_procm_spawn() could be used here. */
	/* LAB 5 TODO BEGIN */
	chcore_procm_spawn(cmdline, &cap);
	/* LAB 5 TODO END */
	return 0;
}

int do_complement(char *buf, char *complement, int complement_time)
{
	int ret = 0, j = 0;
	struct dirent *p;
	char name[BUFLEN];
	char scan_buf[BUFLEN];
	int r = -1;
	int offset;

	/* LAB 5 TODO BEGIN */
	int fd = alloc_fd();
	const char path[] = "/";
	struct ipc_msg *ipc_msg = ipc_create_msg(fs_ipc_struct_for_shell, sizeof(struct fs_request), 0);
	chcore_assert(ipc_msg);
	struct fs_request *fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_OPEN;
	fr->open.new_fd = fd;
	strcpy(fr->open.pathname, path);
	// the `flags` and `mode` fields are useless
	// fr->open.flags = 0;
	// fr->open.mode = 0;
	ret = ipc_call(fs_ipc_struct_for_shell, ipc_msg);
	ipc_destroy_msg(fs_ipc_struct_for_shell, ipc_msg);

	ret = getdents(fd, scan_buf, BUFLEN);
	for (offset = 0; offset < ret; offset += p->d_reclen) {
		p = (struct dirent *)(scan_buf + offset);
		j++;
		get_dent_name(p, complement);
		if(j==complement_time) {
			printf("%s", complement);
		}
	}

	ipc_msg = ipc_create_msg(fs_ipc_struct_for_shell, sizeof(struct fs_request), 0);
	chcore_assert(ipc_msg);
	fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_CLOSE;
	fr->close.fd = fd;
	ret = ipc_call(fs_ipc_struct_for_shell, ipc_msg);
	ipc_destroy_msg(fs_ipc_struct_for_shell, ipc_msg);

	/* LAB 5 TODO END */

	return r;
}
```

## 8

封装了fread、fwrite、fopen、fscanf、fprintf函数

FILE结构体的设计不需要维护buffer和pos信息，因为fd已经在文件系统底层映射并且维护了相关信息

这部分难点在于fprintf和fscanf函数解析format

下面给出部分实现，具体实现详见源码

```c
int fprintf(FILE * f, const char * fmt, ...) {

	/* LAB 5 TODO BEGIN */
	va_list va;
	va_start(va, fmt);
	char buf[512];

	int idx_fmt = 0, idx_buf = 0;
	int len_fmt = strlen(fmt);
	int val;
	char *str_ptr;
	while(idx_fmt < len_fmt) {
		if(fmt[idx_fmt]=='%') {
			idx_fmt++;
			if(fmt[idx_fmt]=='d') {
				val = va_arg(va, int);
				idx_fmt++;
				fprintf_handle_int(val, &idx_buf, buf);
			} else if(fmt[idx_fmt]=='s') {
				str_ptr = va_arg(va, char*);
				idx_fmt++;
				fprintf_handle_str(str_ptr, &idx_buf, buf);
			} else {
				return -1;
			}
		} else {
			buf[idx_buf] = fmt[idx_fmt];
			idx_fmt++;
			idx_buf++;
		}
	}
	fwrite(buf, strlen(buf), 1, f);

	/* LAB 5 TODO END */
    return 0;
}

int fscanf(FILE * f, const char * fmt, ...) {

	/* LAB 5 TODO BEGIN */
	va_list va;
	va_start(va, fmt);
	char buf[512];
	size_t size = fread(buf, 512, 1, f);

	int idx_fmt = 0, idx_buf = 0;
	int len_fmt = strlen(fmt);
	int *int_ptr;
	char *str_ptr;
	while(idx_fmt < len_fmt) {
		if(fmt[idx_fmt]=='%') {
			idx_fmt++;
			if(fmt[idx_fmt]=='d') {
				int_ptr = va_arg(va, int*);
				idx_fmt++;
				fscanf_handle_int(int_ptr, &idx_buf, buf);
			} else if(fmt[idx_fmt]=='s') {
				str_ptr = va_arg(va, char*);
				idx_fmt++;
				fscanf_handle_str(str_ptr, &idx_buf, buf);
			} else {
				return -1;
			}
		} else {
			idx_fmt++;
			idx_buf++;
		}
	}

	/* LAB 5 TODO END */
    return 0;
}
```

## 9

这部分需要注意，需要自己维护fd、mount path和mount info的映射关系，mount info中存有对应文件系统的ipc_struct，通过对应文件系统的ipc_struct发送ipc请求就可以将operation分发到不同的文件系统上去。

```c
void fsm_server_dispatch(struct ipc_msg *ipc_msg, u64 client_badge)
{
	int ret;
	bool ret_with_cap = false;
	struct fs_request *fr;
	fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
	struct mount_point_info_node *mpinfo = NULL;

	/* You could add code here as you want.*/
	/* LAB 5 TODO BEGIN */
	int fd;
	struct ipc_msg *ipc_msg_transfer;
	struct fs_request *fr_transfer;

	/* LAB 5 TODO END */

	spinlock_lock(&fsmlock);

	switch(fr->req) {
		case FS_REQ_MOUNT:
			ret = fsm_mount_fs(fr->mount.fs_path, fr->mount.mount_path); // path=(device_name), path2=(mount_point)
			break;
		case FS_REQ_UMOUNT:
			ret = fsm_umount_fs(fr->mount.fs_path);
			break;
		case FS_REQ_GET_FS_CAP:
			mpinfo = get_mount_point(fr->getfscap.pathname, strlen(fr->getfscap.pathname));
			strip_path(mpinfo, fr->getfscap.pathname);
			ipc_msg->cap_slot_number = 1;
			ipc_set_msg_cap(ipc_msg, 0, mpinfo->fs_cap);
			ret_with_cap = true;
			break;

		/* LAB 5 TODO BEGIN */
		case FS_REQ_OPEN:
			fd = fr->open.new_fd;
			mpinfo = get_mount_point(fr->open.pathname, strlen(fr->open.pathname));
			strip_path(mpinfo, fr->open.pathname);
			ipc_msg_transfer = ipc_create_msg(mpinfo->_fs_ipc_struct, sizeof(struct fs_request), 0);
			fr_transfer = (struct fs_request*)ipc_get_msg_data(ipc_msg_transfer);
			memcpy(fr_transfer, fr, sizeof(struct fs_request));
			ret = ipc_call(mpinfo->_fs_ipc_struct, ipc_msg_transfer);
			memcpy(ipc_get_msg_data(ipc_msg), ipc_get_msg_data(ipc_msg_transfer), ipc_msg_transfer->data_len);
			ipc_destroy_msg(mpinfo->_fs_ipc_struct, ipc_msg_transfer);
			fsm_set_mount_info_withfd(client_badge, fd, mpinfo);
			break;
		case FS_REQ_CLOSE:
			ret = transfer_ipc_with_fd(fr->close.fd, ipc_msg, fr, client_badge);
			break;
		case FS_REQ_CREAT:
			ret = transfer_ipc_with_pathname(fr->creat.pathname, ipc_msg, fr);
			break;
		case FS_REQ_MKDIR:
			ret = transfer_ipc_with_pathname(fr->mkdir.pathname, ipc_msg, fr);
			break;
		case FS_REQ_RMDIR:
			ret = transfer_ipc_with_pathname(fr->rmdir.pathname, ipc_msg, fr);
			break;
		case FS_REQ_UNLINK:
			ret = transfer_ipc_with_pathname(fr->unlink.pathname, ipc_msg, fr);
			break;
		case FS_REQ_READ:
			ret = transfer_ipc_with_fd(fr->read.fd, ipc_msg, fr, client_badge);
			break;
		case FS_REQ_WRITE:
			ret = transfer_ipc_with_fd(fr->write.fd, ipc_msg, fr, client_badge);
			break;
		case FS_REQ_GET_SIZE:
			ret = transfer_ipc_with_pathname(fr->getsize.pathname, ipc_msg, fr);
			break;
		case FS_REQ_LSEEK:
			ret = transfer_ipc_with_fd(fr->lseek.fd, ipc_msg, fr, client_badge);
			break;
		case FS_REQ_GETDENTS64:
			ret = transfer_ipc_with_fd(fr->getdents64.fd, ipc_msg, fr, client_badge);
			break;

		/* LAB 5 TODO END */

		default:
			printf("[Error] Strange FS Server request number %d\n", fr->req);
			ret = -EINVAL;
		break;

	}
	
	spinlock_unlock(&fsmlock);

	if(ret_with_cap) {
		ipc_return_with_cap(ipc_msg, ret);
	} else {
		ipc_return(ipc_msg, ret);
	}
}
```

## 10

根据FS_REQ_GET_FS_CAP获取对应文件系统的cap，然后直接想对应文件系统发送ipc请求

```c
nt fsm_write_file(const char* path, char* buf, unsigned long size) {
        if (!fsm_ipc_struct) {
                connect_fsm_server();
        }
        int ret = 0;

        /* LAB 5 TODO BEGIN */
        u64 cap;
        int fd;
        struct fs_cap_info_node *fs_cap_info_node;
        struct ipc_struct *fsx_ipc_struct;
        struct ipc_msg *fsm_ipc_msg, *fsx_ipc_msg;
        struct fs_request *fsm_fr_request, *fsx_fr_request;
        
        fsm_ipc_msg = ipc_create_msg(fsm_ipc_struct, sizeof(struct fs_request), 0);
        fsm_fr_request = (struct fs_request*)ipc_get_msg_data(fsm_ipc_msg);
        fsm_fr_request->req = FS_REQ_GET_FS_CAP;
        memcpy(fsm_fr_request->getfscap.pathname, path, size);
        ret = ipc_call(fsm_ipc_struct, fsm_ipc_msg);
        fsm_fr_request = (struct fs_request*)ipc_get_msg_data(fsm_ipc_msg);
        cap = ipc_get_msg_cap(fsm_ipc_msg, 0);
        ipc_destroy_msg(fsm_ipc_struct, fsm_ipc_msg);
        
        fs_cap_info_node = get_fs_cap_info(cap);
        fsx_ipc_struct = fs_cap_info_node->fs_ipc_struct;

        fd = alloc_fd();
        fsx_ipc_msg = ipc_create_msg(fsx_ipc_struct, sizeof(struct fs_request), 0);
        fsx_fr_request = (struct fs_request*)ipc_get_msg_data(fsx_ipc_msg);
        fsx_fr_request->req = FS_REQ_OPEN;
        fsx_fr_request->open.new_fd = fd;
        strcpy(fsx_fr_request->open.pathname, fsm_fr_request->getfscap.pathname);
        ret = ipc_call(fsx_ipc_struct, fsx_ipc_msg);
        ipc_destroy_msg(fsx_ipc_struct, fsx_ipc_msg);

        fsx_ipc_msg = ipc_create_msg(fsx_ipc_struct, sizeof(struct fs_request), 0);
        fsx_fr_request = (struct fs_request*)ipc_get_msg_data(fsx_ipc_msg);
        fsx_fr_request->req = FS_REQ_WRITE;
        fsx_fr_request->write.fd = fd;
        fsx_fr_request->write.count = size;
        memcpy((void *)fsx_fr_request + sizeof(struct fs_request), buf, size);
        ret = ipc_call(fsx_ipc_struct, fsx_ipc_msg);
        ipc_destroy_msg(fsx_ipc_struct, fsx_ipc_msg);

        fsx_ipc_msg = ipc_create_msg(fsx_ipc_struct, sizeof(struct fs_request), 0);
        fsx_fr_request = (struct fs_request*)ipc_get_msg_data(fsx_ipc_msg);
        fsx_fr_request->req = FS_REQ_CLOSE;
        fsx_fr_request->close.fd = fd;
        ret = ipc_call(fsx_ipc_struct, fsx_ipc_msg);
        ipc_destroy_msg(fsx_ipc_struct, fsx_ipc_msg);

        /* LAB 5 TODO END */

        return ret;
}

/* Read content from the file at `path`. */
int fsm_read_file(const char* path, char* buf, unsigned long size) {

        if (!fsm_ipc_struct) {
                connect_fsm_server();
        }
        int ret = 0;

        /* LAB 5 TODO BEGIN */
        u64 cap;
        int fd;
        struct fs_cap_info_node *fs_cap_info_node;
        struct ipc_struct *fsx_ipc_struct;
        struct ipc_msg *fsm_ipc_msg, *fsx_ipc_msg;
        struct fs_request *fsm_fr_request, *fsx_fr_request;
        
        fsm_ipc_msg = ipc_create_msg(fsm_ipc_struct, sizeof(struct fs_request), 0);
        fsm_fr_request = (struct fs_request*)ipc_get_msg_data(fsm_ipc_msg);
        fsm_fr_request->req = FS_REQ_GET_FS_CAP;
        memcpy(fsm_fr_request->getfscap.pathname, path, size);
        ret = ipc_call(fsm_ipc_struct, fsm_ipc_msg);
        fsm_fr_request = (struct fs_request*)ipc_get_msg_data(fsm_ipc_msg);
        cap = ipc_get_msg_cap(fsm_ipc_msg, 0);
        ipc_destroy_msg(fsm_ipc_struct, fsm_ipc_msg);
        
        fs_cap_info_node = get_fs_cap_info(cap);
        fsx_ipc_struct = fs_cap_info_node->fs_ipc_struct;

        fd = alloc_fd();
        fsx_ipc_msg = ipc_create_msg(fsx_ipc_struct, sizeof(struct fs_request), 0);
        fsx_fr_request = (struct fs_request*)ipc_get_msg_data(fsx_ipc_msg);
        fsx_fr_request->req = FS_REQ_OPEN;
        fsx_fr_request->open.new_fd = fd;
        strcpy(fsx_fr_request->open.pathname, fsm_fr_request->getfscap.pathname);
        ret = ipc_call(fsx_ipc_struct, fsx_ipc_msg);
        ipc_destroy_msg(fsx_ipc_struct, fsx_ipc_msg);

        fsx_ipc_msg = ipc_create_msg(fsx_ipc_struct, sizeof(struct fs_request), 0);
        fsx_fr_request = (struct fs_request*)ipc_get_msg_data(fsx_ipc_msg);
        fsx_fr_request->req = FS_REQ_READ;
        fsx_fr_request->read.fd = fd;
        fsx_fr_request->read.count = size;
        ret = ipc_call(fsx_ipc_struct, fsx_ipc_msg);
        memcpy(buf, ipc_get_msg_data(fsx_ipc_msg), ret);
        ipc_destroy_msg(fsx_ipc_struct, fsx_ipc_msg);

        fsx_ipc_msg = ipc_create_msg(fsx_ipc_struct, sizeof(struct fs_request), 0);
        fsx_fr_request = (struct fs_request*)ipc_get_msg_data(fsx_ipc_msg);
        fsx_fr_request->req = FS_REQ_CLOSE;
        fsx_fr_request->close.fd = fd;
        ret = ipc_call(fsx_ipc_struct, fsx_ipc_msg);
        ipc_destroy_msg(fsx_ipc_struct, fsx_ipc_msg);

        /* LAB 5 TODO END */

        return ret;
}
```

