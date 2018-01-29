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

#include "xml.h"
#include "common.h"
#include "thread.h"
#include "share.h"
#include "usock.h"

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
find_queue_by_count(Uint32 count)
{
	DataQueue *queue = NULL;
	list_t *pos;

	list_for_each(pos, &queue_head) {
		queue = list_entry(pos, DataQueue, list);
		if (queue->count == count)
			return queue;
	}

	return NULL;
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

static int start_transfer_file(int sfd, const char *path, const char *name)
{
	int fd, n;
	char buf[MAX_BUFFER];

	printf("start transfer file ....\n");

	if (strcmp(name, login.name) == 0) {
		if ((fd = open(path, O_RDONLY)) < 0) {
			fprintf(stderr, "open \"%s\":%s\n", path, strerror(errno));
			return -1;
		}

		while ((n = read(fd, buf, 1000)) > 0) {
			if (writen(sfd, buf, n) != n) {
				perror("!!writen");
				return -1;
			}
		}
	}
	else  {
		fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
		if (fd < 0) {
			fprintf(stderr, "open file \"%s\":%s\n", path, strerror(errno));
			return -1;
		}

		while ((n = readn(sfd, buf, 1000)) > 0) {
			if (writen(fd, buf, n) != n) {
				perror("writen");
				close(fd);
				return -1;
			}
		}
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
		if ((n = usock_read(usock, buf, 1000)) <= 0)
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
	memcpy(queue->data, data, len);
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

	if (dh->type == DataTypeFile)
		start_transfer_file(sfd, queue->path, dh->cred.name);
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

static int accept_transfer_data(int sfd, char *num, char *path)
{
	Command comm;
	struct iovec mvec[2];
	DataInfoHead *dh;
	int count = atoi(num);
	DataQueue *queue;
	struct stat st;

	if (!(queue = find_queue_by_count(count)))
		return -1;

 	if (queue->status != QuePend)
		fprintf(stderr, "queue status error\n");

	queue->status = QueReady;

	dh = (DataInfoHead *)queue->data;
	if (path == NULL || path[0] != '/') {
		getcwd(queue->path, MAX_PATH);
		if (path) {
			strcat(queue->path, "/");
			strcat(queue->path, path);
		}
	}
	else
		strncpy(queue->path, path, MAX_PATH);

	if (stat(queue->path, &st) < 0) {
		if (errno != ENOENT) {
			perror("stat");
			queue_del(queue);
			free(queue);
			return -1;
		}
	}
	else if (S_ISDIR(st.st_mode)) {
		int n = strlen(queue->path);
		if (queue->path[n-1] != '/') {
			queue->path[n] = '/';
			queue->path[n+1] = 0;
		}
		strcat(queue->path, dh->file);
	}

	if (dh->type == DataTypeFileU || dh->type == DataTypeAudioU) {
		queue->sockfd = UDPSTART;
		pthread_mutex_unlock(&queue->lock);
		return 0;
	}

	strcpy(comm.cmd, ACCEPTTRANS);
	comm.len = sizeof(dh->cred);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = &dh->cred;
	mvec[1].iov_len  = sizeof(dh->cred);
	return writev(sfd, mvec, 2);
}

static int send_refuser_cmd(int sfd, Cred *cred)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, REFUSETRANS);
	comm.len = sizeof(Cred);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = cred;
	mvec[1].iov_len  = sizeof(Cred);
	return writev(sfd, mvec, 2);
}

static int send_cancel_cmd(int sfd, DataInfoHead *dh)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, CANCELTRANS);
	comm.len = sizeof(DataInfoHead);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = dh;
	mvec[1].iov_len  = sizeof(DataInfoHead);
	return writev(sfd, mvec, 2);
}

static int cancel_transfer_data(int sfd, char *num)
{
	int count = atoi(num);
	DataQueue *queue;
	DataInfoHead *dh;

	if (!(queue = find_queue_by_count(count)))
		return -1;

	dh = (DataInfoHead *)queue->data;
	if (queue->status == QuePend) {
		if (dh->type == DataTypeAudioU || dh->type == DataTypeFileU) {
			queue->sockfd = UDPSTOP;
			pthread_mutex_unlock(&queue->lock);
		} else {
			send_refuser_cmd(sfd, &dh->cred);
		}
	}
	else if (queue->status == QueReady) {
		DataInfoHead *dh = (DataInfoHead *)queue->data;
		send_cancel_cmd(sfd, dh);
	}
	else if (queue->status == QueStart) {
		DataInfoHead *dh = (DataInfoHead *)queue->data;
		send_cancel_cmd(sfd, dh);
		close(queue->sockfd);
		return 0;
	}

	queue_del(queue);
	free(queue);
	return 0;
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
	printf("%s online\n", (char *)data);
	return 0;
}

