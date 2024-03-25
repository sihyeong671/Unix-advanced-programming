#ifndef __CSECHAT_H__
#define __CSECHAT_H__

typedef struct chatInfo
{
    char userID[20];
    char message[40];
} CHAT_INFO;

typedef struct roomInfo
{
    int chatFlag;
    int userCnt;
    char userIDs[3][20];
    CHAT_INFO chats[10];
} ROOM_INFO;

#endif