#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "list.h"
#include "share.h"

#include "xml.h"
#include "common.h"
#include "thread.h"

#define MAX_FRIENDS		100
#define MAX_GROUP		20
#define USER_PATH		"./user/"

typedef struct _User User;
typedef struct _Friend Friend;
typedef struct _Contact Contact;
typedef struct _TrHandler TrHandler;
struct _Friend {
	char name[MAX_USERNAME];
	char group[MAX_GROUP];
	User *user;
	list_t list;
};

struct _Contact {
	char name[MAX_USERNAME];
	User *user;
	list_t list;
};

struct _User {
	unsigned int type;
	unsigned int status;
	char name[MAX_USERNAME];
	char passwd[MAX_PASSWD];
	unsigned int userid;
	int fd;
	int max_friend;
	int offm_nr;

	list_t contact_h;
	list_t friend_h;
	list_t offmsg_h;
	list_t wait_h;

	list_t on_list;
	list_t reg_list;
	Xml   *xml;
};

typedef enum {WaitTypeFriend, WaitTypeData} WaitType;
typedef struct _Wait Wait;
struct _Wait {
	WaitType type;
	char   name[MAX_USERNAME];
	char   credv[MAX_CRED];
	int    clen;
	time_t timeout;
	User   *user;
	Wait   *peer;
	list_t list;
};

#define TRTIMEOUT		30
typedef enum {TrStatusReady, TrStatusWork} TrStatus;
struct _TrHandler {
	TrStatus status;
	int  fd, sockfd[2];
	char credv[MAX_CRED];
	int  clen;
	void *buf;
	int bsize;
	time_t start_time;
	pthread_mutex_t	lock;
	sem_t sem;
	list_t list;
};

typedef struct {
	User *user;
	char credv[MAX_CRED];
	int  clen;
	void *data;
	int len;
	list_t list;
} UReadyQueue;

static pthread_mutex_t uready_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t tr_lock = PTHREAD_MUTEX_INITIALIZER;
static ThreadPool *thread_pool;

static LIST_HEAD(trans_head);
static LIST_HEAD(register_head);
static LIST_HEAD(online_head);
static LIST_HEAD(uready_head);

static int start_transfer(User *user, Wait *wait);

static int server_real_message(User *user, void *data, int len);
static int server_add_friend(User *user, void *data, int len);
static int server_del_friend(User *user, void *data, int len);
static int server_refuse_add(User *user, void *data, int len);
static int server_accept_add(User *user, void *data, int len);
static int server_accept_and_add(User *user, void *data, int len);
static int server_search_user(User *user, void *data, int len);
static int server_reply_who(User *user, void *data, int len);
static int server_reply_friend(User *user, void *data, int len);
static int server_request_transfer(User *user, void *data, int len);
static int server_accept_transfer(User *user, void *data, int len);
static int server_refuse_transfer(User *user, void *data, int len);
static int server_cancel_transfer(User *user, void *data, int len);
static struct {
	const char cmd[MAX_COMMAND];
	int (*handler)(User *user, void *data, int len);
} server_cmd[] = {
	{ REALMESSAGE,	server_real_message		},
	{ REQUESTADD,	server_add_friend		},
	{ DELFRIEND,	server_del_friend		},
	{ REFUSEADD,	server_refuse_add		},
	{ ACCEPTADD,	server_accept_add		},
	{ ACCEPTANDADD,	server_accept_and_add	},
	{ SEARCHUSER,	server_search_user		},
	{ SEARCHWHO,	server_reply_who		},
	{ SEARCHFRIEND,	server_reply_friend		},
	{ REQTRANS,		server_request_transfer	},
	{ ACCEPTTRANS,	server_accept_transfer	},
	{ REFUSETRANS,	server_refuse_transfer	},
	{ CANCELTRANS,	server_cancel_transfer	},
};

static ebool cred_cmp(void *cred1, void *cred2, int len)
{
	return(memcmp(cred1, cred2, len) == 0);
}

static void add_uready_queue(UReadyQueue *uq)
{
	pthread_mutex_lock(&uready_lock);
	list_add(&uq->list, &uready_head);
	pthread_mutex_unlock(&uready_lock);
}

static void del_uready_queue(UReadyQueue *uq)
{
	pthread_mutex_lock(&uready_lock);
	list_del(&uq->list);
	pthread_mutex_unlock(&uready_lock);
}

static UReadyQueue *find_uready_queue(void *credv, int clen)
{
	UReadyQueue *uq = NULL;
	list_t *pos;

	pthread_mutex_lock(&uready_lock);
	list_for_each(pos, &uready_head) {
		uq = list_entry(pos, UReadyQueue, list);
		if (uq->clen == clen && cred_cmp(uq->credv, credv, clen))
			break;
	}
	pthread_mutex_unlock(&uready_lock);

	return uq;
}