static int sm_notify_offline(void *data, int len)
{
	printf("%s offline\n", (char *)data);
	return 0;
}

static int sm_real_message(void *data, int len)
{
	RealMessage *cmsg;
	cmsg = (RealMessage *)data;
	printf("%s:%s\n", cmsg->name, cmsg->msg);
	return 0;
}

static int sm_request_add(void *data, int len)
{
	printf("request add:%s\n", (char *)data);
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
	printf("all friend: %s  online %s\n", (char *)data, is ? "yes" : "no");
	return 0;
}

static int sm_reply_search(void *data, int len)
{
	printf("search result: %s\n", (char *)data);
	return 0;
}

static int sm_accept_add(void *data, int len)
{
	printf("%s:recv accept add\n", (char *)data);
	return 0;
}

static int send_file(int sfd, const char *name, char *file, DataType type)
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

	if (stat(file, &st) < 0) {
		perror("file send");
		return -1;
	}

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

	queue_add(queue, QueReady);

	printf("id:[%d] send file \"%s\" to \"%s\" size:%d\n\n",
			queue->count, dh->file, dh->name, dh->size);

	strcpy(comm.cmd, REQTRANS);
	comm.len = sizeof(DataInfoHead);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = dh;
	mvec[1].iov_len  = sizeof(DataInfoHead);
	return writev(sfd, mvec, 2);
}

static int tcp_send_file(int sfd, char *name, char *file)
{
	return send_file(sfd, name, file, DataTypeFile);
}

static int udp_send_file(int sfd, char *name, char *file)
{
	return send_file(sfd, name, file, DataTypeFileU);
}

static int audio_request(int sfd, char *name, DataType type)
{
	Command comm;
	struct iovec mvec[2];
	DataInfoHead *dh;
	DataQueue *queue;

	if (strcmp(name, login.name) == 0)
		return -1;

	queue = (DataQueue *)malloc(sizeof(DataQueue) + sizeof(DataInfoHead));
	dh = (DataInfoHead *)(queue + 1);
	queue->data = dh;

	dh->type = type;
	strcpy(dh->name, name);
	memset(&dh->cred, 0, sizeof(dh->cred));
	strcpy(dh->cred.name, login.name);
	gettimeofday(&dh->cred.time, NULL);

	queue_add(queue, QueReady);

	printf("id:[%d] request audio to \"%s\"\n\n", queue->count, dh->name);

	strcpy(comm.cmd, REQTRANS);
	comm.len = sizeof(DataInfoHead);
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = dh;
	mvec[1].iov_len  = sizeof(DataInfoHead);
	return writev(sfd, mvec, 2);
}

static int tcp_audio_request(int sfd, char *name)
{
	return audio_request(sfd, name, DataTypeAudio);
}

static int udp_audio_request(int sfd, char *name)
{
	return audio_request(sfd, name, DataTypeAudioU);
}

static int send_realmsg(int sfd, char *name, char *msg)
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

static int send_add_friend(int sfd, char *name)
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

static int send_del_friend(int sfd, char *name)
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

static int  accept_add_friend(int sfd, char *name)
{
	Command comm;
	struct iovec mvec[2];

	strcpy(comm.cmd, ACCEPTADD);
	comm.len = MAX_USERNAME;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = MAX_USERNAME;
	writev(sfd, mvec, 2);

	return 0;
}

static int search_user(int sfd, char *name)
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

static int search_who(int sfd)
{
	Command comm;
	struct iovec mvec[1];

	strcpy(comm.cmd, SEARCHWHO);
	comm.len = 0;
	mvec[0].iov_base = &comm;
	mvec[0].iov_len  = sizeof(comm);
	writev(sfd, mvec, 1);

	return 0;
}

static int search_friend(int sfd)
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

static int args_ip(int argc, char * const argv[], struct sockaddr_in *seraddr)
{
	char ip[16], port[8];
	int ch, n;

	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch (ch) {
		case 's':
			if ((n = sscanf(optarg, "%15[0-9.]:%5[0-9]", ip, port)) < 1) {
				fprintf(stderr, "ip error\n");
				break;
			}

			seraddr->sin_family = AF_INET;
			inet_pton(AF_INET, ip, &seraddr->sin_addr);
			printf("%d\n", n);
			if (n == 2)
				seraddr->sin_port = htons(atoi(port));
			else
				seraddr->sin_port = htons(PORTCOM);

			return 0;
		}
	}
	return -1;
}

