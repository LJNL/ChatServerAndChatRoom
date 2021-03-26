
#include <regex>
#include <signal.h>
#include <unistd.h>

#include "server.h"
#include "parse.h"
#include "config.h"
#include "log.h"
#include "RespondFromServer.h"


ChatRoom::ChatRoom(UserInfo owner,int chatroom_id){
    _is_live=true;
    _client_ids.clear();
    _owner_id=owner.user_id;
    _forbidden=false;
    _chatroom_id=chatroom_id;
    add_client(owner);
}
bool ChatRoom::add_client(UserInfo client){

    if( _client_ids.count(client.user_id)==1){
        return false;
    }

    if(_forbidden){
        return false;
    }
    _client_ids.insert(std::make_pair(client.user_id,client));
    return true;
}
bool ChatRoom::remove_client(int client_id){
    if(client_id == _owner_id)
    {
        _is_live=false;
    }
    if(_client_ids.count(client_id)==0)
    {
        return false;
    }
     _client_ids.erase(client_id);
     return true;
}

bool ChatRoom::modify_nickname(int client_id,std::string new_name){

    if(_client_ids.count(client_id)==0)
    {
        return false;
    }

    _client_ids[client_id].nick_name=new_name;
    return true;
}

bool ChatRoom::update_fd(int client_id,int client_fd){
    if(_client_ids.count(client_id)==0)
    {
        return false;
    }

    _client_ids[client_id].user_fd=client_fd;
    return true;
}

bool ChatRoom::request_forbidden(int client_id,int status){
    if(client_id == _owner_id){
        if(status==0)
            _forbidden=true;
        else
            _forbidden=false;
        return true;
    }
    return false;
}

int ChatRoom::get_room_id(){
    return _chatroom_id;
}

int ChatRoom::get_owner_id(){
    return _owner_id;
}

bool ChatRoom::get_status(){
    return _is_live;
}

bool ChatRoom::contains(int client_id){
    return _client_ids.count(client_id)==1;
}

ChatRoomServer::ChatRoomServer() {
    _socket_epoll.set_watcher(this);
}

static int current_fd;
// 处理接受请求逻辑
int ChatRoomServer::on_accept(EpollContext &epoll_context) {
    LOG(DEBUG)<<"Handle: on_accept start"<<std::endl;
    int client_fd = epoll_context.fd;
    current_fd=client_fd;

    printf("client %s:%d connected to server\n", epoll_context.client_ip.c_str(), epoll_context.client_port);

    // 返回欢迎信息
    int ret=respond_connection(client_fd);
    return ret;
}