static void tr_free_thread(TrHandler *tr)
{
	pthread_mutex_lock(&tr_lock);
	list_del(&tr->list);
	free(tr);
	pthread_mutex_unlock(&tr_lock);
}

static int tr_handler(void *data)
{
	TrHandler *tr = (TrHandler *)data;

	sem_wait(&tr->sem);
	if (tr->fd == -1)
		goto err;
	sem_wait(&tr->sem);
	if (tr->fd == -1)
		goto err;

	tr->status = TrStatusWork;
	transmit_loop(tr->sockfd, tr->buf, tr->bsize);

err:
	if (tr->sockfd[0] >= 0)
		close(tr->sockfd[0]);
	if (tr->sockfd[1] >= 0)
		close(tr->sockfd[1]);

	tr_free_thread(tr);
	return 0;
}

static TrHandler *tr_alloc_thread(void *cred, int clen)
{
	TrHandler *tr;

	if (!(tr = malloc(sizeof(TrHandler) + MAX_BUFFER)))
		return NULL;

	tr->status = TrStatusReady;
	tr->bsize = MAX_BUFFER;
	tr->buf = (void *)(tr + 1);

	tr->fd = 0;
	tr->sockfd[0] = -1;
	tr->sockfd[1] = -1;
	tr->clen = clen;
	memcpy(tr->credv, cred, clen);
	sem_init(&tr->sem, 0, 0);
	tr->start_time = time(NULL);

	if (alloc_thread(thread_pool, tr_handler, tr) < 0) {
		free(tr);
		return NULL;
	}

	list_add(&tr->list, &trans_head);

	return  tr;
}

static TrHandler *tr_find_thread(void *cred, int clen)
{
	list_t *pos;
	TrHandler *tr = NULL;

	pthread_mutex_lock(&tr_lock);
	list_for_each(pos, &trans_head) {
		tr = list_entry(pos, TrHandler, list);
		if (clen == tr->clen
				&& cred_cmp(cred, tr->credv, clen)) {
			break;
		}
	}
	pthread_mutex_unlock(&tr_lock);

	return tr;
}

static ebool is_timeout(TrHandler *tr, time_t curtime)
{
	if (curtime - tr->start_time > TRTIMEOUT)
		return etrue;
	return efalse;
}

static void clear_timeout_thread(void)
{
	TrHandler *tr;
	list_t *pos;
	time_t curtime = time(NULL);

	pthread_mutex_lock(&tr_lock);
	list_for_each(pos, &trans_head) {
		tr = list_entry(pos, TrHandler, list);
		if (tr->status == TrStatusReady && is_timeout(tr, curtime)) {
			tr->fd = -1;
			sem_post(&tr->sem);
			sem_destroy(&tr->sem);
		}
	}
	pthread_mutex_unlock(&tr_lock);
}

static void release_ready_thread(Cred *cred, int clen)
{
	TrHandler *tr;
	if ((tr = tr_find_thread(cred, clen))) {
		tr->fd = -1;
		sem_post(&tr->sem);
		sem_destroy(&tr->sem);
		fprintf(stderr, "transfer fail\n");
	}
}

typedef struct {
	void *msg;
	int len;
	list_t list;
} OffLineMessage;
static int offline_sendv(User *user, struct iovec *mvec, int iovl)
{
	int i, len = 0;
	OffLineMessage *olm;
	char *p = 0;

	for (i = 0; i < iovl; i++)
		len += mvec[i].iov_len;
	olm = (OffLineMessage *)malloc(sizeof(OffLineMessage) + len);
	olm->len = len;
	olm->msg = olm + 1; 
	for (p = (char *)olm->msg, i = 0; i < iovl; i++) {
		memcpy(p, mvec[i].iov_base, mvec[i].iov_len);
		p += mvec[i].iov_len;
	}

	list_add(&olm->list, &user->offmsg_h);
	user->offm_nr++;

	return len;
}

static OffLineMessage *next_offmsg(User *user)
{
	list_t *pos;

	list_for_eachp(pos, &user->offmsg_h) {
		OffLineMessage *olm = list_entry(pos, OffLineMessage, list);
		list_del(pos);
		pos = pos->next;
		return olm;
	}
	return NULL;
}

static Friend* find_friend_by_name(User *user, const char *name)
{
	list_t *pos;

	list_for_each(pos, &user->friend_h) {
		Friend *fri = list_entry(pos, Friend, list);
		if (strcmp(fri->name, name) == 0)
			return fri;
	}

	return NULL;
}

