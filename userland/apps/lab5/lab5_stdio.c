/*
 * Copyright (c) 2022 Institute of Parallel And Distributed Systems (IPADS)
 * ChCore-Lab is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#include "lab5_stdio.h"


extern struct ipc_struct *tmpfs_ipc_struct;

/* You could add new functions or include headers here.*/
/* LAB 5 TODO BEGIN */
#define READ 1
#define WRITE 2
#define IS_READ(flag) (((flag)&READ)==READ)
#define SET_READ(flag) ((flag)|READ)
#define IS_WRITE(flag) (((flag)&WRITE)==WRITE)
#define SET_WRITE(flag) ((flag)|WRITE)

int alloc_fd() {
	static int cnt = 0;
	return ++cnt;
}

int readfile(int fd, char *buf, int count)
{
	struct ipc_msg *ipc_msg = 0;
	struct fd_record_extension *fd_ext;
	struct fs_request *fr_ptr;
	int ret = 0, remain = count, cnt;

	ipc_msg = ipc_create_msg(tmpfs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
	while (remain > 0) {
		fr_ptr->req = FS_REQ_READ;
		fr_ptr->read.fd = fd;
		cnt = MIN(remain, PAGE_SIZE);
		fr_ptr->read.count = cnt;
		ret = ipc_call(tmpfs_ipc_struct, ipc_msg);
		if (ret < 0)
			goto error;
		memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
		buf += ret;
		remain -= ret;
		if (ret != cnt)
			break;
	}
	ret = count - remain;
error:
	ipc_destroy_msg(tmpfs_ipc_struct, ipc_msg);
	return ret;
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

bool is_separate(char c) {
    return c=='\t' || c=='\n' || c==' ' || c==0;
}

void int2str(int n, char *str)
{
    char buf[32];
    int i = 0, temp = n, len;

    if (str == NULL) {
        return;
    }
    while(temp) {
        buf[i++] = (char)(temp % 10) + '0';  //把temp的每一位上的数存入buf
        temp = temp / 10;
    }
    len = i;
    str[i] = 0;
    while(i > 0) {
        str[len - i] = buf[i - 1];
        i--;
    }
}

int str2int(const char *str)
{
    int temp = 0;
    while(*str != 0) {
        if (!is_digit(*str)) break;
        temp = temp * 10 + (*str - '0');
        str++;
    }
    return temp;
}

void fscanf_handle_int(int *int_ptr, int *idx_buf, const char *buf)
{
    char int_buf[32];
    int start = *idx_buf, end = 0, i = *idx_buf, j = 0;
    while(is_digit(buf[i])) {
        i++;
    }
    end = i;
    for(j=start;j < end;++j) {
        int_buf[j - start] = buf[j];
    }
    int_buf[j-start] = '\0';
    *idx_buf = end;
    *int_ptr = str2int(int_buf);
}

void fscanf_handle_str(char *str_ptr, int *idx_buf, const char *buf)
{
    int i = *idx_buf, start = *idx_buf;
    while(!is_separate(buf[i])) {
        str_ptr[i - start] = buf[i];
        i++;
    }
    str_ptr[i - start] = 0;
    *idx_buf = i;
}

void fprintf_handle_int(int val, int *idx_buf, char *buf)
{
    char int_buf[32];
    int2str(val, int_buf);
    int i = 0, start = *idx_buf;
    while(int_buf[i]!=0) {
        buf[i + start] = int_buf[i];
        ++i;
    }
    buf[i + start] = 0;
    *idx_buf = i + start;
}

void fprintf_handle_str(char *str, int *idx_buf, char *buf)
{
    int i = 0, start = *idx_buf;
    int len = strlen(str);
    while(i < len) {
        buf[i + start] = str[i];
        ++i;
    }
    buf[i + start] = 0;
    *idx_buf = i + start;
}

/* LAB 5 TODO END */


FILE *fopen(const char * filename, const char * mode) {

	/* LAB 5 TODO BEGIN */
	struct FILE* file = calloc(sizeof(struct FILE), 1);
	struct ipc_msg *ipc_msg;
	struct fs_request *fr;

	if(strlen(mode)==0) 
		return NULL;

	if(mode[0]=='w'){
		file->w = true;
		ipc_msg = ipc_create_msg(tmpfs_ipc_struct, sizeof(struct fs_request), 0);
		fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
		fr->req = FS_REQ_CREAT;
		strcpy(fr->creat.pathname, filename);
		ipc_call(tmpfs_ipc_struct, ipc_msg);
		ipc_destroy_msg(tmpfs_ipc_struct, ipc_msg);
	} else {
		file->w = false;
	}

	file->fd = alloc_fd();
	ipc_msg = ipc_create_msg(tmpfs_ipc_struct, sizeof(struct fs_request), 0);
	fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_OPEN;
	fr->open.new_fd = file->fd;
	strcpy(fr->open.pathname, filename);
	ipc_call(tmpfs_ipc_struct, ipc_msg);
	ipc_destroy_msg(tmpfs_ipc_struct, ipc_msg);

	return file;

	/* LAB 5 TODO END */
    return NULL;
}

size_t fwrite(const void * src, size_t size, size_t nmemb, FILE * f) {

	/* LAB 5 TODO BEGIN */
	if(!f->w) return 0;
	size_t bytes = size * nmemb;
	struct ipc_msg *ipc_msg = ipc_create_msg(tmpfs_ipc_struct, sizeof(struct fs_request), 0);
	struct fs_request *fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_WRITE;
	fr->write.fd = f->fd;
	fr->write.count = bytes;
	memcpy((void *)fr + sizeof(struct fs_request), src, bytes);
	int ret = ipc_call(tmpfs_ipc_struct, ipc_msg);
	ipc_destroy_msg(tmpfs_ipc_struct, ipc_msg);
	return ret;

	/* LAB 5 TODO END */
    return 0;

}

size_t fread(void * destv, size_t size, size_t nmemb, FILE * f) {

	/* LAB 5 TODO BEGIN */
	if(f->w) return 0;
	size_t bytes = size * nmemb;
	size_t ret = readfile(f->fd, destv, bytes);
	return ret;

	/* LAB 5 TODO END */
    return 0;

}

int fclose(FILE *f) {

	/* LAB 5 TODO BEGIN */
	struct ipc_msg *ipc_msg = ipc_create_msg(tmpfs_ipc_struct, sizeof(struct fs_request), 0);
	struct fs_request *fr = (struct fs_request*) ipc_get_msg_data(ipc_msg);
	fr->req = FS_REQ_CLOSE;
	fr->close.fd = f->fd;
	int ret = ipc_call(tmpfs_ipc_struct, ipc_msg);
	ipc_destroy_msg(tmpfs_ipc_struct, ipc_msg);

	/* LAB 5 TODO END */
    return 0;

}

/* Need to support %s and %d. */
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

/* Need to support %s and %d. */
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