// 处理来信请求, 出错返回-1， 退出连接返回-2
int ChatRoomServer::on_readable(EpollContext &epoll_context, std::unordered_map<int, std::string> &client_list) {
    int client_fd = epoll_context.fd;
    set_client_fd(client_fd);
    current_fd=client_fd;
    Msg recv_m;
    recv_m.recv_diy(client_fd);
    int ret=0;

    /*
    通信内容格式规定：
    client_id=[id];chatroom_id=[id];content=[x];
    */

    std::regex r("client_id=(\\d*);chatroom_id=(\\d*);content=(.*);");
    std::smatch m;
    bool found= std::regex_search(recv_m.context,m,r);
    if(!found){
        //ret=respond_failure(client_fd); 
        printf("client %s:%d connecting to server\n", epoll_context.client_ip.c_str(), epoll_context.client_port);
        std::cout<<"Client fd: "<<client_fd<<std::endl;
        std::cout<<"Client MSG FORMAT ERROR."<<std::endl;
        close(client_fd);
        return ret;
        }
    int client_id=std::stoi(m.str(1));
    int chatroom_id=std::stoi(m.str(2));
    std::string content=m.str(3);


    switch (recv_m.code)
    {
    case M_WELCOME:
        //开始连接（包括初连和重连）
    {
        if(client_id == 0)
        {
            //出连处理
            client_id=unused_client_id();
            if(client_id==0) 
            {
                ret=respond_failure(client_fd);
                break;
            }
            ret=respond_allocation(client_fd,client_id);
            if(ret!= -1)
            {
                set_client_id(client_id);
            }
        }else{
            //检查id和chatroom是否同时存在
            if(is_chatroom_ids_used(chatroom_id)){
                if(_chatroom_ids[chatroom_id].contains(client_id)){
                    ret=respond_reconnection(client_fd,chatroom_id);
                    if(ret!=-1)
                    {
                        //更新fd
                        _chatroom_ids[chatroom_id].update_fd(client_id,client_fd);
                    }
                }
            }else{
                reset_client_id(client_id);
                client_id=unused_client_id();
                if(client_id==0){
                    ret=respond_failure(client_fd);
                    break;
                } 
                ret=respond_allocation(client_fd,client_id);
                if(ret!= -1)
                {
                    set_client_id(client_id);
                }
            }
        }
        break;
    }

    case M_CHAT_LIST:
    {
        //id 检查略过
        std::vector<int> cids=show_chat_room();
        ret=respond_chatroom_list(client_fd,cids);
        break;

    }
    case M_ENTER_OR_CREATE:
    {
        //id,cid,nick_name
        UserInfo client{client_id,client_fd,content};

        if(chatroom_id == 0){
            //创建
            chatroom_id=create_chatroom(client);
            if(chatroom_id==0){
                ret=respond_failure(client_fd);
                break;
            }else{
                ret=respond_create_or_enter_chatroom(client_fd,chatroom_id);
            }
        }else{
            //进入
            if(is_chatroom_ids_used(chatroom_id)){

                bool suc=_chatroom_ids[chatroom_id].add_client(client);
                if(!suc){
                    //关门后
                    ret=respond_failure(client_fd,2);
                }else{
                    ret=respond_create_or_enter_chatroom(client_fd,chatroom_id);
                }
            }else{
                //进入失败
                ret=respond_failure(client_fd,2);
            }
        }
        break;
    }
    //室内操作，包括开关门，聊天，改名，退出
    case M_FORBIDDEN:
    {
        //0关门，1开门
        int forb= std::stoi(content);
        if(is_chatroom_ids_used(chatroom_id) && _chatroom_ids[chatroom_id].contains(client_id)){
            bool suc=_chatroom_ids[chatroom_id].request_forbidden(client_id,forb);
                ret=respond_forbidden(client_fd,suc,chatroom_id);
        }else{
            ret=respond_failure(client_fd,2);
        }
        break;
    }
    case M_NORMAL:
        //检测群主是否解散群
    {
        if(is_chatroom_ids_used(chatroom_id) && _chatroom_ids[chatroom_id].contains(client_id)){
            auto users=_chatroom_ids[chatroom_id].get_users();
            content = _chatroom_ids[chatroom_id].get_nickname(client_id)+">"+content;
            for(auto it: users){
                respond_forward(it.second.user_fd,content);
            }
        }else{
            ret=respond_failure(client_fd,2);
        }
        break;
    }
    case M_CNAME:
    {
        //检测群主是否解散群
        if(is_chatroom_ids_used(chatroom_id) && _chatroom_ids[chatroom_id].contains(client_id)){
            _chatroom_ids[chatroom_id].modify_nickname(client_id,content);
            ret=respond_change_name(client_fd,content);
        }else{
            ret=respond_failure(client_fd,2);
        }
        break;
    }
    case M_EXIT_ROOM:
    {
        if(is_chatroom_ids_used(chatroom_id) && _chatroom_ids[chatroom_id].contains(client_id))
        {
            int cid=chatroom_id;
            if(cid != 0){
                if(_chatroom_ids[cid].get_owner_id()!=client_id){
                    _chatroom_ids[cid].remove_client(client_id);
                }else{
                    del_chatroom(cid);

                }
            }
            ret=respond_exit(client_fd);
        }else{
            ret=respond_failure(client_fd,2);
        }
        break;
    }
    case M_EXIT:
    {
        //不用回复，只进行资源处理
        //room的解散，chatserver标记清除
        //room

        printf("client %s:%d exited from server\n", epoll_context.client_ip.c_str(), epoll_context.client_port);
        
        int cid=get_user_cid(client_id);
        if(cid != 0){
            if(_chatroom_ids[cid].get_owner_id()!=client_id){
                _chatroom_ids[cid].remove_client(client_id);
            }else{
                del_chatroom(cid);
            }
        }
        //chatserver
        reset_client_id(client_id);
        reset_client_fd(client_fd);
        close(client_fd);
        break;
    }
    default:
        break;
    }

    return ret;
}

int ChatRoomServer::start_server(const std::string bind_ip, int port, int backlog, int max_events) {
    LOG(INFO)<<"Server start: start"<<std::endl;
    std::cout<<"ChatRoom start!"<<std::endl;
    _socket_epoll.set_bind_ip(bind_ip);
    _socket_epoll.set_port(port);
    _socket_epoll.set_backlog(backlog);
    _socket_epoll.set_max_events(max_events);
    return _socket_epoll.start_epoll();
}

int ChatRoomServer::stop_server() {
    LOG(INFO)<<"Server: stop server"<<std::endl;
    return _socket_epoll.stop_epoll();
}


static void handle_server(int sig){
    close(current_fd);
}

int main()
{
    // 设置配置
    std::map<std::string, std::string> config;
    get_config_map("server.config", config);
    init_logger("server_log", "debug.log", "info.log", "warn.log", "error.log", "all.log");
    set_logger_mode(1);
    signal(SIGPIPE,handle_server);
    ChatRoomServer server;
    server.start_server(config["ip"], std::stoi(config["port"]), 20, 200);
    return 0;
}