static User* find_friend_user(User *user, const char *name)
{
	Friend *fri;
	if ((fri = find_friend_by_name(user, name)))
		return fri->user;
	return NULL;
}

static Contact* find_contact_by_name(User *user, const char *name)
{
	list_t *pos;

	list_for_each(pos, &user->contact_h) {
		Contact *con = list_entry(pos, Contact, list);
		if (strcmp(con->name, name) == 0)
			return con;
	}

	return NULL;
}

static User* find_contact_user(User *user, const char *name)
{
	Contact *con;
	if ((con = find_contact_by_name(user, name)))
		return con->user;
	return NULL;
}

static User* find_user(const char *name)
{
	list_t *pos;

	list_for_each(pos, &register_head) {
		User *user;
		user = list_entry(pos, User, reg_list);
		if (strcmp(user->name, name) == 0)
			return user;
	}

	return NULL;
}

static Wait *find_wait_by_cred(User *user, void *cred)
{
	list_t *pos;
	list_for_each(pos, &user->wait_h) {
		Wait *wait = list_entry(pos, Wait, list);
		if (cred_cmp(wait->credv, cred, wait->clen))
			return wait;
	}
	return NULL;
}

/*
static Wait *find_wait_by_name(User *user, char *name, void *cred)
{
	list_t *pos;
	list_for_each(pos, &user->wait_h) {
		Wait * wait = list_entry(pos, Wait, list);
		if (strcmp(wait->name, name))
			return wait;
	}
	return NULL;
}
*/

static void free_wait(Wait *wait)
{
	list_del(&wait->list);
	free(wait);
}

static void clear_data_wait(User *user)
{
	list_t *pos;

	list_for_each(pos, &user->wait_h) {
		Wait *wait = list_entry(pos, Wait, list);
		if (wait->type == WaitTypeData) {
			Command cm;
			struct iovec mvec[2];

			strcpy(cm.cmd, CANCELTRANS);
			cm.len = wait->clen;
			mvec[0].iov_base = &cm;
			mvec[0].iov_len  = sizeof(cm);
			mvec[1].iov_base = wait->credv;
			mvec[1].iov_len  = wait->clen;
			writev(wait->user->fd, mvec, 2);

			free_wait(wait->peer);
			free_wait(wait);
			pos = pos->next;
		}
	}
}

static int free_user_wait(User *user1, User *user2)
{
	Wait *wait;

	if ((wait = find_wait_by_cred(user1, user2->name))) {
		//wait over time limit return -1
		free_wait(wait);
		return 0;
	}
	return -1;
}

static Wait* add_wait(User *user, User *d_user, WaitType type, void *pcred, int clen)
{
	Wait *wait = (Wait *)malloc(sizeof(Wait));
	wait->type = type;
	wait->timeout = time(NULL);
	strcpy(wait->name, d_user->name);
	if (pcred) {
		memcpy(wait->credv, pcred, clen);
		wait->clen = clen;
	}
	wait->user = d_user;
	wait->peer = NULL;
	list_add(&wait->list, &user->wait_h);

	return wait;
}

static void update_wait(User *user, User *wuser)
{
	Wait *wait;

	if ((wait = find_wait_by_cred(user, wuser->name))) {
		wait->timeout = time(NULL);
		return;
	}

	add_wait(user, wuser, WaitTypeFriend, wuser->name, strlen(wuser->name)+1);
}

