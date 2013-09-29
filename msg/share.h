#ifndef __SHARE_H
#define __SHARE_H

#include "logmsg.h"

#define TRANS_TUNNEL_END			1u
#define TRANS_TUNNEL_DATA			2u

#define IP_BUF_SIZE					24
#define MAX_BUF_SIZE				16383
#define READ_TIMEOUT				300

#define PIPEMSG_MAX_SIZE			100

typedef unsigned char  Uint8;
typedef unsigned short Uint16;
typedef unsigned int   Uint32;
typedef struct sockaddr Sockaddr;

typedef struct MsgBuf_s {
	unsigned short type;
    unsigned short size;
	char data[MAX_BUF_SIZE];
} Msgbuf_t;
#define MSGBUF_HEAD_SIZE	(sizeof(Msgbuf_t) - MAX_BUF_SIZE)

typedef unsigned short Port_t;

typedef struct {
    int pos;
    int len;
    int size;
    char data[0];
} Buffer;

inline static int buffer_space(Buffer *buf)
{
	return buf->size - buf->len;
}

inline static int buffer_empty(Buffer *buf)
{
	return (buf->len == 0);
}
int buffer_write(Buffer *buf, void *data, int size);
int buffer_read(Buffer *buf, void *data, int size);
Buffer *buffer_create(int max);

int socket_listen(unsigned short *port, char *listenip, int queue);
int socket_is_usable(int sock);
int get_host_address(
		const char			*host,
		struct sockaddr_in	*addr,
		struct in_addr		**addrlist);
int connect_to_host(const char *host,
		const unsigned short port,
		struct sockaddr_in *fromaddr,
		struct sockaddr_in *toaddr,
		unsigned short timeout);
void set_linger(int fd, int on, int time);
void set_keep_alive(int fd);
void set_non_block(int fd, unsigned long block);
char * ip_string(struct in_addr addr, char *buf);
int ipaddr_cmp(struct sockaddr_in *addr1, struct sockaddr_in *addr2);
char * ip2str(struct sockaddr_in *addr);
ssize_t writen(int fd, const void *ptr, size_t n);
ssize_t readn(int fd, void *ptr, size_t n);
int transmit_loop(int sockfd[2], char *buf, int bsize);
#endif

