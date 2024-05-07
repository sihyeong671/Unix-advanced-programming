#ifndef __CHAT_INFO_H__
#define __CHAT_INFO_H__

#define MAX_CAPACITY 100


typedef struct chatInfo
{
    char senderID[20];
    char receiverID[20];
    char message[40];
} CHAT_INFO;

typedef struct roomInfo
{
    int chatFlag;
    int userCnt;
    char userIDs[3][20];
    CHAT_INFO chats[MAX_CAPACITY];
} ROOM_INFO;

#endif