static int server_request_transfer(User *s_user, void *data, int len)
{
	User *d_user;
	Command cm;
	DataInfoHead *dh;
	struct iovec mvec[2];

	if (len != sizeof(DataInfoHead))
		return -1;

	dh = (DataInfoHead *)data;
	if ((!(d_user = find_friend_user(s_user, dh->name)) &&
				!(d_user = find_contact_user(s_user, dh->name)))
			|| d_user->fd < 0)
		return -1;

	strcpy(dh->name, s_user->name);
	if (dh->type == DataTypeFileU || dh->type == DataTypeAudioU) {
		UReadyQueue *uq;
		uq = (UReadyQueue *)malloc(sizeof(UReadyQueue) + sizeof(DataInfoHead));
		uq->user = d_user;
		uq->data = uq + 1;
		uq->len = sizeof(DataInfoHead);
		memcpy(uq->credv, &dh->cred, sizeof(dh->cred));
		uq->clen = sizeof(dh->cred);
		memcpy(uq->data, dh, len);
		add_uready_queue(uq);

		strcpy(cm.cmd, UREADYTRANS);
		cm.len = sizeof(dh->cred);
		mvec[0].iov_base = &cm;
		mvec[0].iov_len  = sizeof(cm);
		mvec[1].iov_base = &dh->cred;
		mvec[1].iov_len  = sizeof(dh->cred);
		return writev(s_user->fd, mvec, 2);
	}
	else {
		Wait *wait1, *wait2;

		wait1 = add_wait(d_user, s_user, WaitTypeData, &dh->cred, sizeof(dh->cred));
		wait2 = add_wait(s_user, d_user, WaitTypeData, &dh->cred, sizeof(dh->cred));
		wait1->peer = wait2;
		wait2->peer = wait1;

		strcpy(cm.cmd, REQTRANS);
		cm.len = len;
		mvec[0].iov_base = &cm;
		mvec[0].iov_len  = sizeof(cm);
		mvec[1].iov_base = dh;
		mvec[1].iov_len  = len;
		if (writev(d_user->fd, mvec, 2) < 0) {
			perror("writev");
			free_user_wait(s_user, d_user);
			//send transfer fail
			return -1;
		}
	}

	return 0;
}

static int server_cancel_transfer(User *user, void *data, int len)
{
	DataInfoHead *dh = (DataInfoHead *)data;
	Wait *wait;

	if ((wait = find_wait_by_cred(user, &dh->cred))) {
		Command cm;
		struct iovec mvec[2];

		strcpy(cm.cmd, CANCELTRANS);
		cm.len = sizeof(dh->cred);
		mvec[0].iov_base = &cm;
		mvec[0].iov_len  = sizeof(cm);
		mvec[1].iov_base = &dh->cred;
		mvec[1].iov_len  = cm.len;
		writev(wait->user->fd, mvec, 2);

		free_wait(wait->peer);
		free_wait(wait);
	}
	else
		release_ready_thread(&dh->cred, sizeof(dh->cred));

	return 0;
}

static int server_accept_transfer(User *user, void *data, int len)
{
	Cred *cred = (Cred *)data;
	Wait *wait;

	if ((wait = find_wait_by_cred(user, cred))) {
		start_transfer(user, wait);
		free_wait(wait->peer);
		free_wait(wait);
		return 0;
	}
	return -1;
}

static int server_refuse_transfer(User *user, void *data, int len)
{
	Cred *cred = (Cred *)data;
	Wait *wait;

	if ((wait = find_wait_by_cred(user, cred))) {
		if (wait->user && wait->user->fd >= 0) {
			Command cm;
			struct iovec mvec[2];
			strcpy(cm.cmd, REFUSETRANS);
			cm.len = len;
			mvec[0].iov_base = &cm;
			mvec[0].iov_len  = sizeof(cm);
			mvec[1].iov_base = data;
			mvec[1].iov_len  = len;
			writev(wait->user->fd, mvec, 2);
			free_wait(wait->peer);
			free_wait(wait);
		}
	}

	return -1;
}

static int add_user_to_friend(User *user, User *user1, ebool wait)
{
	Command cm;
	struct iovec mvec[2];
	Friend  *fri;
	Contact *con;

	if (wait && free_user_wait(user, user1) < 0)
		return -1;

	fri = (Friend *)malloc(sizeof(Friend) + sizeof(Contact));
	fri->user = user1;
	strcpy(fri->name, user1->name);
	list_add(&fri->list, &user->friend_h);
	con = (Contact *)(fri + 1);
	con->user = user;
	strcpy(con->name, user->name);
	list_add(&con->list, &user1->contact_h);	/*write data base*/

	xml_add_element(
			xml_find_node_by_name(user->xml->root, "friend"),
			xml_create_node(user1->name));
	{
		char path[256];
		strcpy(path, USER_PATH);
		strcat(path, user->name);
		strcat(path, "/");
		strcat(path, "profile.xml");
		xml_save(user->xml, path);
	}

	strcpy(cm.cmd, ACCEPTADD);
	cm.len = sizeof(user1->name);
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);
	mvec[1].iov_base = user1->name;
	mvec[1].iov_len  = sizeof(user1->name);
	if (user->fd >= 0)
		writev(user->fd, mvec, 2);
	else
		offline_sendv(user, mvec, 2);

	return 0;
}

