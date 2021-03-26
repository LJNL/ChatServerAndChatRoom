#define WELCOM_MES "Welcome to this chatServer! "
#define SUCCESS_MES "SUCCESS"
#define FAILURE_MES "1"
#define FAILURE_NO_ROOM "2"

#include "parse.h"
#include<vector>
#include<sstream>
#include <algorithm>
#include <iostream>

int respond_connection(int client_fd){
    Msg m;
    m.code = M_WELCOME;
    m.context = WELCOM_MES;
    //m.context = m.context+ "\nYou nickname is " + std::to_string(client_fd) + " (Enter \"r\" ro replace).";
    int ret = m.send_diy(client_fd);
    return ret;
}

int respond_reconnection(int client_fd,int chatroom_id){
    Msg m;
    m.code = M_RECONN;
    m.context = std::to_string(chatroom_id);
    int ret = m.send_diy(client_fd);
    return ret;
}

int respond_allocation(int client_fd,int client_id){
    Msg m;
    m.code = M_CLIENT_ID;
    m.context = std::to_string(client_id);
    int ret = m.send_diy(client_fd);
    return ret;
}

int respond_create_or_enter_chatroom(int client_fd,  int chatroom_id){
    Msg m;
    m.code = M_ENTER_OR_CREATE;
    int ret;
    m.context = std::to_string(chatroom_id);
    ret = m.send_diy(client_fd);
    return ret;
}

int respond_change_name(int client_fd, std::string name){
    Msg m;
    m.code = M_CNAME;
    m.context = name;
    int ret = m.send_diy(client_fd);
    return ret;
}

int respond_forbidden(int client_fd, bool success, int chatroom_id){
    Msg m;
    m.code = M_FORBIDDEN;
    int ret;
    if(success)
    {
        m.context = std::to_string(chatroom_id);
        ret = m.send_diy(client_fd);
    }else{
        m.context = BAD;
        ret = m.send_diy(client_fd);
    }

    return ret;
}

int respond_chatroom_list(int client_fd, std::vector<int> id_list){
    Msg m;
    m.code = M_CHAT_LIST;
    int ret;
    std::stringstream ss;
    ss.clear();  
    std::for_each(id_list.begin(),id_list.end(),
    [&ss,sep=' '](int x)mutable{
        ss<<sep<<x;
        sep=',';
    });

    ss>>m.context;
    ret = m.send_diy(client_fd);
    return ret;
}

void respond_forward(int client_fd,std::string context){
    Msg send_m(M_NORMAL, context);
    send_m.send_diy(client_fd);
}

int respond_exit(int client_fd){

    Msg m;
    m.code = M_EXIT_ROOM;
    m.context = SUCCESS_MES;
    int ret = m.send_diy(client_fd);
    return ret;

}

int respond_log_out(int client_fd){

    Msg m;
    m.code = M_EXIT;
    m.context = SUCCESS_MES;
    int ret = m.send_diy(client_fd);
    return ret;

}




int respond_failure(int client_fd,int code){
    Msg m;

    switch(code){
        case 1:
            m.context = FAILURE_MES;
        break;
        case 2:
            m.context = FAILURE_NO_ROOM;
        break;

        default:
            m.context = FAILURE_MES;
    }
    m.code = M_SERVER_ERR;
    int ret = m.send_diy(client_fd);
    return ret;
}