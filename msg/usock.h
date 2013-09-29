#ifndef __USOCK_H
#define __USOCK_H

#include "common.h"
#include "share.h"

#define UDPTIMEOUT		1
#define MAX_MTU			1500
#define MAX_UBUF		(MAX_MTU - sizeof(umsg_t))

typedef struct _USocket {
	int fd;
	int status;
	sem_t sem;
	char credv[MAX_CRED];
	int clen;
	pthread_mutex_t rlock;
	pthread_mutex_t wlock;
	pthread_cond_t rcond;
	pthread_cond_t wcond;
	Uint32 r_pack_count;
	Uint32 w_pack_count;
	Buffer *rbuf;
	char tbuf[MAX_MTU];
	int tsize;
	pthread_t pid;
} USocket;

USocket *usock_connect(int fd, struct sockaddr_in *addr_p, void *cred_p, Uint32 clen);
USocket *usock_accept(int fd, void *cred_p, int clen);
int usock_close(USocket *usock);
int usock_read(USocket *usock, void *data, Uint32 size);
int usock_write(USocket *usock, const void *data, Uint32 len);

#endif