static int server_add_friend(User *user, void *data, int len)
{
	User *wuser;
	Command cm;
	struct iovec mvec[2];
	char *name = (char *)data;

	if (find_friend_by_name(user, name))
		return 0;

	if (!(wuser = find_user(name)))
		return -1;

	if (find_friend_by_name(wuser, user->name))
		return add_user_to_friend(user, wuser, efalse);

	update_wait(user, wuser);

	strcpy(cm.cmd, REQUESTADD);
	cm.len = len;
	strcpy(name, user->name);
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);
	mvec[1].iov_base = data;
	mvec[1].iov_len  = len;
	if (wuser->fd >= 0)
		writev(wuser->fd, mvec, 2);
	else
		offline_sendv(wuser, mvec, 2);

	return 0;
}

static int server_del_friend(User *user, void *data, int len)
{
	Command cm;
	Friend  *fri;
	Contact *con;
	char *name = (char *)data;
	struct iovec mvec[2];

	if (!(fri = find_friend_by_name(user, name)))
		return 0;

	list_del(&fri->list);
	con = (Contact *)(fri + 1);
	list_del(&con->list);
	free(fri);
	//if (find_contact_by_name(fri->user, user->name) != con)
	//	fpritnf(stderr, "error\n");

	XmlElement *parent = xml_find_node_by_name(user->xml->root, "friend");
	xml_del_node_by_name(parent, name);
	{
		char path[256];
		strcpy(path, USER_PATH);
		strcat(path, user->name);
		strcat(path, "/");
		strcat(path, "profile.xml");
		xml_save(user->xml, path);
	}

	strcpy(cm.cmd, DELFRIEND);
	cm.len = strlen(name) + 1;
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);
	mvec[1].iov_base = name;
	mvec[1].iov_len  = cm.len;
	writev(user->fd, mvec, 2);

	return 0;
}

static int server_reply_friend(User *user, void *data, int len)
{
	Command cm;
	struct iovec mvec[3];
	list_t *pos;

	strcpy(cm.cmd, REPLYFRIEND);
	cm.len = sizeof(user->name) + sizeof(ebool);
	list_for_each(pos, &user->friend_h) {
		Friend *fri = list_entry(pos, Friend, list);
		ebool online = (fri->user->fd >= 0);

		mvec[0].iov_base = &cm;
		mvec[0].iov_len  = sizeof(cm);
		mvec[1].iov_base = fri->user->name;
		mvec[1].iov_len  = sizeof(fri->user->name);
		mvec[2].iov_base = &online;
		mvec[2].iov_len  = sizeof(ebool);

		writev(user->fd, mvec, 3);
	}

	return 0;
}

static int server_reply_who(User *user, void *data, int len)
{
	Command cm;
	struct iovec mvec[2];
	list_t *pos;

	strcpy(cm.cmd, REPLYWHO);
	cm.len = sizeof(user->name);
	list_for_each(pos, &user->friend_h) {
		Friend *fri = list_entry(pos, Friend, list);
		if (fri->user->fd >= 0) {
			mvec[0].iov_base = &cm;
			mvec[0].iov_len  = sizeof(cm);
			mvec[1].iov_base = fri->user->name;
			mvec[1].iov_len  = sizeof(fri->user->name);
			writev(user->fd, mvec, 2);
		}
	}

	return 0;
}

static int server_search_user(User *user, void *data, int len)
{
	User *u;
	int   n;
	Command cm;
	struct iovec mvec[3];
	char name[MAX_USERNAME];

	u = find_user((char *)data);

	strcpy(cm.cmd, REPLYSEARCH);
	if (u) {
		ebool online = u->fd > 0 ? etrue : efalse;
		n = 3;
		strcpy(name, u->name);
		cm.len = MAX_USERNAME + sizeof(ebool);

		mvec[1].iov_base = name;
		mvec[1].iov_len  = len;
		mvec[2].iov_base = &online;
		mvec[2].iov_len  = sizeof(ebool);
	}
	else {
		n      = 1;
		cm.len = 0;
	}
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);

	return writev(user->fd, mvec, n);
}

static int server_accept_and_add(User *user, void *data, int len)
{
	User *d_user;
	char *name = (char *)data;

	if (!strcmp(user->name, name))
		return -1;
	if (!(d_user = find_user(name)))
		return -1;

	if (find_friend_by_name(d_user, user->name))
		return 0;

	if (add_user_to_friend(d_user, user, etrue) < 0)
		return 0;
	if (find_friend_by_name(user, name))
		return 0;
	return add_user_to_friend(user, d_user, efalse);
}

static int server_accept_add(User *user, void *data, int len)
{
	User *d_user;
	char *name = (char *)data;

	if (!strcmp(user->name, name))
		return -1;
	if (!(d_user = find_user(name)))
		return -1;

	if (find_friend_by_name(d_user, user->name))
		return 0;

	return add_user_to_friend(d_user, user, etrue);
}