int main(int argc, char * const argv[])
{
	struct sockaddr_in seraddr;
	int sfd;
	int confirm = 0;
	char buf[1024];
	fd_set allfds;

	if (args_ip(argc, argv, &seraddr) < 0)
		return 1;

	memcpy(&trans_addr, &seraddr, sizeof(trans_addr));
	trans_addr.sin_port = htons(PORTFILE);
	memcpy(&trans_addrU, &seraddr, sizeof(trans_addrU));
	trans_addrU.sin_port = htons(PROTUDP);

	printf("username:");
	fflush(stdout);
	fgets(login.name, sizeof(login.name), stdin);
	int n = strlen(login.name);
	if (login.name[n-1] == '\n')
		login.name[n-1] = 0;
#if 1
	strcpy(login.passwd, getpass("passwd:"));
#else
	printf("passwd:");
	fflush(stdout);
	fgets(login.passwd, sizeof(login.passwd), stdin);
	if (login.passwd[n-1] == '\n')
		login.passwd[n-1] = 0;
#endif

	signal(SIGPIPE, SIG_IGN);
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		return 1;
    }

	if (connect(sfd, (Sockaddr *)&seraddr, sizeof(seraddr)) < 0) {
		fprintf(stderr, "socket connect failed\n");
		return 1;
	}

	if (writen(sfd, &login, sizeof(login)) != sizeof(login)) {
		perror("login fail write\n");
		return 1;
	}

	if (readn(sfd, &confirm, sizeof(confirm)) != sizeof(confirm)
			|| confirm != LOGIONSUCCESS) {
		perror("login fail read");
		return 1;
	}

	FD_ZERO(&allfds);
	FD_SET(0, &allfds);
	FD_SET(sfd, &allfds);
	while (1) {
		fd_set fds;

		fds = allfds;
		if (select(sfd+1, &fds, 0, 0, NULL) < 0) {
			perror("select");
			break;
		}

		if (FD_ISSET(0, &fds)) {
			int n;
			char comm[MAX_COMMAND];
#define MAX_ARG	200
			char arg1[MAX_ARG];
			char arg2[MAX_ARG];
			char arg3[MAX_ARG];
			fgets(buf, 200, stdin);
			if ((n = sscanf(buf, "%s %s %s %s", comm, arg1, arg2, arg3)) >= 1) {
				if (n > 2 && strcmp(comm, "send") == 0)
					send_realmsg(sfd, arg1, arg2);
				else if (n > 1 && strcmp(comm, "add") == 0)
					send_add_friend(sfd, arg1);
				else if (n > 1 && strcmp(comm, "del") == 0)
					send_del_friend(sfd, arg1);
				else if (n > 1 && strcmp(comm, "accept") == 0)
					accept_add_friend(sfd, arg1);
				else if (n > 1 && strcmp(comm, "search") == 0)
					search_user(sfd, arg1);
				else if (strcmp(comm, "who") == 0)
					search_who(sfd);
				else if (strcmp(comm, "friend") == 0)
					search_friend(sfd);
				else if (n > 2 && strcmp(comm, "file") == 0)
					tcp_send_file(sfd, arg1, arg2);
				else if (n > 2 && strcmp(comm, "ufile") == 0)
					udp_send_file(sfd, arg1, arg2);
				else if (n > 1 && strcmp(comm, "audio") == 0)
					tcp_audio_request(sfd, arg1);
				else if (n > 1 && strcmp(comm, "uaudio") == 0)
					udp_audio_request(sfd, arg1);
				else if (n > 1 && strcmp(comm, "cancel") == 0) {
					cancel_transfer_data(sfd, arg1);
				}
				else if (n > 1 && strcmp(comm, "recv") == 0) {
					if (n == 2)
						accept_transfer_data(sfd, arg1, NULL);
					else
						accept_transfer_data(sfd, arg1, arg2);
				}
			}
		}
		if (FD_ISSET(sfd, &fds)) {
			Command cmd;
			int n;
			if ((n = readn(sfd, &cmd, sizeof(cmd))) != sizeof(cmd)) {
				if (n == 0)
					printf("server close\n");
				else
					perror("server close");
				exit(1);
			}

			if (cmd.len > 0) {
				if ((n = readn(sfd, buf, cmd.len)) != cmd.len) {
					if (n == 0)
						printf("server close\n");
					else
						perror("server close");
					exit(1);
				}
			}
			for (n = 0; n < TABLESIZE(recv_cmd); n++) {
				if (strcmp(recv_cmd[n].cmd, cmd.cmd) == 0) {
					recv_cmd[n].handler(buf, cmd.len);
					break;
				}
			}
		}
	}

	return 0;
}

