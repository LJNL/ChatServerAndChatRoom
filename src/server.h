#ifndef CHARROOM_SERVER_H
#define CHARROOM_SERVER_H

#include "socket_epoll.h"
#include <utility>
#include <vector>
#define MAX_CLIENT 1000
#define MAX_CHATROOM 1000


struct UserInfo{
    //标识身份
    int user_id;
    //连接身份
    int user_fd;
    //显示身份
    std::string nick_name;
};

class ChatRoom{

private:
    //聊天室用户信息
    std::unordered_map<int,UserInfo> _client_ids;
    //聊天室id
    int _chatroom_id;
    bool _forbidden;
    int _owner_id;
    bool _is_live;

public:

    ChatRoom(UserInfo owner,int chatroom_id);
    ChatRoom()=default;
    bool add_client(UserInfo client);
    bool remove_client(int client_id);
    bool modify_nickname(int client_id,std::string new_name);
    bool update_fd(int client_id,int client_fd);
    bool request_forbidden(int client_id,int forb);
    int get_room_id();
    int get_owner_id();
    bool get_status();
    std::unordered_map<int,UserInfo>& get_users(){
        return _client_ids;
    }
    bool contains(int client_id);
    std::string get_nickname(int client_id){
        return _client_ids[client_id].nick_name;
    }
    bool is_forbidden(){
        return _forbidden;
    }

};

//聊天室管理平台
class ChatRoomServer: public SocketEpollWatcher{
private:
    //通信相关
    SocketEpoll _socket_epoll;
    //身份id记录
    std::unordered_map<int,ChatRoom> _chatroom_ids;
    
    //身份id标记
    bool client_ids[MAX_CLIENT]={false};
    bool chatroom_ids[MAX_CHATROOM]={false};
    //连接id标记
    bool client_fds[MAX_CLIENT]={false};

    //找一个未分配的id,返回0则失败
    int unused_client_id(){
        for(int i=1;i<MAX_CLIENT;i++){
            if(client_ids[i]==false)
            {
                client_ids[i]=true;
                return i;
            }
        }
        return 0;
    }

    //找一个未分配的id,返回0则失败
    int unused_chatroom_id(){
        for(int i=1;i<MAX_CHATROOM;i++){
            if(chatroom_ids[i]==false)
            {
                chatroom_ids[i]=true;
                return i;
            }
        }
        return 0;
    }

    int create_chatroom(UserInfo user){
        int chat_id=unused_chatroom_id();
        if(chat_id==0){
            return 0;
        }
        set_chatroom_id(chat_id);
        ChatRoom cr(user,chat_id);
        _chatroom_ids.insert(std::make_pair(chat_id,cr));
        return chat_id;
    }


    bool del_chatroom(int cid){
        _chatroom_ids.erase(cid);
        reset_chatroom_id(cid);
        return true;
    }


    //检查id是否使用，无越界检查
    bool is_client_ids_used(int index){
        return client_ids[index];
    }
    bool is_chatroom_ids_used(int index){
        return chatroom_ids[index];
    }
    bool is_client_fds_used(int index){
        return client_fds[index];
    }

    //重置id，无越界检查
    void reset_client_id(int index){
        client_ids[index]=false;
    }
    void reset_chatroom_id(int index){
        chatroom_ids[index]=false;
    }
    void reset_client_fd(int index){
        client_fds[index]=false;
    }


    //占用id，无越界检查
    void set_client_id(int index){
        client_ids[index]=true;
    }
    void set_chatroom_id(int index){
        chatroom_ids[index]=true;
    }
    void set_client_fd(int index){
        client_fds[index]=true;
    }

    int get_user_cid(int client_id){
        for(auto i : _chatroom_ids){
            bool res=i.second.contains(client_id);
            if(res){
                return i.second.get_room_id();
            }
        }
        return 0;
    }


public:
    ChatRoomServer();

    std::vector<int> show_chat_room(){
        std::vector<int> id_list;
        for(auto kv: _chatroom_ids){
            id_list.push_back(kv.first);
        }
        return id_list;
    };

    int start_server(const std::string bind_ip, int port, int backlog, int max_events);

    int stop_server();

    virtual int on_accept(EpollContext &epoll_context);
    virtual int on_readable(EpollContext &epoll_context, std::unordered_map<int, std::string> &client_list);
};




#endif //CHARROOM_SERVER_H
