/*
本文件用于发送请求通用格式为：
code: 状态标志，参考parse.h
context: 通用格式如下
client_id=[];chatroom_id=[];content=[];
*/

#include "parse.h"

int request_connect(int server_fd,int client_id, int chatroom_id){
    Msg m;
    m.code = M_WELCOME;
    //client_id=0,chatroom_id=0;
    //
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content=;";
    int ret = m.send_diy(server_fd);
    return ret;
}

int request_chatroom_list(int server_fd,int client_id, int chatroom_id){
     Msg m;
    m.code = M_CHAT_LIST;
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content=;";
    int ret = m.send_diy(server_fd);
    return ret;
}

int request_enter(int server_fd,int client_id, int chatroom_id,std::string nickname){
     Msg m;
    m.code = M_ENTER_OR_CREATE;
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content="+nickname+";";
    int ret = m.send_diy(server_fd);
    return ret;
}

int request_forbidden(int server_fd,int client_id, int chatroom_id,int state){
     Msg m;
    m.code = M_FORBIDDEN;
    std::string stat=std::to_string(state);
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content="+stat+";";
    int ret = m.send_diy(server_fd);
    return ret;
}


int request_forward(int server_fd,int client_id, int chatroom_id,std::string content){
     Msg m;
    m.code = M_NORMAL;
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content="+content+";";
    int ret = m.send_diy(server_fd);
    return ret;
}

int request_rename(int server_fd,int client_id, int chatroom_id,std::string content){
    Msg m;
    m.code = M_CNAME;
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content="+content+";";
    int ret = m.send_diy(server_fd);
    return ret;
}

int request_exit_room(int server_fd,int client_id, int chatroom_id){
     Msg m;
    m.code = M_EXIT_ROOM;
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content=;";
    int ret = m.send_diy(server_fd);
    return ret;
}

int request_log_out(int server_fd,int client_id, int chatroom_id){
     Msg m;
    m.code = M_EXIT;
    m.context = "client_id="+std::to_string(client_id)+";"+"chatroom_id="+std::to_string(chatroom_id)+";"+"content=;";
    int ret = m.send_diy(server_fd);
    return ret;
}