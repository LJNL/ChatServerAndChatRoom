#ifndef CHARROOM_CLIENT_H
#define CHARROOM_CLIENT_H

#include <string>

//COMMAND:

//退出命令，第一个为程序退出，第二为房间退出，注意群主退出会解散房间。
#define EXIT_MSG "LOG_OUT"
#define EXIT_ROOM_MSG "EXIT_ROOM"
//查找房间号
#define LIST_ROOM_MSG "LIST"
//开关门，只有群主有权力
#define OPEN_DOOR_MSG "OPEN"
#define CLOSE_DOOR_MSG "CLOSE"
//改名
#define RENAME_MSG "RENAME"
//在失败后，或断网后，可输入重连
#define RETRY_MSG "RETRY"

#define SIG_CMD "SIGNAL"

//提示
#define WELCOMES_WORDS "Welcome to the chat server!"
#define CHECKINGROOM_WORDS "Checking chatrooms!"
#define ENTER_WORDS "Illegal input,try [chatroom id]-[nickname] or 0-[nickname]"

#define EXIT_ROOM_WORDS "Byes"
#define SERVER_ERR_WORDS "Server resources exhausted!"
#define ROOM_ERR_WORDS "Room does not exist!"
#define RETRY_WORDS "Please input \"RETRY\"."
#define OP_WORDS "OP success."
#define ACCESS_WORDS "You do not have right to control the room!"

enum CSTATUS {
    ORIGIN,
    FAILURE,
    ACCESS_CHATSERVER,
    ACCESS_CHATROOM,
    RENAME
};


class ChatRoomClient {
private:
    std::string _server_ip;
    int _server_port;

    int _client_id;
    int _client_cid;
    int _client_fd;
    int _epollfd;

    CSTATUS status=ORIGIN;


public:

    ChatRoomClient();

    ~ChatRoomClient() = default;

    void set_server_ip(const std::string &_server_ip);

    void set_server_port(int _server_port);

    int connect_to_server(std::string server_ip, int port);

    int set_noblocking(int fd);

    int addfd(int epollfd, int fd, bool enable_et);

    int work_loop();

    int start_client(std::string ip, int port);

    std::string get_time_str();

    void print(std::string msg);
};

#endif //CHARROOM_CLIENT_H
