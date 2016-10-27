#ifndef __REALMSG_H
#define __REALMSG_H

int   client_login(const char *name, const char *passwd);
int   client_search_user(int sfd, char *name);
int   client_read_thread(int sfd);
int   client_search_friend(int);
int   client_send_realmsg(int sfd, char *name, char *msg);
int   client_add_friend(int sfd, char *name);
int   client_send_del_friend(int sfd, char *name);
int   client_accept_add_friend(int sfd, char *name, const char *cmd);
int   client_send_file(int sfd, char *name, char *file, int *, void *);
int   client_accept_transfer(int sfd, DataInfoHead *dh, const char *path);
int   ui_accept_add_dlg(char *name);
int   ui_notify_add_dlg(char *name);
int   ui_add_user(const char *name, ebool);
void  ui_set_transfer_size(void *cred, int size);
void  ui_recv_realmsg(const char *name, const char *msg);
void  ui_change_online(const char *name, ebool online);
void  ui_request_file_notify(DataInfoHead *);
void *ui_friend_to_clist(const char *, ebool, ebool);

#endif
