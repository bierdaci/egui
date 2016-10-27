#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <egui/egui.h>

#include "common.h"
#include "share.h"
#include "usock.h"

#include "msg.h"

typedef struct _DataQueue DataQueue;
#define		QueueRefuse		(0x1 <<0)

#define UDPSTOP		0
#define UDPSTART	1
typedef enum {QuePend, QueReady, QueStart} QueStatus;

struct _DataQueue {
	QueStatus status;
	int count;
	int sockfd;
	void *data;
	pthread_t pid;
	pthread_mutex_t lock;
	char path[MAX_PATH];
	list_t list;
};

static LIST_HEAD(queue_head);
static LoginInfo login;
static int pending_count = 0;
static struct sockaddr_in trans_addr;
static struct sockaddr_in trans_addrU;

static int sm_real_message(void *data, int len);
static int sm_request_add(void *data, int len);
static int sm_refuse_add(void *data, int len);
static int sm_accept_add(void *data, int len);
static int sm_reply_search(void *data, int len);
static int sm_reply_who(void *data, int len);
static int sm_reply_friend(void *data, int len);
static int sm_notify_online(void *data, int len);
static int sm_notify_offline(void *data, int len);
static int sm_del_friend(void *data, int len);
static int sm_request_transfer(void *data, int len);
static int sm_start_transfer(void *data, int len);
static int sm_refuser_transfer(void *data, int len);
static int sm_cancel_transfer(void *data, int len);
static int sm_uready_transfer(void *data, int len);

static struct {
	const char cmd[MAX_COMMAND];
	int (*handler)(void *data, int len);
} recv_cmd[] = {
	{ REALMESSAGE,	sm_real_message		},
	{ REQUESTADD,	sm_request_add		},
	{ REFUSEADD,	sm_refuse_add		},
	{ ACCEPTADD,	sm_accept_add		},
	{ REPLYWHO,		sm_reply_who		},
	{ REPLYFRIEND,	sm_reply_friend		},
	{ REPLYSEARCH,	sm_reply_search		},
	{ NOTIFYON,		sm_notify_online	},
	{ NOTIFYOFF,	sm_notify_offline	},
	{ DELFRIEND,	sm_del_friend		},
	{ REQTRANS, 	sm_request_transfer },
	{ STARTTRANS,	sm_start_transfer	},
	{ REFUSETRANS,	sm_refuser_transfer },
	{ CANCELTRANS,	sm_cancel_transfer	},
	{ UREADYTRANS,	sm_uready_transfer	},
};

static void queue_del(DataQueue *node)
{
	list_del(&node->list);
}

static void queue_add(DataQueue *node, QueStatus status)
{
	node->status = status;
	node->count = pending_count++;
	list_add(&node->list, &queue_head);
}

static DataQueue *
find_queue_by_data(void *data, int len, ebool (*cmp)(void *, void *, int))
{
	DataQueue *node = NULL;
	list_t *pos;

	list_for_each(pos, &queue_head) {
		node = list_entry(pos, DataQueue, list);
		if (cmp(node->data, data, len))
			return node;
	}

	return NULL;
}

static ebool cred_cmp(void *data1, void *data2, int len)
{
	return(memcmp(data1, data2, len) == 0);
}

static ebool _cred_cmp(void *data1, void *data2, int len)
{
	DataInfoHead *dh = (DataInfoHead *)data1;
	return cred_cmp(&dh->cred, data2, len);
}

static DataQueue *find_queue_by_cred(Cred *cred)
{
	return find_queue_by_data(cred, sizeof(Cred), _cred_cmp);
}

static int start_audio(int sfd)
{
	fd_set allfds;
	int fd;
	char buf[MAX_BUFFER];

	printf("start audio ....\n");
	if ((fd = open("/dev/dsp", O_RDWR)) < 0) {
		fprintf(stderr, "open \"/dev/dsp\":%s\n", strerror(errno));
		return -1;
	}

	fcntl(sfd, F_SETFL, O_NONBLOCK);
	FD_ZERO(&allfds);
	FD_SET(sfd, &allfds);
	FD_SET(fd, &allfds);
	while (1) {
		fd_set fds = allfds;
		if (select(50, &fds, 0, NULL, NULL) < 0) {
			perror("select");
			break;
		}
		if (FD_ISSET(fd, &fds)) {
			int n = read(fd, buf, sizeof(buf));
			if (n <= 0) {
				return -1;
			}
			if (write(sfd, buf, n) < 0 && errno != EAGAIN) {
				close(fd);
				return -1;
			}
		}
		if (FD_ISSET(sfd, &fds)) {
			int n = read(sfd, buf, sizeof(buf));
			if (n <= 0) {
				close(fd);
				return -1;
			}
			if ((writen(fd, buf, n) != n))
				return -1;
		}
	}

	close(fd);
	return 0;
}