static int server_refuse_add(User *user, void *data, int len)
{
	User *d_user;
	Command cm;
	struct iovec mvec[2];
	char  *name = (char *)data;

	if (!(d_user = find_user(name)))
		return -1;

	strcpy(cm.cmd, REFUSEADD);
	cm.len = sizeof(user->name);
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);
	mvec[1].iov_base = user->name;
	mvec[1].iov_len  = sizeof(user->name);
	if (d_user->fd >= 0)
		writev(d_user->fd, mvec, 2);
	else
		offline_sendv(d_user, mvec, 2);

	return 0;
}

static int server_real_message(User *user, void *data, int len)
{
	char *name;
	User *user1;
	Command  cmd;
	struct iovec mvec[2];

	if (len <= sizeof(RealMessage))
		return -1;

	name = (char *)data;

	if (!(user1 = find_friend_user(user, name))
			&& !(user1 = find_contact_user(user, name))) {
		return -1;
	}

	//check permission
	strcpy(cmd.cmd, REALMESSAGE);
	cmd.len = len;
	strcpy(name, user->name);
	mvec[0].iov_base = &cmd;
	mvec[0].iov_len  = sizeof(cmd);
	mvec[1].iov_base = data;
	mvec[1].iov_len  = len;
	if (user1->fd >= 0)
		writev(user1->fd, mvec, 2);
	else
		offline_sendv(user1, mvec, 2);

	return 0;
}

static void add_online_user(User *user, int fd)
{
	user->fd = fd;
	list_add(&user->on_list, &online_head);
}

static void del_online_user(User *user)
{
	close(user->fd);
	user->fd = -1;
	list_del(&user->on_list);
}

typedef enum { Online, Offline } LineType;
static int send_online_msg(User *user, LineType type)
{
	struct iovec mvec[2];
	Command cm;
	list_t *pos;

	cm.len = sizeof(user->name);
	if (type == Online)
		strcpy(cm.cmd, NOTIFYON);
	else
		strcpy(cm.cmd, NOTIFYOFF);
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);
	mvec[1].iov_base = user->name;
	mvec[1].iov_len  = sizeof(user->name);

	list_for_each(pos, &user->contact_h) {
		Contact *con = list_entry(pos, Contact, list);
		if (con->user->fd >= 0)
			writev(con->user->fd, mvec, 2);
	}
	return 0;
}

static User* login_user(int listen_fd, fd_set *allset)
{
	int reply = 1;
	int clifd;
	User *user;
	LoginInfo login;
	OffLineMessage *olm;

	if ((clifd = accept(listen_fd, NULL, NULL)) < 0) {
		perror("accept");
		return NULL;
	}
	if (readn(clifd, &login, sizeof(login)) != sizeof(login)) {
		perror("login fail write\n");
		close(clifd);
		return NULL;
	}

	if ((user = find_user(login.name)) == NULL)
		reply = USERNOEXIST;
	else {
		if (strcmp(user->passwd, login.passwd) == 0)
			reply = LOGIONSUCCESS;
		else
			reply = PASSWDERR;
	}

	if (writen(clifd, &reply, sizeof(reply)) != sizeof(reply)) {
		fprintf(stderr, "client connect failed\n");
		close(clifd);
		return NULL;
	}

	if (reply != LOGIONSUCCESS) {
		close(clifd);
		return NULL;
	}

	if (user->fd > 0) {
		FD_CLR(user->fd, allset);
		del_online_user(user);
		//send exit msg
	}
	//else {
		add_online_user(user, clifd);
		send_online_msg(user, Online);
	//}

	while ((olm = next_offmsg(user))) {
		user->offm_nr--;
		writen(clifd, olm->msg, olm->len);
		free(olm);
	}
	return user;
}

static int start_transfer(User *user, Wait *wait)
{
	Command cm;
	struct iovec mvec[2];
	TrHandler *tr;

	if ((tr = tr_alloc_thread(wait->credv, wait->clen)) == NULL)
		return -1;

	strcpy(cm.cmd, STARTTRANS);
	cm.len = wait->clen;
	mvec[0].iov_base = &cm;
	mvec[0].iov_len  = sizeof(cm);
	mvec[1].iov_base = wait->credv;
	mvec[1].iov_len  = wait->clen;
	
	if ((writev(user->fd, mvec, 2) < 0) ||
			(writev(wait->user->fd, mvec, 2) < 0)) {
		perror("writev\n");
		tr->fd = -1;
		sem_post(&tr->sem);
		sem_destroy(&tr->sem);
	}

	return 0;
}

