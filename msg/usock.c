#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "usock.h"

#define UMSG_CONNECT		0x1
#define UMSG_ACCEPT			0x2
#define UMSG_REPLY_ACCEPT 	0x3
#define UMSG_CLOSE			0x4
#define UMSG_REPLY_CLOSE 	0x5
#define UMSG_BUSY 			0x6
#define UMSG_CONT 			0x7
#define UMSG_REPLY_CONT 	0x8
#define UMSG_PACK 			0x9
#define UMSG_PACK_COUNT 	0xA

#define USOCK_CONNECT		(0x1 << 0)
#define USOCK_CLOSE			(0x1 << 1)
#define USOCK_READY_CLOSE	(0x1 << 2)
#define	USOCK_PEER_BUSY		(0x1 << 3)

typedef struct {
	Uint32 mcount;
	Uint32 mtype;
	Uint32 mlen;
} umsg_t;

static void maketimeout(struct timespec *tsp, Uint32 sec)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	tsp->tv_sec = now.tv_sec;
	tsp->tv_nsec = now.tv_usec * 1000;
	tsp->tv_sec += sec;
}

static void *usock_read_handler(void *data);
static USocket *
create_usock(int fd, struct sockaddr_in *addr, void *cred_p, int clen)
{
	USocket *usock;

	if (connect(fd, (Sockaddr *)addr, sizeof(*addr)) < 0)
		return NULL;

	usock = (USocket *)malloc(sizeof(USocket));
	if (!usock)
		return NULL;

	usock->fd = fd;
	usock->status = 0;
	usock->clen = clen;
	usock->tsize = 0;
	memcpy(usock->credv, cred_p, clen);

	pthread_mutex_init(&usock->rlock, NULL);
	pthread_mutex_init(&usock->wlock, NULL);
	pthread_cond_init(&usock->rcond, NULL);
	pthread_cond_init(&usock->wcond, NULL);
	usock->r_pack_count = 0;
	usock->w_pack_count = 1;
	usock->rbuf = buffer_create(MAX_UBUF);
	sem_init(&usock->sem, 0, 0);
	pthread_create(&usock->pid, NULL, usock_read_handler, usock);

	return usock;
}

#define udp_timeout()					\
	do {								\
		int newtime = time(NULL);		\
		count += (newtime - oldtime);	\
		if (count >= timeout) {			\
			count = 0;					\
			timeout *= 2;				\
		}								\
		oldtime = newtime;				\
	} while (0)

static int send_umsg(USocket *usock, int mtype, const void *data, int len)
{
	int n = 0;
	umsg_t umsg;

	umsg.mtype = htonl(mtype);
	umsg.mlen = htonl(len);
	umsg.mcount = htonl(usock->w_pack_count);

	if (write(usock->fd, &umsg, sizeof(umsg)) < 0)
		return -1;

	if (len > 0)
		n = write(usock->fd, data, len);

	return n;
}

static int recv_umsg(USocket *usock, umsg_t *umsg, void *data, int len)
{
	int n = 0;

	if (read(usock->fd, umsg, sizeof(*umsg)) < sizeof(*umsg))
		return -1;

	umsg->mlen   = ntohl(umsg->mlen);
	umsg->mtype  = ntohl(umsg->mtype);
	umsg->mcount = ntohl(umsg->mcount);

	if (umsg->mlen > 0) {
		if (!data || umsg->mlen > len) {
			fprintf(stderr, "recv buffer not enough.\n");
			return -1;
		}
		n = readn(usock->fd, data, umsg->mlen);
		if (n < umsg->mlen)
			return -1;
	}

	return n;
}

static int
send_connect_msg(int sfd, void *data, int len, Uint32 *reply)
{
	Uint32 timeout = 2;
	Uint32 count = 0;
	Uint32 oldtime = 0;
	fd_set fds;
	int retval = -1;

	oldtime = time(NULL);
	FD_ZERO(&fds);
	FD_SET(sfd, &fds);
	while (timeout < 32) {
		struct timeval t = {1, 0};
		int n;

		if (count == 0 && write(sfd, data, len) != len)
			break;
		if ((n = select(sfd + 1, &fds, 0, 0, &t)) < 0) {
			perror("select");
			break;
		}
		else if (n == 0) {
			udp_timeout();
			FD_SET(sfd, &fds);
			continue;
		}
		if (FD_ISSET(sfd, &fds)) {
			Uint32 type;
			retval = read(sfd, &type, sizeof(type));
			*reply = ntohl(type);
			break;
		}

		udp_timeout();
	};

	return retval;
}

