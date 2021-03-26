/*
客户端部分：

使用epoll作为socket管理机制。

双进程隔离用户输入，tcp消息收发。

断网，服务器崩溃等重新连接的功能，采用signal处理函数解决。
因signal处理函数的声明限制，采用文件作用域全局变量来传递部分信息。 
注意signal处理函数中，调用的函数必须异步安全。
如有更好解决思路欢迎提出！


感谢Sixzeroo朋友提供思路。
*/
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <regex>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "RequestFromClient.h"
#include "client.h"
#include "log.h"
#include "config.h"
#include "parse.h"


//作为输入缓冲区
static char message[BUFF_SIZE];
//作为发送缓冲区
static char send_msg[BUFF_SIZE];

static int client_fd;

ChatRoomClient::ChatRoomClient()
{
    _server_ip = "";
    _server_port = -1;
    _client_fd = -1;
    _epollfd = -1;
    //初始化
    _client_id=0;
    _client_cid=0;
}

void ChatRoomClient::set_server_ip(const std::string &_server_ip) {
    LOG(DEBUG)<<"Set sever ip: "<<_server_ip<<std::endl;
    ChatRoomClient::_server_ip = _server_ip;
}

void ChatRoomClient::set_server_port(int _server_port) {
    LOG(DEBUG)<<"Set sever port: "<<_server_port<<std::endl;
    ChatRoomClient::_server_port = _server_port;
}

int ChatRoomClient::connect_to_server(std::string server_ip, int port) {
    if(_client_fd != -1)
    {
        LOG(ERROR)<<"Connect to server: epoll had created"<<std::endl;
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    unsigned int ip = inet_addr(server_ip.c_str());
    server_addr.sin_addr.s_addr = ip;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        LOG(ERROR)<<"Connect to server: create socket error"<<std::endl;
        return -1;
    }

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        LOG(ERROR)<<"Connect to server: connnect to server error"<<std::endl;
        return -1;
    }
    //fd赋值
    _client_fd = sockfd;
    return 0;
}

int ChatRoomClient::set_noblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag < 0)
        flag = 0;
    int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if(ret < 0)
    {
        // LOG ERROR
        return -1;
    }
    return 0;
}

