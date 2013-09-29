#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <semaphore.h>

#include "share.h"

static pthread_mutex_t mutexlock;
//pthread_cond_t condition;
//pthread_attr_t threadattr;

int get_host_address(
		const char			*host,
		struct sockaddr_in	*addr,
		struct in_addr		**addrlist)
{
	struct hostent *entry = NULL;
	int result = 0;

	pthread_mutex_lock(&mutexlock);

	if ((addr->sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
		if ((entry = gethostbyname(host)) == NULL)
			result = -1;
		else
			memcpy(&addr->sin_addr, &entry->h_addr, entry->h_length);
	}

	if (addrlist != NULL) {
		if (entry != NULL) {
			int i, count;

			for (count = 0; entry->h_addr_list[count]; count++);

			*addrlist = (struct in_addr *)calloc(count + 1, sizeof(struct in_addr));

			if (*addrlist != NULL) {
				for (i = 0; i < count; i++)
					memcpy(&((*addrlist)[i]), entry->h_addr_list[i], sizeof(struct in_addr));
				memset(&((*addrlist)[i]), 0xff, sizeof(struct in_addr));
			}
			else
				result = -1;
		}
		else {
			*addrlist = (struct in_addr *)calloc(2, sizeof(struct in_addr));
			memcpy(&((*addrlist)[0]), &(addr->sin_addr), sizeof(struct in_addr));
			memset(&((*addrlist)[1]), 0xff, sizeof(struct in_addr));
		}
	}

	pthread_mutex_unlock(&mutexlock);

	return result;
}

int socket_listen(unsigned short *port, char *listenip, int queue)
{
	int sfd = -1;
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	int val = 1;

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LOG_MSG(1, errno, "can't create listener socket");
		goto failure;
	}

	if (port && *port) {
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val)) < 0)
			LOG_MSG(1, 0, "Warning: failed to set SO_REUSEADDR option on socket");
	}

	memset(&addr, 0, sizeof(addr));
	if (listenip != NULL) {
		if (get_host_address(listenip, &addr, NULL) < 0)
			LOG_MSG(1, 0, "can't resolve listen address '%s'", listenip);
	}
	else {
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	addr.sin_port = (port ? htons(*port) : 0);
	LOG_MSG(3, 0, "listening on %s", ip2str(&addr));

	if (bind(sfd, (struct sockaddr *)&addr, addrlen) < 0) {
		LOG_MSG(1, errno, "listener bind failed");
		goto failure;
	}

	if (listen(sfd, queue) < 0) {
		LOG_MSG(1, errno, "listen failed");
		goto failure;
	}

	if (port) {
		memset(&addr, 0, sizeof(addr));
		if (getsockname(sfd, (struct sockaddr *)&addr, (socklen_t *)&addrlen)) {
			LOG_MSG(0, errno, "can't get local port number");
			goto failure;
		}
		*port = ntohs(addr.sin_port);
	}

	return sfd;

failure:
	if (sfd != -1)
		close(sfd);

	return -1;
}

int socket_is_usable(int sock)
{
    fd_set testset;
    struct timeval delay;
    unsigned char buf[1];
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
	int size;

    if (getpeername(sock, (struct sockaddr *)&addr, &addrlen)) {
		LOG_MSG(1, errno, "getpeername %s", ip2str(&addr));
		return -1;
    }

    FD_ZERO(&testset);
    FD_SET(sock, &testset);
    delay.tv_sec = 0;
    delay.tv_usec = 0;
    if (select(sock + 1, 0, &testset, 0, &delay) <= 0) {
		LOG_MSG(1, 0, "socket %d is not writable", sock);
		return -1;
    }

    FD_ZERO(&testset);
    FD_SET(sock, &testset);
    delay.tv_sec = 0;
    delay.tv_usec = 0;
    if (select(sock + 1, &testset, 0, 0, &delay) > 0) {
		LOG_MSG(4, 0, "socket %d is readable, checking for EOF", sock);
		errno = 0;
		if ((size = recv(sock, buf, sizeof(buf), MSG_PEEK)) < 0) {
			LOG_MSG(1, errno, "socket %d error", sock);
			return -1;
		}
		else if (size == 0) {
			LOG_MSG(1, errno, "socket %d has immediate EOF", sock);
			return -1;
		}
    }

    return 0;
}

void set_linger(int fd, int on, int time)
{
    struct linger val;

    val.l_onoff = on;
    val.l_linger = time;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&val, sizeof(val)) < 0)
		LOG_MSG(1, 0, "Warning: failed to set SO_LINGER option on socket");
}

void set_keep_alive(int fd)
{
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&val, sizeof(val)) < 0)
		LOG_MSG(1, 0, "Warning: failed to set SO_KEEPALIVE option on socket");
}

void set_non_block(int fd, unsigned long block)
{
    fcntl(fd, F_SETFL, (block ? O_NONBLOCK : 0));
}

