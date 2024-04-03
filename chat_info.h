#ifndef __CHAT_INFO_H__
#define __CHAT_INFO_H__

typedef struct chatInfo
{
    char userID[20];
    char message[40];
} CHAT_INFO;

typedef struct whisperInfo
{
    int chatFlag;
    char participant1[20];
    char participant2[20];
    CHAT_INFO chats[10];
} WHISPER_INFO;

typedef struct roomInfo
{
    int chatFlag;
    int userCnt;
    char userIDs[3][20];
    CHAT_INFO chats[10];
} ROOM_INFO;

#endif