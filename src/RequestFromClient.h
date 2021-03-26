#ifndef REQUEST_H
#define REQUEST_H
#include<string>


int request_connect(int server_fd,int client_id, int chatroom_id);
int request_chatroom_list(int server_fd,int client_id, int chatroom_id);
int request_connect(int server_fd,int client_id, int chatroom_id);
int request_forbidden(int server_fd,int client_id, int chatroom_id,int state);
int request_forward(int server_fd,int client_id, int chatroom_id,std::string content);
int request_rename(int server_fd,int client_id, int chatroom_id,std::string content);
int request_log_out(int server_fd,int client_id, int chatroom_id);
int request_exit_room(int server_fd,int client_id, int chatroom_id);
int request_enter(int server_fd,int client_id, int chatroom_id,std::string nickname);
#endif