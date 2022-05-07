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

#include <stdio.h>
#include <chcore/types.h>
#include <chcore/fsm.h>
#include <chcore/tmpfs.h>
#include <chcore/ipc.h>
#include <chcore/internal/raw_syscall.h>
#include <chcore/internal/server_caps.h>
#include <chcore/procm.h>
#include <chcore/fs/defs.h>
#include "mount_info.h"
#include "fsm.h"

extern struct spinlock fsmlock;


extern struct list_head fsm_mount_info_mapping;

/* Mapping a pair of client_badge and fd to a mount_point_info_node struct*/
void fsm_set_mount_info_withfd(u64 client_badge, int fd, 
						struct mount_point_info_node* mount_point_info) {

	struct client_fd_info_node *private_iter;
	for_each_in_list(private_iter, struct client_fd_info_node, node, &fsm_mount_info_mapping) {
		if (private_iter->client_badge == client_badge) {
			private_iter->mount_point_info[fd] = mount_point_info;
			return;
		}
	}
	struct client_fd_info_node *n = (struct client_fd_info_node *)malloc(sizeof(struct client_fd_info_node));
	n->client_badge = client_badge;
	int i;
	for (i = 0; i < MAX_SERVER_ENTRY_PER_CLIENT; i++)
		n->mount_point_info[i] = NULL;

	n->mount_point_info[fd] = mount_point_info;
	/* Insert node to fsm_server_entry_mapping */
	list_append(&n->node, &fsm_mount_info_mapping);
}


/* Get a mount_point_info_node struct with a pair of client_badge and fd*/
struct mount_point_info_node* fsm_get_mount_info_withfd(u64 client_badge, int fd) {
	struct client_fd_info_node *n;
	for_each_in_list(n, struct client_fd_info_node, node, &fsm_mount_info_mapping)
		if (n->client_badge == client_badge)
			return n->mount_point_info[fd];
	return NULL;
}

void strip_path(struct mount_point_info_node *mpinfo, char* path) {
	if(strcmp(mpinfo->path, "/")) {
		char* s = path;
		int i, len_after_strip;
		len_after_strip = strlen(path) - mpinfo->path_len;
		if(len_after_strip == 0) {
			path[0] = '/';
			path[1] = '\0';
		} else {
			for(i = 0; i < len_after_strip; ++i) {
				path[i] = path[i + mpinfo->path_len];
			}
			path[i] = '\0';
		}
	}
}

/* You could add new functions here as you want. */
/* LAB 5 TODO BEGIN */
int transfer_ipc_with_fd(int fd, struct ipc_msg *ipc_msg, struct fs_request *fr, u64 client_badge)
{
	int ret;
	struct ipc_msg *ipc_msg_transfer;
	struct fs_request *fr_transfer;
	struct mount_point_info_node *mpinfo = fsm_get_mount_info_withfd(client_badge, fd);
	ipc_msg_transfer = ipc_create_msg(mpinfo->_fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_transfer = (struct fs_request*)ipc_get_msg_data(ipc_msg_transfer);
	memcpy(fr_transfer, fr, sizeof(struct fs_request));
	ret = ipc_call(mpinfo->_fs_ipc_struct, ipc_msg_transfer);
	memcpy(ipc_get_msg_data(ipc_msg), ipc_get_msg_data(ipc_msg_transfer), ipc_msg_transfer->data_len);
	ipc_destroy_msg(mpinfo->_fs_ipc_struct, ipc_msg_transfer);
	return ret;
}

int transfer_ipc_with_pathname(const char *path, struct ipc_msg *ipc_msg, struct fs_request *fr)
{
	int ret;
	struct ipc_msg *ipc_msg_transfer;
	struct fs_request *fr_transfer;
	struct mount_point_info_node *mpinfo = get_mount_point(path, strlen(path));
	strip_path(mpinfo, path);
	ipc_msg_transfer = ipc_create_msg(mpinfo->_fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_transfer = (struct fs_request*)ipc_get_msg_data(ipc_msg_transfer);
	memcpy(fr_transfer, fr, sizeof(struct fs_request));
	ret = ipc_call(mpinfo->_fs_ipc_struct, ipc_msg_transfer);
	memcpy(ipc_get_msg_data(ipc_msg), ipc_get_msg_data(ipc_msg_transfer), ipc_msg_transfer->data_len);
	ipc_destroy_msg(mpinfo->_fs_ipc_struct, ipc_msg_transfer);
	return ret;
}

/* LAB 5 TODO END */


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


int main(int argc, char *argv[])
{

	init_fsm();

	ipc_register_server(fsm_server_dispatch);

	while (1) {
		__chcore_sys_yield();
	}
	return 0;
}
