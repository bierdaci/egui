#ifndef __COMMON_H
#define __COMMON_H

#define MAX_PASSWD		20
#define MAX_USERNAME	20
#define MAX_COMMAND		20
#define MAX_MESSAGE		200
#define MAX_FILENAME	100
#define MAX_PATH		256
#define MAX_BUFFER		1024
#define MAX_CRED		(MAX_USERNAME + sizeof(struct timeval))
#define MAX_IPSTR		24

#define USERNOEXIST		0
#define LOGIONSUCCESS	1
#define PASSWDERR		2

#define REGSUCCESS		3
#define REGFAILED		4

#define CREDYES			0
#define CREDNO			1

#define PORTCOM			9000
#define PORTFILE		9001
#define PROTUDP			9002
#define PROTREGIST		9003

#define SYSUSER			"system"

#define REALMESSAGE		"realmessage"
#define REQUESTADD		"requestadd"
#define REFUSEADD		"refuseadd"
#define ACCEPTADD		"acceptadd"
#define ACCEPTANDADD	"acceptandadd"
#define DELFRIEND		"delfriend"
#define SEARCHUSER		"searchuser"
#define REPLYSEARCH		"replysearch"
#define SEARCHFRIEND	"searchfriend"
#define REPLYFRIEND		"replyfriend"
#define SEARCHWHO		"searchwho"
#define REPLYWHO		"replywho"
#define NOTIFYON		"notifyon"
#define NOTIFYOFF		"notifyoff"
#define REQTRANS		"reqtrans"
#define ACCEPTTRANS		"accepttrans"
#define STARTTRANS		"readytrans"
#define CANCELTRANS		"canceltrans"
#define REFUSETRANS		"refusetrans"
#define UREADYTRANS		"ureadytrans"

#define USER_STATUS_ONLINE		(0x1 << 0)
#define USER_STATUS_HIDE		(0x1 << 1)

typedef struct {
	char cmd[MAX_COMMAND];
	int len;
} Command;

typedef struct {
	struct timeval time;
	char name[MAX_USERNAME];
} Cred;

typedef enum {DataTypeFile, DataTypeAudio, DataTypeFileU, DataTypeAudioU} DataType;
typedef struct {
	DataType type;
	Cred	 cred;
	char	 name[MAX_USERNAME];
	char	 file[MAX_FILENAME];
	char	 ipstr[MAX_IPSTR];
	unsigned int size;
} DataInfoHead;

typedef struct {
	char name[MAX_USERNAME];
	char msg[0];
} RealMessage;

typedef struct {
	char name[MAX_USERNAME];
	char passwd[MAX_PASSWD];
} LoginInfo;

typedef struct {
	char name[MAX_USERNAME];
	char passwd[MAX_PASSWD];
} RegisterInfo;

#define TABLESIZE(table)	(sizeof(table) / sizeof(table[0]))

#endif