static void *listen_data_handler(void * data)
{
	Uint16 port = PORTFILE;
	int transfd, clifd;
	char buf[MAX_CRED];

	if ((transfd = socket_listen(&port, NULL, 1)) < 0) {
		perror("socket_listen");
		exit(1);
	}

	while (1) {
		int n, clen;
		TrHandler *tr;

		if ((clifd = accept(transfd, NULL, NULL)) < 0) {
			perror("transfer accept");
			continue;
		}
		if (readn(clifd, &clen, sizeof(clen)) != sizeof(clen)) {
			close(clifd);
			continue;
		}
		if ((n = readn(clifd, buf, clen)) != clen) {
			close(clifd);
			continue;
		}

		if ((tr = tr_find_thread(buf, clen))) {
			tr->sockfd[tr->fd++] = clifd;
			sem_post(&tr->sem);
			sem_destroy(&tr->sem);
			continue;
		}
		close(clifd);
	}
	return NULL;
}

static void *clear_thread_handler(void * data)
{
	while (1) {
		sleep(10);
		clear_timeout_thread();
	}
	return NULL;
}

static void *udp_transfer_handler(void *data)
{
	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);
	char credv[MAX_CRED];
	int sfd;

    if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed\n");
		return NULL;
    }
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PROTUDP);
	if (bind(sfd, (Sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		goto err;
	}

	while (1) {
		int n;
		UReadyQueue *uq;

		if ((n = recvfrom(sfd, credv, sizeof(credv),
						0, (Sockaddr *)&addr, &alen)) < 0) {
			perror("udp recvform");
			continue;
		}

		if ((uq = find_uready_queue(credv, n))) {
			DataInfoHead *dh = (DataInfoHead *)uq->data;
			if (uq->user->fd >= 0) {
				struct iovec mvec[2];
				Command cm;

				strcpy(dh->ipstr, ip2str(&addr));
				strcpy(cm.cmd, REQTRANS);
				cm.len = uq->len;
				mvec[0].iov_base = &cm;
				mvec[0].iov_len  = sizeof(cm);
				mvec[1].iov_base = uq->data;
				mvec[1].iov_len  = uq->len;
				writev(uq->user->fd, mvec, 2);
			}
			del_uready_queue(uq);
			free(uq);
		}
	}

err:
	close(sfd);
	return NULL;
}

static void load_friend_from_xml(User *user)
{
	XmlElement *parent = xml_find_node_by_name(xml_get_root_node(user->xml), "friend");
	XmlElement *elm = NULL;

	while ((elm = xml_next_element(user->xml, parent, elm))) {
		Friend  *fri;
		Contact *con;
		User *friend;

		if (!(friend = find_user(xml_get_node_name(elm))))
			continue;

		fri = (Friend *)malloc(sizeof(Friend) + sizeof(Contact));
		fri->user = friend;
		strcpy(fri->name, friend->name);
		list_add(&fri->list, &user->friend_h);
		con = (Contact *)(fri + 1);
		con->user = user;
		strcpy(con->name, user->name);
		list_add(&con->list, &friend->contact_h);
	}
}

static void load_friend(void)
{
	list_t *pos;
	list_for_each(pos, &register_head) {
		User *user = list_entry(pos, User, reg_list);
		load_friend_from_xml(user);
	}
}

static Xml *create_user_xml(const char *path, const char *name, const char *passwd)
{
	XmlElement *root;
	XmlElement *node, *leaf;
	Xml *xml = xml_create(100);

	root = xml_create_node("profile");
	xml_root_add_element(xml, root);

	node = xml_create_node("username");
	leaf = xml_create_leaf(name);
	xml_add_element(node, leaf);
	xml_add_element(root, node);

	node = xml_create_node("password");
	leaf = xml_create_leaf(passwd);
	xml_add_element(node, leaf);
	xml_add_element(root, node);

	node = xml_create_node("friend");
	xml_add_element(root, node);

	xml_save(xml, path);

	return xml;
}

static void add_user(const char *name, const char *passwd)
{
	User  *user;
	char   path[256];
	struct stat st;

	user = (User *)malloc(sizeof(User));
	strcpy(user->name, name);
	if (passwd)
		strcpy(user->passwd, passwd);
	user->fd = -1;
	user->max_friend = MAX_FRIENDS;
	user->offm_nr = 0;
	list_add(&user->reg_list, &register_head);
	INIT_LIST_HEAD(&user->contact_h);
	INIT_LIST_HEAD(&user->friend_h);
	INIT_LIST_HEAD(&user->offmsg_h);
	INIT_LIST_HEAD(&user->wait_h);

	strcpy(path, USER_PATH);
	strcat(path, name);
	strcat(path, "/");
	if (passwd) {
		if (stat(path, &st) < 0)
			mkdir(path, S_IRWXU);

		strcat(path, "profile.xml");
		user->xml = create_user_xml(path, name, passwd);
	}
	else {
		XmlElement *root;

		user->xml = xml_create(100);

		strcat(path, "profile.xml");

		if (xml_load(user->xml, path) < 0) {
			xml_release(user->xml);
			return;
		}

		root = xml_get_root_node(user->xml);
		xml_get_val_by_node_name(user->xml, root, "password", user->passwd, MAX_PASSWD);
	}
}

static void load_user(void)
{
	DIR *dir;
	struct dirent *ent;

	if (!(dir = opendir(USER_PATH)))
		return;

	while ((ent = readdir(dir))) {
		if (ent->d_name[0] == '.')
			continue;
		if (ent->d_type != DT_DIR)
			continue;
		add_user(ent->d_name, NULL);
	}

	closedir(dir);

	load_friend();
}

static void* register_handler(void *data)
{
	Uint16 port = PROTREGIST;
	int registfd, clifd;
	int reply;
	RegisterInfo reg;

	if ((registfd = socket_listen(&port, NULL, 1)) < 0) {
		perror("socket_listen");
		exit(1);
	}

	while (1) {
		if ((clifd = accept(registfd, NULL, NULL)) < 0) {
			perror("transfer accept");
			continue;
		}
		if (readn(clifd, &reg, sizeof(reg)) != sizeof(reg)) {
			close(clifd);
			continue;
		}

		reply = REGSUCCESS;
		if (!find_user(reg.name))
			add_user(reg.name, reg.passwd);
		else
			reply = REGFAILED;

		writen(clifd, &reply, sizeof(reply));
		close(clifd);
	}
	return NULL;
}

static char recv_buf[MAX_BUFFER];
int main(int argc, char *const argv[])
{
	int listen_fd;
	int max;
	fd_set allset;
	unsigned short port = PORTCOM;
	pthread_t pid;

	load_user();

	signal(SIGPIPE, SIG_IGN);
	if ((listen_fd = socket_listen(&port, NULL, 1)) < 0) {
		fprintf(stderr, "listen:%s\n", strerror(errno));
		return 1;
	}
	if (!(thread_pool = create_thread_pool(100)))
		return 1;
	if (pthread_create(&pid, NULL, listen_data_handler, NULL) == -1) {
		fprintf(stderr, "failed to create listen_transfer thread\n");
		return -1;
	}
	if (pthread_create(&pid, NULL, clear_thread_handler, NULL) == -1) {
		fprintf(stderr, "failed to create listen_transfer thread\n");
		return -1;
	}
	if (pthread_create(&pid, NULL, udp_transfer_handler, NULL) == -1) {
		fprintf(stderr, "failed to create udp_transfer_handler thread\n");
		return -1;
	}
	if (pthread_create(&pid, NULL, register_handler, NULL) == -1) {
		fprintf(stderr, "failed to create register_handler thread\n");
		return -1;
	}

	max = listen_fd;
	FD_ZERO(&allset);
	FD_SET(listen_fd, &allset);
	while (1) {
		list_t *pos, *next;
		fd_set fds = allset;

		if (select(max+1, &fds, NULL, NULL, NULL) < 0) {
			perror("select");
			continue;
		}

		if (FD_ISSET(listen_fd, &fds)) {
			User *user = login_user(listen_fd, &allset);
			if (user) {
				if (user->fd > max)
					max = user->fd;
				FD_SET(user->fd, &allset);
			}
		}

		list_for_each_safe(pos, next, &online_head) {
			User *user = list_entry(pos, User, on_list);

			if (FD_ISSET(user->fd, &fds)) {
				char *p = NULL;
				Command cm;
				int i;

				if (readn(user->fd, &cm, sizeof(cm)) != sizeof(cm)) {
					FD_CLR(user->fd, &allset);
					del_online_user(user);
					clear_data_wait(user);
					send_online_msg(user, Offline);
					continue;
				}

				if (cm.len > 0) {
					if (readn(user->fd, recv_buf, cm.len) != cm.len) {
						FD_CLR(user->fd, &allset);
						del_online_user(user);
						clear_data_wait(user);
						send_online_msg(user, Offline);
						continue;
					}
					p = recv_buf;
				}

				for (i = 0; i < TABLESIZE(server_cmd); i++) {
					if (strcmp(server_cmd[i].cmd, cm.cmd) == 0) {
						server_cmd[i].handler(user, p, cm.len);
						break;
					}
				}
			}
		}

	}
}

