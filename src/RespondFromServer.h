#ifndef RESPOND_H
#define RESPOND_H

#include<vector>
#include<string>



int respond_connection(int client_fd);
int respond_reconnection(int client_fd,int chatroom_id);
int respond_allocation(int client_fd,int client_id);
int respond_create_or_enter_chatroom(int client_fd, int chatroom_id);
int respond_change_name(int client_fd, std::string name);
int respond_forbidden(int client_fd, bool success, int chatroom_id);
int respond_chatroom_list(int client_fd, std::vector<int> id_list);
void respond_forward(int client_fd,std::string context);
int respond_exit(int client_fd);
int respond_log_out(int client_fd);
//1 服务器资源耗尽 2 房间信息错误
int respond_failure(int client_fd,int code = 1);

#endif