static int start_recv_file(int sfd, const char *path, Cred *cred)
{
	int  fd, n;
	int  completed = 0;
	char buf[MAX_BUFFER];

	printf("start recv file ....\n");

	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "open file \"%s\":%s\n", path, strerror(errno));
		return -1;
	}

	while ((n = readn(sfd, buf, MAX_BUFFER)) > 0) {
		if (writen(fd, buf, n) != n) {
			perror("writen");
			close(fd);
			return -1;
		}
		completed += n;
		ui_set_transfer_size(cred, completed);
	}

	close(fd);
	return 0;
}

static int start_send_file(int sfd, const char *path, Cred *cred)
{
	int  fd, n;
	int  completed = 0;
	char buf[MAX_BUFFER];

	printf("start send file ....\n");

	if ((fd = open(path, O_RDONLY)) < 0) {
		fprintf(stderr, "open \"%s\":%s\n", path, strerror(errno));
		return -1;
	}

	while ((n = read(fd, buf, 1000)) > 0) {
		if (writen(sfd, buf, n) != n) {
			perror("!!writen");
			return -1;
		}
		completed += n;
		ui_set_transfer_size(cred, completed);
	}

	close(fd);
	return 0;
}

static int usock_send_file(USocket *usock, const char *path)
{
	int fd, n;
	char buf[MAX_BUFFER];

	printf("start send file ....\n");

	if ((fd = open(path, O_RDONLY)) < 0) {
		fprintf(stderr, "open \"%s\":%s\n", path, strerror(errno));
		return -1;
	}

	while ((n = read(fd, buf, 500)) > 0) {
		if (usock_write(usock, buf, n) != n) {
			perror("!!writen");
			return -1;
		}
	}

	printf("send file finish\n");
	close(fd);
	return 0;
}

Uint32 get_usec(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 * 1000 + tp.tv_usec;
}

static int usock_recv_file(USocket *usock, const char *path)
{
	int fd, n;
	char buf[MAX_BUFFER];

	printf("start recv file ....\n");

	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "open file \"%s\":%s\n", path, strerror(errno));
		return -1;
	}

	while (1) {
		if ((n = usock_read(usock, buf, MAX_BUFFER)) <= 0)
			break;

		if (writen(fd, buf, n) != n) {
			perror("writen");
			close(fd);
			return -1;
		}
	}
	printf("recv file finish\n");
	return 0;
}

static void *udp_ready_handler(void *data)
{
	DataQueue *queue = (DataQueue *)data;
	DataInfoHead *dh = (DataInfoHead *)queue->data;
	int clen = sizeof(dh->cred);
	int sfd;
	USocket *usock;

    if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		goto err1;
    }

	sendto(sfd, &dh->cred, sizeof(dh->cred), 0,
			(Sockaddr *)&trans_addrU, sizeof(trans_addrU));

	if ((usock = usock_accept(sfd, &dh->cred, clen))) {
		if (dh->type == DataTypeFileU)
			usock_send_file(usock, queue->path);
		else if (dh->type == DataTypeAudioU)
			;
	}

	usock_close(usock);
err1:
	queue_del(queue);
	free(queue);
	return NULL;
}

static int sm_uready_transfer(void *data, int len)
{
	DataQueue *queue;

	if ((queue = find_queue_by_cred(data)) && queue->status == QueReady) {
		if (pthread_create(&queue->pid, NULL, udp_ready_handler, queue) < 0) {
			queue_del(queue);
			free(queue);
			return -1;
		}
	}

	return 0;
}

static void *udp_recv_handler(void *data)
{
	DataQueue *queue = (DataQueue *)data;
	DataInfoHead *dh;
	struct sockaddr_in addr;
	char ip[16], port[8];
	int sfd;
	USocket *usock;

	pthread_mutex_lock(&queue->lock);

	if (queue->sockfd == UDPSTOP)
		goto err1;

	queue->status = QueReady;
    if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		goto err1;
    }

	dh = (DataInfoHead *)queue->data;
	sscanf(dh->ipstr, "%15[0-9.]:%5[0-9]", ip, port);
	addr.sin_port = htons(atoi(port));
	inet_pton(AF_INET, ip, &addr.sin_addr);
	addr.sin_family = AF_INET;

	usock = usock_connect(sfd, &addr, &dh->cred, sizeof(dh->cred));
	if (usock) {
		if (dh->type == DataTypeFileU)
			usock_recv_file(usock, queue->path);
		else if (dh->type == DataTypeAudioU)
			;//udp_start_audio(usock);
	}

	usock_close(usock);
