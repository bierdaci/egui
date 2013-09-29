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
			if (n == 2)
				seraddr->sin_port = htons(atoi(port));
			else
				seraddr->sin_port = htons(PROTREGIST);

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
	RegisterInfo reg;
	char password[MAX_PASSWD];
	fd_set allfds;

	if (args_ip(argc, argv, &seraddr) < 0)
		return 1;

	printf("username:");
	fflush(stdout);
	fgets(reg.name, sizeof(reg.name), stdin);
	int n = strlen(reg.name);
	if (reg.name[n-1] == '\n')
		reg.name[n-1] = 0;

	strcpy(reg.passwd, getpass("passwd:"));
	strcpy(password, getpass("re-enter passwd:"));
	if (strcmp(reg.passwd, password)) {
		fprintf(stderr, "Enter the password is not the same.\n");
		return 1;
	}

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		return 1;
    }

	if (connect(sfd, (Sockaddr *)&seraddr, sizeof(seraddr)) < 0) {
		fprintf(stderr, "socket connect failed\n");
		return 1;
	}

	if (writen(sfd, &reg, sizeof(reg)) != sizeof(reg)) {
		perror("register fail write\n");
		return 1;
	}

	if (readn(sfd, &confirm, sizeof(confirm)) != sizeof(confirm)
			|| confirm != REGSUCCESS) {
		perror("register fail\n");
		return 1;
	}

	printf("register success\n");
	return 0;
}