int send_accept_msg(int sfd, Uint32 *reply)
{
	Uint32 timeout = 2;
	Uint32 count = 0;
	Uint32 oldtime = 0;
	fd_set fds;
	int retval = -1;
	Uint32 val = htonl(UMSG_ACCEPT);

	oldtime = time(NULL);
	FD_ZERO(&fds);
	FD_SET(sfd, &fds);
	while (timeout < 32) {
		struct timeval t = {1, 0};
		int n;

		if (count == 0 && write(sfd, &val, sizeof(val)) != sizeof(val))
			break;
		if ((n = select(sfd + 1, &fds, 0, 0, &t)) < 0) {
			perror("select");
			break;
		}
		else if (n == 0) {
			udp_timeout();
			FD_SET(sfd, &fds);
			continue;
		}
		if (FD_ISSET(sfd, &fds)) {
			Uint32 type;
			retval = read(sfd, &type, sizeof(type));
			*reply = ntohl(type);
			break;
		}

		udp_timeout();
	};

	return retval;
}

USocket *
usock_connect(int fd, struct sockaddr_in *addr_p, void *cred_p, Uint32 clen)
{
	USocket *usock;
	Uint32 val;
	int n;

	if (!(usock = create_usock(fd, addr_p, cred_p, clen)))
		return NULL;

	n = send_connect_msg(fd, cred_p, clen, &val);
	if (n != sizeof(val) || val != UMSG_ACCEPT)
		goto reterr;

	val = htonl(UMSG_REPLY_ACCEPT);
	if (write(fd, &val, sizeof(val)) == sizeof(val)) {
		usock->status |= USOCK_CONNECT;
		sem_post(&usock->sem);
		return usock;
	}

reterr:
	sem_post(&usock->sem);
	usock_close(usock);
	return NULL;
}

USocket *usock_accept(int fd, void *cred_p, int clen)
{
	USocket *usock = NULL;
	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);
	char buf[clen];
	Uint32 val, n;

	while (1) {
		n = recvfrom(fd, buf, clen, 0, (Sockaddr *)&addr, &alen);
		if (n < 0) {
			perror("recvform");
			break;
		}
		else if (n == clen && memcmp(cred_p, buf, clen) == 0) {
			usock = create_usock(fd, &addr, cred_p, clen);
			break;
		}
	}

	if (usock == NULL)
		return NULL;

	n = send_accept_msg(fd, &val);
	if (n == sizeof(val) || val == UMSG_REPLY_ACCEPT) {
		usock->status |= USOCK_CONNECT;
		sem_post(&usock->sem);
		return usock;
	}

	sem_post(&usock->sem);
	usock_close(usock);
	return NULL;
}

int usock_close(USocket *usock)
{
	int timeout = 2;

	if (usock->status & USOCK_CONNECT) {
		pthread_mutex_lock(&usock->wlock);
		while (timeout < 16) {
			struct timespec tsp;
			if (send_umsg(usock, UMSG_CLOSE, NULL, 0) < 0)
				break;
			maketimeout(&tsp, timeout);
			if (pthread_cond_timedwait(&usock->wcond, &usock->wlock, &tsp) != ETIMEDOUT)
				break;
			timeout *= 2;
		}
		pthread_mutex_lock(&usock->wlock);
	}

	usock->status &= ~USOCK_CONNECT;
	usock->status |= USOCK_CLOSE;
	close(usock->fd);
	sem_wait(&usock->sem);
	free(usock);
	return 0;
}

static char tbuf[MAX_MTU];
static void *
usock_read_handler(void *data)
{
	USocket *usock = (USocket *)data;

	sem_wait(&usock->sem);

	while (!(usock->status & USOCK_CLOSE)) {
		umsg_t umsg;

		if (recv_umsg(usock, &umsg, tbuf, MAX_MTU) < 0)
			goto err;

		switch (umsg.mtype) {
		case UMSG_PACK:
		{
			Uint32 mcount = htonl(umsg.mcount);
			Uint32 mtype;

			pthread_mutex_lock(&usock->rlock);
			if (umsg.mcount > usock->r_pack_count) {
				if (umsg.mcount > usock->r_pack_count + 1)
					goto err;
				if (buffer_space(usock->rbuf) < umsg.mlen) {
					usock->tsize = umsg.mlen;
					memcpy(usock->tbuf, tbuf, umsg.mlen);
				}
				else {
					buffer_write(usock->rbuf, tbuf, umsg.mlen);
				}
				usock->r_pack_count = umsg.mcount;
			}
			pthread_mutex_unlock(&usock->rlock);
			pthread_cond_signal(&usock->rcond);

			if (usock->tsize > 0)
				mtype = UMSG_BUSY;
			else
				mtype = UMSG_PACK_COUNT;
			if (send_umsg(usock, mtype, &mcount, sizeof(mcount)) < 0)
				goto err;

			continue;
		}

		case UMSG_PACK_COUNT:
		{
			int mcount = ntohl(*(Uint32 *)tbuf);
			//printf("UMSG_PACK_COUNT:%d  %d\n", mcount, usock->w_pack_count);
			if (mcount == usock->w_pack_count)
				pthread_cond_signal(&usock->wcond);
			continue;
		}

		case UMSG_BUSY:
		{
			int mcount = ntohl(*(Uint32 *)tbuf);
			//printf("UMSG_BUSY:%d  %d\n", mcount, usock->w_pack_count);
			if (mcount == usock->w_pack_count) {
				usock->status |= USOCK_PEER_BUSY;
				pthread_cond_signal(&usock->wcond);
			}
			continue;
		}

		case UMSG_CONT:
		{
			Uint32 mcount = ntohl(*(Uint32 *)tbuf);
			if (send_umsg(usock, UMSG_REPLY_CONT, tbuf, sizeof(mcount)) < 0)
				goto err;
			if (mcount == usock->w_pack_count) {
				if (usock->status & USOCK_PEER_BUSY) {
					usock->status &= ~USOCK_PEER_BUSY;
					pthread_cond_signal(&usock->wcond);
				}
			}
			continue;
		}

		case UMSG_REPLY_CONT:
		{
			Uint32 mcount = ntohl(*(Uint32 *)tbuf);
			//printf("UMSG_REPLY_CONT:%d  %d\n", mcount, usock->w_pack_count);
			if (mcount == usock->r_pack_count)
				pthread_cond_signal(&usock->rcond);
			continue;
		}

		case UMSG_CLOSE:
			send_umsg(usock, UMSG_REPLY_CLOSE, NULL, 0);
			usock->status &= ~USOCK_CONNECT;
			pthread_cond_broadcast(&usock->rcond);
			goto err;

		case UMSG_REPLY_CLOSE:
			pthread_cond_broadcast(&usock->wcond);
			goto err;

		default: continue;
		}
	}

err:
	pthread_mutex_unlock(&usock->rlock);
	sem_post(&usock->sem);
	return NULL;
}