err1:
	queue_del(queue);
	free(queue);
	return NULL;
}

static int sm_request_transfer(void *data, int len)
{
	DataQueue *queue;
	DataInfoHead *dh = (DataInfoHead *)data;

	queue = (DataQueue *)malloc(sizeof(DataQueue) + len);
	queue->data = queue + 1;
	memcpy(queue->data, dh, len);
	queue_add(queue, QuePend);

	if (dh->type == DataTypeFile || dh->type == DataTypeFileU) {
		printf("id:[%d]  \"%s\" send file:\"%s\"  size:%d\n\n",
				queue->count, dh->name, dh->file, dh->size);
	}
	else if (dh->type == DataTypeAudio || dh->type == DataTypeAudioU) {
		printf("id:[%d] \"%s\" request audio\n\n", queue->count, dh->name);
	}

	if (dh->type == DataTypeFileU || dh->type == DataTypeAudioU) {
		pthread_mutex_init(&queue->lock, NULL);
		pthread_mutex_lock(&queue->lock);
		if (pthread_create(&queue->pid, NULL, udp_recv_handler, queue) < 0) {
			queue_del(queue);
			free(queue);
			return -1;
		}
	}

	if (dh->type == DataTypeFile)
		ui_request_file_notify(dh);

	return 0;
}

static int sm_refuser_transfer(void *data, int len)
{
	DataQueue *queue;

	if (!(queue = find_queue_by_cred((Cred *)data)))
		return -1;

	DataInfoHead *dh = (DataInfoHead *)queue->data;
	if (queue->status == QuePend)
		fprintf(stderr, "%s: refuser recv file:%s\n", dh->name, dh->file);
	else if (queue->status == QueReady)
		fprintf(stderr, "%s: refuser recv file:%s\n", dh->name, dh->file);
	else if (queue->status == QueStart)
		fprintf(stderr, "%s: refuser recv file:%s\n", dh->name, dh->file);

	queue_del(queue);
	free(queue);
	return 0;
}

static int sm_cancel_transfer(void *data, int len)
{
	DataQueue *queue;
	DataInfoHead *dh;

	if (!(queue = find_queue_by_cred((Cred *)data)))
		return -1;

	dh = (DataInfoHead *)queue->data;

	if (queue->status == QuePend)
		fprintf(stderr, "%s: cancel send file:%s\n", dh->name, dh->file);
	else if (queue->status == QueReady)
		fprintf(stderr, "%s: cancel send file:%s\n", dh->name, dh->file);
	else if (queue->status == QueStart)
		fprintf(stderr, "%s: cancel send file:%s\n", dh->name, dh->file);

	queue_del(queue);
	free(queue);
	return 0;
}

static int start_tcp_transfer(DataQueue *queue, DataInfoHead *dh)
{
	int sfd;
	struct iovec mvec[2];
	int clen = sizeof(dh->cred);

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		goto err;
    }

	if (connect(sfd, (Sockaddr *)&trans_addr, sizeof(trans_addr)) < 0) {
		fprintf(stderr, "socket connect failed\n");
		goto err;
	}

	mvec[0].iov_base = &clen;
	mvec[0].iov_len  = sizeof(clen);
	mvec[1].iov_base = &dh->cred;
	mvec[1].iov_len  = sizeof(dh->cred);
	if (writev(sfd, mvec, 2) < 0) {
		perror("writev");
		goto err;
	}

	queue->sockfd = sfd;

	if (dh->type == DataTypeFile) {
		if (strcmp(dh->cred.name, login.name) == 0)
			start_send_file(sfd, queue->path, &dh->cred);
		else
			start_recv_file(sfd, queue->path, &dh->cred);
	}
	else if (dh->type == DataTypeAudio)
		start_audio(sfd);

err:
	if (close(sfd) < 0)
		perror("close");
	else
		fprintf(stderr, "transfer finish\n");

	return 0;
}

static void *tcp_transfer_handler(void *data)
{
	DataQueue *queue = (DataQueue *)data;
	DataInfoHead *dh = (DataInfoHead *)queue->data;

	queue->status = QueStart;

	start_tcp_transfer(queue, dh);

	queue_del(queue);
	free(queue);
	return NULL;
}