int connect_to_host(const char *host,
		const unsigned short port,
		struct sockaddr_in *fromaddr,
		struct sockaddr_in *toaddr,
		unsigned short timeout)
{
    int sfd = -1;
    int ready = -1;
    struct sockaddr_in addr;
    struct timeval delay;
    fd_set testset;

    assert(host != NULL && port != 0);

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LOG_MSG(0, errno, "socket creation failed");
		errno = 0;
		return -1;
    }

    if (fromaddr && fromaddr->sin_addr.s_addr) {
		memset(&addr, 0, sizeof(addr));
		addr.sin_addr.s_addr = fromaddr->sin_addr.s_addr;
		if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			LOG_MSG(1, errno, "WARNING: failed to set connection source address -- ignored");
    }

    memset(&addr, 0, sizeof(addr));
    if (get_host_address(host, &addr, NULL) < 0) {
		LOG_MSG(0, 0, "can't resolve host or address '%s'", host);
		close(sfd);
		return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

	set_linger(sfd, 0, 0);

	set_non_block(sfd, 1);

	if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		if (errno != EWOULDBLOCK && errno != EINPROGRESS && errno != EINTR) {
			close(sfd);
			return -1;
		}
	}

	delay.tv_sec = timeout;
	delay.tv_usec = 0;
	FD_ZERO(&testset);
	FD_SET(sfd, &testset);

	ready = select(sfd + 1, 0, &testset, 0, &delay);

	if (ready <= 0) {
		close(sfd);
		return -1;
	}

	set_non_block(sfd, 0);

	if (socket_is_usable(sfd) < 0) {
		close(sfd);
		return -1;
	}

    if (toaddr)
		memcpy(toaddr, &addr, sizeof(addr));

    return sfd;
}

int ipaddr_cmp(struct sockaddr_in *addr1, struct sockaddr_in *addr2)
{
	if (addr1->sin_addr.s_addr == addr2->sin_addr.s_addr
			&& addr1->sin_port == addr2->sin_port
			&& addr1->sin_family == addr2->sin_family)
		return 0;

	return -1;
}

char * ip2str(struct sockaddr_in *addr)
{
	static char buf[24];
	inet_ntop(AF_INET, &addr->sin_addr, buf, 16);
	sprintf(buf, "%s:%u", buf, ntohs(addr->sin_port));
	return buf;
}

ssize_t readn(int fd, void *ptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;

	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return -1;
			else
				break;
		}
		else if (nread == 0) {
			break;
		}
		nleft -= nread;
		ptr   += nread;
	}
	return n - nleft;
}

ssize_t writen(int fd, const void *ptr, size_t n)
{
	size_t	nleft;
	ssize_t	nwritten;

	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return -1;
			else
				break;
		}
		else if (nwritten == 0)
			break;
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return n - nleft;
}

int transmit_loop(int sockfd[2], char *buf, int bsize)
{
    fd_set savefds;
    int maxfd = (sockfd[0] > sockfd[1] ? sockfd[0] : sockfd[1]) + 1;
    int num = 0;

	FD_ZERO(&savefds);
	FD_SET(sockfd[0], &savefds);
	FD_SET(sockfd[1], &savefds);

	for (;;) {
		fd_set fds = savefds;
		if (select(maxfd, &fds, NULL, NULL, NULL) <= 0) {
			perror("error in select");
			return -1;
		}

		if (FD_ISSET(sockfd[0], &fds)) {
			num = readn(sockfd[0], buf, bsize);
			if (num < 0) {
				perror("read error closed connection");
				return -1;
			}
			else if (num == 0) {
				fprintf(stderr, "peer connection closed\n");
				return 0;
			}
			else if (writen(sockfd[1], buf, num) != num) {
				perror("write data faild");
				return -1;
			}
		}

		if (FD_ISSET(sockfd[1], &fds)) {
			num = readn(sockfd[1], buf, bsize);
			if (num < 0) {
				LOG_MSG(1, errno, "read tunnel error connection closed");
				return -1;
			}
			else if (num == 0) {
				fprintf(stderr, "peer connection closed\n");
				return 0;
			}
			else if (writen(sockfd[0], buf, num) != num) {
				perror("write data faild");
				return -1;
			}
		}
	}

    return 0;
}

Buffer *buffer_create(int max)
{
	Buffer *buffer;

	buffer = malloc(sizeof(Buffer) + max);
	buffer->pos = 0;
	buffer->len = 0;
	buffer->size = max;

	return buffer;
}

int buffer_read(Buffer *buf, void *data, int size)
{
	char *t = (char *)data;

	if (size > buf->len)
		size = buf->len;

	if (buf->pos + size > buf->size) {
		int n = buf->size - buf->pos;
		memcpy(t, buf->data + buf->pos, n);
		buf->pos  = 0;
		buf->len -= n;
		size -= n;
		t    += n;
	}

	memcpy(t, buf->data + buf->pos, size);
	buf->pos += size;
	buf->len -= size;
	t += size;

	if (buf->len == 0)
		buf->pos = 0;

	return (int)(t - (char *)data);
}

int buffer_write(Buffer *buf, void *data, int size)
{
	char *t = (char *)data;

	if (size > buf->size - buf->len)
		size = buf->size - buf->len;

	while (size > 0) {
		int n, begin;

		if (buf->len == 0) {
			begin = 0;
			n = buf->size;
		}
		else {
			begin = (buf->pos + buf->len) % buf->size;
			if (begin > buf->pos)
				n = buf->size - begin;
			else
				n = buf->pos - begin;
		}

		if (n > size) n = size;

		memcpy(buf->data + begin, t, n);

		buf->len += n;
		size -= n;
		t    += n;
	}

	return (int)(t - (char *)data);
}

