#ifndef CHARROOM_PARSE_H
#define CHARROOM_PARSE_H

#include <string>

#define MSG_CHAR_SIZE 4000
#define BUFF_SIZE 4096
#define BAD "0"

enum MsgType {
    M_WELCOME= 1,
    M_NORMAL = 2,
    M_EXIT = 3,
    M_CNAME = 4,
    M_RECONN = 5,
    M_CLIENT_ID = 6,
    M_ENTER_OR_CREATE = 7,
    M_FORBIDDEN = 8,
    M_CHAT_LIST = 9,
    M_SERVER_ERR= 10,
    M_ROOM_ERR= 11,
    M_EXIT_ROOM=12
};

// 消息格式
struct MsgPacket {
    // 消息类型
    int code;
    char context[MSG_CHAR_SIZE];
};

class Msg {
public:
    Msg() = default;

    Msg(int code, std::string context);

    int code;
    std::string context;

    int send_diy(int fd);

    int recv_diy(int fd);
};

#endif //CHARROOM_PARSE_H