int client_accept_transfer(int sfd, DataInfoHead *dh, const char *path)
{
	DataQueue *queue = find_queue_by_cred(&dh->cred);
	Command comm;
	struct iovec mvec[2];

 	if (queue->status != QuePend)
		fprintf(stderr, "queue status error\n");

	queue->status = QueReady;
	strcpy(queue->path, path);
	strcat(queue->path, dh->file);

	strcpy(comm.cmd, ACCEPTTRANS);
	comm.len = sizeof(dh->cred);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = &dh->cred;
	mvec[1].iov_len  = sizeof(dh->cred);
	return writev(sfd, mvec, 2);
}

static int sm_start_transfer(void *data, int len)
{
	DataQueue *queue;

	if ((queue = find_queue_by_cred(data)) && (queue->status = QueReady)) {
		if (pthread_create(&queue->pid, NULL, tcp_transfer_handler, queue) < 0) {
			queue_del(queue);
			free(queue);
			return -1;
		}
	}
	return 0;
}

static int sm_del_friend(void *data, int len)
{
	printf("del user:%s\n", (char *)data);
	return 0;
}

static int sm_notify_online(void *data, int len)
{
	ui_change_online((char *)data, etrue);
	return 0;
}

static int sm_notify_offline(void *data, int len)
{
	printf("offline %s\n", (char *)data);
	ui_change_online((char *)data, efalse);
	return 0;
}

static int sm_real_message(void *data, int len)
{
	RealMessage *cmsg;
	cmsg = (RealMessage *)data;
	ui_recv_realmsg(cmsg->name, cmsg->msg);
	return 0;
}

static int sm_request_add(void *data, int len)
{
	printf("request add:%s\n", (char *)data);
	ui_accept_add_dlg((char *)data);
	return 0;
}

static int sm_reply_who(void *data, int len)
{
	printf("online friend: %s\n", (char *)data);
	return 0;
}

static int sm_reply_friend(void *data, int len)
{
	ebool is = *(ebool *)(data + MAX_USERNAME);
	printf("all friend: %s  online \"%s\" \n", (char *)data, is ? "yes" : "no");
	ui_friend_to_clist(data, is, efalse);
	return 0;
}

static int sm_reply_search(void *data, int len)
{
	if (len > 0) {
		ebool is = *(ebool *)(data + MAX_USERNAME);
		printf("search result: %s  online %d\n", (char *)data, is);
		ui_add_user((const char *)data, is);
	}
	else {
		printf("search result: unknown\n");
		ui_add_user(NULL, efalse);
	}

	return 0;
}

static int sm_refuse_add(void *data, int len)
{
	printf("%s refuse add\n", (char *)data);
	return 0;
}

static int sm_accept_add(void *data, int len)
{
	printf("%s accept add\n", (char *)data);
	ui_notify_add_dlg((char *)data);
	return 0;
}

static int send_file(int sfd, const char *name, char *file, DataType type, int *size, Cred *cred)
{
	Command comm;
	struct iovec mvec[2];
	DataInfoHead *dh;
	DataQueue *queue;
	struct stat st;
	char *p;
	int len;

	if (strcmp(name, login.name) == 0)
		return -1;

	len = strlen(file);
	if (file[len-1] == '\n')
		file[len-1] = 0;
	if ((p = strrchr(file, '/'))) ++p; else p = file;
	if ((len = strlen(p) + 1) > MAX_FILENAME) {
		fprintf(stderr, "file name too long\n");
		return -1; 
	}

	if (stat(file, &st) < 0 || S_ISDIR(st.st_mode)) {
		perror("file send");
		return -1;
	}
	if (size) *size = st.st_size;

	queue = (DataQueue *)malloc(sizeof(DataQueue) + sizeof(DataInfoHead));
	strncpy(queue->path, file, MAX_PATH);
	dh = (DataInfoHead *)(queue + 1);
	queue->data = dh;

	dh->type = type;
	dh->size = st.st_size;
	strcpy(dh->name, name);
	strcpy(dh->file, p);
	memset(&dh->cred, 0, sizeof(dh->cred));
	strcpy(dh->cred.name, login.name);
	gettimeofday(&dh->cred.time, NULL);
	memcpy(cred, &dh->cred, sizeof(Cred));

	queue_add(queue, QueReady);

	printf("id:[%d] send file \"%s\" to \"%s\" size:%d\n\n",
			queue->count, dh->file, dh->name, dh->size);

	strcpy(comm.cmd, REQTRANS);
	comm.len = sizeof(DataInfoHead);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = dh;
	mvec[1].iov_len  = sizeof(DataInfoHead);
	writev(sfd, mvec, 2);
	return queue->count;
}