int usock_read(USocket *usock, void *data, Uint32 size)
{
	int retval = -1;

	if (size <= 0)
		return 0;

	if (usock->status & USOCK_CLOSE)
		return -1;
	if (!(usock->status & USOCK_CONNECT))
		return 0;

	pthread_mutex_lock(&usock->rlock);

	if (usock->tsize > 0) {
		int n = buffer_write(usock->rbuf, usock->tbuf, usock->tsize);
		usock->tsize -= n;

		if (usock->tsize == 0) {
			int timeout = 1;
			int ret;
			Uint32 mcount = htonl(usock->r_pack_count);
			while (timeout < 16) {
				struct timespec tsp;
				maketimeout(&tsp, timeout);
				if (send_umsg(usock, UMSG_CONT, &mcount, sizeof(mcount)) < 0)
					return -1;
				ret = pthread_cond_timedwait(&usock->rcond, &usock->rlock, &tsp);
				if (ret == 0)
					break;
				if (ret != ETIMEDOUT)
					return -1;
				timeout *= 2;
			}
			if (timeout >= 16)
				return -1;
		}
	}

	while (buffer_empty(usock->rbuf)) {
		if (usock->status & USOCK_CLOSE)
			goto err;
		if (!(usock->status & USOCK_CONNECT)) {
			retval = 0;
			goto err;
		}

		pthread_cond_wait(&usock->rcond, &usock->rlock);
	}

	retval = buffer_read(usock->rbuf, data, size);

err:
	pthread_mutex_unlock(&usock->rlock);
	pthread_cond_signal(&usock->wcond);

	return retval;
}

int usock_write(USocket *usock, const void *data, Uint32 len)
{
	int timeout = 2;
	int retval = -1;

	if (len <= 0)
		return 0;
	if ((usock->status & USOCK_CLOSE) ||
			!(usock->status & USOCK_CONNECT))
		return -1;

	pthread_mutex_lock(&usock->wlock);

	while (timeout < 16) {
		struct timespec tsp;
		int ret;

		if ((retval = send_umsg(usock, UMSG_PACK, data, len)) < 0)
			goto err;

		maketimeout(&tsp, timeout);
		ret = pthread_cond_timedwait(&usock->wcond, &usock->wlock, &tsp);
		if ((usock->status & USOCK_CLOSE) || !(usock->status & USOCK_CONNECT)) {
			retval = -1;
			break;
		}
		if (ret == 0) {
			while (usock->status & USOCK_PEER_BUSY) {
				pthread_cond_wait(&usock->wcond, &usock->wlock);
				if ((usock->status & USOCK_CLOSE) ||
						!(usock->status & USOCK_CONNECT)) {
					retval = -1;
					goto err;
				}
			}
			break;
		}
		else if (ret != ETIMEDOUT) {
			retval = -1;
			goto err;
		}

		timeout *= 2;
	}

	if (timeout >= 16)
		retval = -1;
	else
		usock->w_pack_count++;
err:
	pthread_mutex_unlock(&usock->wlock);
	return retval;
}


		//Uint32 diff;
		//Uint32 old = get_usec();
		//diff = get_usec() - old;
		//if (diff > 1000) printf("%u\n", diff);
