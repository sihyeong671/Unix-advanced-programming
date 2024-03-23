#ifndef __CSECHAT_H__
#define __CSECHAT_H__

typedef struct chatInfo
{
    char userID[20];
    char message[40];
} CHAT_INFO;

typedef struct roomInfo
{
    char userIDs[3][20];
    int userCnt;
} ROOM_INFO;

#endif