int client_send_file(int sfd, char *name, char *file, int *size, void *cred)
{
	return send_file(sfd, name, file, DataTypeFile, size, (Cred *)cred);
}

int client_send_realmsg(int sfd, char *name, char *msg)
{
	Command comm;
	struct iovec mvec[3];
	int mlen = strlen(msg) + 1;

	strcpy(comm.cmd, REALMESSAGE);
	comm.len = mlen + MAX_USERNAME;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = MAX_USERNAME;
	mvec[2].iov_base = msg;
	mvec[2].iov_len  = mlen;
	writev(sfd, mvec, 3);

	return 0;
}

int client_add_friend(int sfd, char *name)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, REQUESTADD);
	comm.len = MAX_USERNAME;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = MAX_USERNAME;
	writev(sfd, mvec, 2);

	return 0;
}

int client_send_del_friend(int sfd, char *name)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, DELFRIEND);
	comm.len = MAX_USERNAME;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = MAX_USERNAME;
	writev(sfd, mvec, 2);

	return 0;
}

int  client_accept_add_friend(int sfd, char *name, const char *cmd)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, cmd);
	comm.len = MAX_USERNAME;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = MAX_USERNAME;
	writev(sfd, mvec, 2);

	return 0;
}

int client_search_user(int sfd, char *name)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, SEARCHUSER);
	comm.len = MAX_USERNAME;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = MAX_USERNAME;
	writev(sfd, mvec, 2);

	return 0;
}

int client_search_friend(int sfd)
{
	Command comm;
	struct iovec mvec[1];

	strcpy(comm.cmd, SEARCHFRIEND);
	comm.len = 0;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	writev(sfd, mvec, 1);

	return 0;
}

static void *read_sock_cmd(void *data)
{
	char buf[1024];
	Command cmd;
	int sfd = (eint)data;

	while (readn(sfd, &cmd, sizeof(cmd)) == sizeof(cmd)) {
		int i;

		if (cmd.len > 0 && readn(sfd, buf, cmd.len) != cmd.len)
			break;

		for (i = 0; i < TABLESIZE(recv_cmd); i++) {
			if (strcmp(recv_cmd[i].cmd, cmd.cmd) == 0) {
				recv_cmd[i].handler(buf, cmd.len);
				break;
			}
		}
	}

	printf("server close\n");

	return NULL;
}

static int config_read_ip(char *ip)
{
	int  fd;
	char buf[MAX_BUFFER];

	if ((fd = open("msg.conf", O_RDONLY)) < 0)
		return -1;

	if (read(fd, buf, MAX_BUFFER) < 0) {
		close(fd);
		return -1;
	}

	if (sscanf(buf, "%15[0-9.]", ip) < 1) {
		fprintf(stderr, "ip error\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int client_login(const char *name, const char *passwd)
{
	struct sockaddr_in seraddr;
	int sfd;
	int confirm = 0;
	char ip[20];

	seraddr.sin_family = AF_INET;
	if (config_read_ip(ip) < 0)
		inet_pton(AF_INET, "127.0.0.1", &seraddr.sin_addr);
	else
		inet_pton(AF_INET, ip, &seraddr.sin_addr);
	seraddr.sin_port = htons(PORTCOM);

	memcpy(&trans_addr, &seraddr, sizeof(trans_addr));
	trans_addr.sin_port = htons(PORTFILE);
	memcpy(&trans_addrU, &seraddr, sizeof(trans_addrU));
	trans_addrU.sin_port = htons(PROTUDP);

	signal(SIGPIPE, SIG_IGN);
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		return -1;
    }

	if (connect(sfd, (Sockaddr *)&seraddr, sizeof(seraddr)) < 0) {
		fprintf(stderr, "socket connect failed\n");
		return -1;
	}

	strcpy(login.name, name);
	strcpy(login.passwd, passwd);

	if (writen(sfd, &login, sizeof(login)) != sizeof(login)) {
		perror("login fail write\n");
		return -1;
	}

	if (readn(sfd, &confirm, sizeof(confirm)) != sizeof(confirm)
			|| confirm != LOGIONSUCCESS) {
		perror("login fail read");
		return -1;
	}

	return sfd;
}

int client_read_thread(int sfd)
{
	pthread_t pid;
	if (pthread_create(&pid, NULL, read_sock_cmd, (void *)sfd) < 0)
		return -1;
	return 0;
}