int ChatRoomClient::addfd(int epollfd, int fd, bool enable_et) {
    struct epoll_event ev;
    ev.data.fd = fd;
    // 允许读
    ev.events = EPOLLIN;
    // 设置为边缘出发模式
    if(enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    // 添加进epoll中
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    set_noblocking(fd);
    return 0;
}

std::string ChatRoomClient::get_time_str() {
    time_t tm;
    time(&tm);
    char time_string[128];
    ctime_r(&tm, time_string);
    std::string s_time_string = time_string;
    s_time_string.pop_back();
    s_time_string = "[" + s_time_string + "]";

    return s_time_string;
}

void ChatRoomClient::print(std::string msg){
    std::cout<<msg<<std::endl;
}


//传出信号
static int pipwrite;

int ChatRoomClient::work_loop() {
    if(_epollfd != -1)
    {
        LOG(ERROR)<<"Start client error: epoll created"<<std::endl;
        return -1;
    }

    int pipefd[2];
    if(pipe(pipefd) < 0)
    {
        LOG(ERROR)<<"Start client error: create pipe error"<<std::endl;
        return -1;
    }
    pipwrite=pipefd[1];

    _epollfd = epoll_create(1024);
    LOG(DEBUG)<<"Start client: create epoll : "<<_epollfd<<std::endl;
    if(_epollfd < 0)
    {
        // LOG ERROR
        return -1;
    }
    static struct epoll_event events[2];

    addfd(_epollfd, _client_fd, true);
    addfd(_epollfd, pipefd[0], true);

    bool isworking = true;

    int pid = fork();
    if(pid < 0)
    {
        LOG(ERROR)<<"Start client: fork error"<<std::endl;
        return -1;
    }
    //子进程
    else if(pid == 0)
    {
        //关读
        LOG(DEBUG)<<"Client: create child successful"<<std::endl;
        close(pipefd[0]);
        close(_client_fd);
        LOG(DEBUG)<<"Client: child process close fds successful"<<std::endl;


        while (isworking)
        {
            bzero(message, BUFF_SIZE);
            fgets(message, BUFF_SIZE, stdin);
            //处理一下空格问题
            message[strlen(message) - 1] = '\0';

            if(strncasecmp(message, EXIT_MSG, strlen(EXIT_MSG)) == 0)
            {
                LOG(INFO)<<"Client exit"<<std::endl;
                isworking = false;
            }
            if(write(pipefd[1], message, strlen(message)) < 0)
            {
                LOG(ERROR)<<"Client child process: write error"<<std::endl;
                return -1;
            }

        }
    }
    //父进程 父进程控制状态转移
    else
    {
        //关写，signal处理函数会用到就不关了。
        //close(pipefd[1]);
        LOG(DEBUG)<<"Client: parent process close pipefd[1] successful"<<std::endl;

        while (isworking)
        {
            int epoll_event_count = epoll_wait(_epollfd, events, 2, -1);
            int ret=0;

            for(int i = 0; i < epoll_event_count; i++)
            {
                if(events[i].data.fd == _client_fd)
                {
                    LOG(DEBUG)<<"Client epoll: get events from sockfd"<<std::endl;
                    Msg recv_m;
                    ret= recv_m.recv_diy(_client_fd);
                    if(ret == -1)
                    {
                        std::cout<<"Connection failed, input \"RETRY\" to reconnect server!"<<std::endl;
                        status=ORIGIN;
                        continue;
                    } 
                    
                    //std::cout<<"DEBUG content:"+recv_m.context<<std::endl;

                    switch(recv_m.code){
                        case M_WELCOME:
                            //发id和cid 检查一下
                            std::cout<<WELCOMES_WORDS<<std::endl;
                            request_connect(_client_fd,_client_id,_client_cid);
                            status=ORIGIN;
                            break;

                        case M_CLIENT_ID:
                            _client_id=std::stoi(recv_m.context);
                            std::cout<<CHECKINGROOM_WORDS<<std::endl;
                            //发查看平台聊天室的要求
                            request_chatroom_list(_client_fd,_client_id,_client_cid);

                            break;
                        case M_CHAT_LIST:
                            // 显示更新，就好，别的不用做
                            if(recv_m.context.empty()){
                                std::cout<<"No chatrooms,\nplease input \"0-[nickname]\" to create!"<<std::endl;
                            }else{
                                std::cout<<"Chatroom ids : "<<recv_m.context<<".\nplease input \"[id]-[nickname]\" to enter"<<std::endl;
                            }
                            status=ACCESS_CHATSERVER;
                            break;
                        case M_ENTER_OR_CREATE:
                            //显示一下，聊天室页面，更新状态
                            std::cout<<"Log in chatroom with id = "<<recv_m.context<<std::endl;
                            _client_cid=std::stoi(recv_m.context);
                            status=ACCESS_CHATROOM;
                            break;
                        case M_RECONN:
                            //显示一下，聊天室页面，更新状态
                            std::cout<<"Welcome come back, relog in chatroom with id = "<<recv_m.context<<std::endl;
                            status=ACCESS_CHATROOM;
                            break;

                        case M_FORBIDDEN:
                            //显示是否成功
                        {
                            int code=std::stoi(recv_m.context);
                            if(code==0){
                                std::cout<<ACCESS_WORDS<<std::endl;
                            }else{
                                std::cout<<OP_WORDS<<std::endl;
                            }
                            break;
                        }
                        case M_CNAME:
                            //显示新名称
                            std::cout<<"Your new name : "<<recv_m.context<<std::endl;
                            std::cout<<OP_WORDS<<std::endl;
                            status=ACCESS_CHATROOM;
                            break;
                        case M_NORMAL:
                            //显示聊天信息
                        {
                            std::string time_str = get_time_str();
                            std::cout<<time_str<<recv_m.context<<std::endl;
                            break;
                        }
                        case M_EXIT_ROOM:
                            //更改状态
                            _client_cid=0;
                            request_chatroom_list(_client_fd,_client_id,_client_cid);
                            std::cout<<EXIT_ROOM_WORDS<<std::endl;
                            status=ACCESS_CHATSERVER;
                            break;
                        case M_SERVER_ERR:
                            //告知作用,有时需要改变状态
                            //1 资源耗尽，2 房间不存在，可能由于群主退群，解散了。
                        {
                            int code=std::stoi(recv_m.context);
                            if(code == 1){
                                std::cout<<SERVER_ERR_WORDS<<std::endl;
                            }else{
                                std::cout<<ROOM_ERR_WORDS<<std::endl;
                            }
                            std::cout<<RETRY_WORDS<<std::endl;
                            status=FAILURE;
                            break;
                        }
                        default :
                        break;
                    }

                }
                else
                {
                    bzero(send_msg, sizeof(send_msg));
                    ret = read(events[i].data.fd, send_msg, BUFF_SIZE);
                    std::string message_str(send_msg);
                    //退出处理
                    if(strcmp(send_msg,EXIT_MSG)==0){
                        LOG(INFO)<<"Client epoll: close connetct"<<std::endl;
                        request_log_out(_client_fd,_client_id, _client_cid);
                        isworking = false;

                    }

                    if(strncmp(send_msg,SIG_CMD,strlen(SIG_CMD))==0){
                        //close(_client_fd);
                        status=ORIGIN;
                        std::cout<<send_msg<<std::endl;
                    }

                    if(!isworking) continue;

                    switch (status)
                    {
                    case ACCESS_CHATSERVER:
                        //cid-nickname,cid-list
                        // 0 创建,指定cid 加入,list
                    {
                        if(strcmp(send_msg,LIST_ROOM_MSG)==0)
                        {
                            request_chatroom_list(_client_fd,_client_id,_client_cid);
                        }
                        else{
                            //选择，加入，还是创建
                            //解析message,构建发送包

                            //格式是：cid-nickname
                            std::regex r("(\\d*)-(.*)");
                            std::smatch m;
                            
                            bool found= std::regex_search(message_str,m,r);
                            if(!found){
                                std::cerr << ENTER_WORDS << std::endl;
                                break;
                            }

                            try
                            {
                                int cid=std::stoi(m.str(1));
                                std::string nickname=m.str(2);
                                request_enter(_client_fd,_client_id,cid,nickname);
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << ENTER_WORDS << std::endl;
                            }
                        }
                        
                        
                        break;
                    }
                    case ACCESS_CHATROOM:
                        //解析message,改名，开关门，还是发消息,如果失败，提示一下
                        if(strcmp(send_msg,RENAME_MSG)==0){
                            status=RENAME;

                        }else if(strcmp(send_msg,OPEN_DOOR_MSG)==0){
                            request_forbidden(_client_fd,_client_id,_client_cid,1);
                        }else if(strcmp(send_msg,CLOSE_DOOR_MSG)==0){
                            request_forbidden(_client_fd,_client_id,_client_cid,0);
                        }else if(strcmp(send_msg,EXIT_ROOM_MSG)==0){
                            request_exit_room(_client_fd,_client_id,_client_cid);
                        }else{
                            request_forward(_client_fd,_client_id,_client_cid,message_str);
                        }

                        break;

                    case RENAME:
                        request_rename(_client_fd,_client_id,_client_cid,send_msg);
                        break;
                    case FAILURE:
                        
                        if(strcmp(send_msg,RETRY_MSG)==0)
                        {
                            std::cout<<"Retry connecting"<<std::endl;
                            ret=request_connect(_client_fd,_client_id,_client_cid);
                        }

                        break;

                    case ORIGIN:
                        if(strcmp(send_msg,RETRY_MSG)==0)
                        {
                            //reconnection,fd
                            close(_client_fd);
                            _client_fd=-1;
                            if(connect_to_server(_server_ip, _server_port) < 0)
                            {
                                std::cout<<"Client start: connect to server error"<<std::endl;
                                
                                break;
                            }
                            addfd(_epollfd, _client_fd, true);
                            break;
                        }
                    default:

                        break;
                    }
                }
            }
        }
    }

    if(pid)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        close(_epollfd);
    }
    else
    {
        close(pipefd[1]);
    }

    return 0;
}

int ChatRoomClient::start_client(std::string ip, int port) {
    set_server_port(port);

    set_server_ip(ip);

    if(connect_to_server(_server_ip, _server_port) < 0)
    {
        LOG(ERROR)<<"Client start: connect to server error"<<std::endl;
        return -1;
    }

    return work_loop();
}

//通知关闭fd，转入origin状态。
static char sig_buffer[100]="SIGNAL RECEIVED, connection interrupted, try \"RETRY\".";
static void handle_client(int sig){ 
    write(pipwrite,sig_buffer,strlen(sig_buffer));
}


int main()
{
    std::map<std::string, std::string> config;
    get_config_map("client.config", config);
    init_logger("client_log", "debug.log", "info.log", "warn.log", "error.log", "all.log");
    set_logger_mode(1);
    signal(SIGPIPE,handle_client);
    ChatRoomClient client;
    client.start_client(config["ip"], std::stoi(config["port"]));
    return 0;
}