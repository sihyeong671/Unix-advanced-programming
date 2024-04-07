#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <ncurses.h>

#include "chat_info.h"

#define MAX_LEN 100

int getKey(char *file_path);
void initWindow();
void setShmAddr(int key, int size, void **shmAddr);
void chatRead();
void chatWrite();

WINDOW *OutputWnd, *InputWnd, *UserWnd, *AppWnd;

bool quit;
int chatShmId, roomShmId;
int shmbufindex, readmsgcount;
CHAT_INFO *chatInfo = NULL;
void *roomShmAddr = (void *)0;
int roomKey;
char userID[20];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char inputStr[40];
char allMsg[40];
char whisperMsg[40];
char receiverID[20];
char *pch;

void initWindow()
{
    initscr();

    OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    InputWnd = subwin(stdscr, 3, 42, 13, 0);
    UserWnd = subwin(stdscr, 12, 20, 0, 43);
    AppWnd = subwin(stdscr, 3, 20, 13, 43);

    wtimeout(InputWnd, 3000);

    box(OutputWnd, 0, 0);
    box(InputWnd, 0, 0);
    box(UserWnd, 0, 0);
    box(AppWnd, 0, 0);

    mvwprintw(OutputWnd, 0, 2, "Output");
    mvwprintw(InputWnd, 0, 2, "Input");
    mvwprintw(UserWnd, 0, 2, "User");
    mvwprintw(AppWnd, 0, 2, "App");
    mvwprintw(AppWnd, 1, 3, "Unix Chat App");
    refresh();
}

int getKey(char *file_path)
{
    FILE *fs;
    fs = fopen(file_path, "r");
    char str[MAX_LEN];
    fgets(str, MAX_LEN, fs);
    int num = atoi(str);
    return num;
}

void setShmAddr(int key, int size, void **shmAddr)
{
    int shmId = shmget((key_t)key, size, 0666 | IPC_CREAT | IPC_EXCL);

    if (shmId < 0)
    {
        shmId = shmget((key_t)key, size, 0666);
        *shmAddr = shmat(shmId, (void *)0, 0666);
        if (*shmAddr < 0)
        {
            perror("shmat attach is failed : ");
            exit(0);
        }
    }
    else
    {
        *shmAddr = shmat(shmId, (void *)0, 0666);
    }
}

void login()
{
    int currentUserCnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
    if (currentUserCnt < 3)
    {
        memcpy(((ROOM_INFO *)roomShmAddr)->userIDs[currentUserCnt], userID, sizeof(userID));
        ((ROOM_INFO *)roomShmAddr)->userCnt++;
    }
    else
    {
        perror("too much users in room");
        exit(0);
    }
}

void logout()
{
    int index = 0;
    for (int i = 0; i < 3; i++)
    {
        if (!strcmp(((ROOM_INFO *)roomShmAddr)->userIDs[i], userID))
        {
            index = i;
        }
    }
    for (int i = index; i < 3; i++)
    {
        memcpy(((ROOM_INFO *)roomShmAddr)->userIDs + i, ((ROOM_INFO *)roomShmAddr)->userIDs + (i + 1), 20 * sizeof(char));
    }
    ((ROOM_INFO *)roomShmAddr)->userCnt--;
}

void chatRead()
{
    while (!quit)
    {
        pthread_mutex_lock(&mutex);

        wclear(OutputWnd);
        box(OutputWnd, 0, 0);
        mvwprintw(OutputWnd, 0, 2, "Output");

        int line = 10;
        for (int i = MAX_CAPACITY - 1; i >= 0; i--)
        {
            bool msgEmpty = !(strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].message, ""));
            if (!msgEmpty)
            {
                bool isAllMsg = !strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].receiverID, "ALL");
                bool sendWhisper = !strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].senderID, userID);
                bool receiveWhisper = !strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].receiverID, userID);
                if (isAllMsg)
                {
                    mvwprintw(
                        OutputWnd,
                        line--,
                        1,
                        "[%s]: %s",
                        ((ROOM_INFO *)roomShmAddr)->chats[i].senderID,
                        ((ROOM_INFO *)roomShmAddr)->chats[i].message);
                }
                else if (sendWhisper || receiveWhisper)
                {
                    mvwprintw(
                        OutputWnd,
                        line--,
                        1,
                        "[%s] >> [%s]: %s",
                        ((ROOM_INFO *)roomShmAddr)->chats[i].senderID,
                        ((ROOM_INFO *)roomShmAddr)->chats[i].receiverID,
                        ((ROOM_INFO *)roomShmAddr)->chats[i].message);
                }
            }
            if (line == 0)
            {
                break;
            }
        }
        wrefresh(OutputWnd);

        wclear(UserWnd);
        box(UserWnd, 0, 0);
        mvwprintw(UserWnd, 0, 2, "User");

        for (int i = 0; i < ((ROOM_INFO *)roomShmAddr)->userCnt; i++)
        {
            if ((strcmp(((ROOM_INFO *)roomShmAddr)->userIDs[i], "")))
            {
                mvwprintw(
                    UserWnd,
                    i + 1,
                    1,
                    "%s",
                    ((ROOM_INFO *)roomShmAddr)->userIDs[i]);
            }
        }
        wrefresh(UserWnd);
        pthread_mutex_unlock(&mutex);
        sleep(0);
    }
}

void chatWrite()
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        mvwgetstr(InputWnd, 1, 1, inputStr);

        bool isEmptyMsg = !strcmp(inputStr, "");
        if (isEmptyMsg)
        {
            pthread_mutex_unlock(&mutex);
            sleep(0);
            continue;
        }

        bool isQuitMsg = !strcmp(inputStr, "/quit");
        if (isQuitMsg)
        {
            logout();
            pthread_mutex_unlock(&mutex);
            quit = true;
            break;
        }

        strcpy(allMsg, inputStr);

        // strtok은 동작 과정 중 inputStr의 space를 '\0'으로 변경시킴(귓속말이 아닐 때 그대로 inputStr을 출력할 경우 공백 이후의 문자가 출력되지 않음)
        pch = strtok(inputStr, " ");

        bool isWhisper = !strcmp(pch, "/stalk");
        if (isWhisper)
        {
            pch = strtok(NULL, " ");
            if (pch != NULL)
            {
                strcpy(receiverID, pch);
            }
            else
            {
                wclear(InputWnd);
                box(InputWnd, 0, 0);
                mvwprintw(InputWnd, 0, 2, "Input");
                wrefresh(InputWnd);

                pthread_mutex_unlock(&mutex);
                sleep(0);
                continue;
            }
            pch = strtok(NULL, " ");
            if (pch != NULL)
            {
                strcpy(whisperMsg, pch);
            }
            else
            {
                wclear(InputWnd);
                box(InputWnd, 0, 0);
                mvwprintw(InputWnd, 0, 2, "Input");
                wrefresh(InputWnd);

                pthread_mutex_unlock(&mutex);
                sleep(0);
                continue;
            }
            pch = strtok(NULL, " ");
            while (pch != NULL)
            {
                strcat(whisperMsg, " ");
                strcat(whisperMsg, pch);
                pch = strtok(NULL, " ");
            }
            for (int i = 0; i < MAX_CAPACITY - 1; i++)
            {
                memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i + 1), sizeof(CHAT_INFO));
            }
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].receiverID, receiverID);
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].message, whisperMsg);
        }
        else
        {
            for (int i = 0; i < MAX_CAPACITY - 1; i++)
            {
                memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i + 1), sizeof(CHAT_INFO));
            }
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].receiverID, "ALL");
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].message, allMsg);
        }
        strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].senderID, userID);

        wclear(InputWnd);
        box(InputWnd, 0, 0);
        mvwprintw(InputWnd, 0, 2, "Input");
        wrefresh(InputWnd);

        pthread_mutex_unlock(&mutex);
        sleep(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechat userID\n");
        exit(0);
    }
    strcpy(userID, argv[1]);

    initWindow();
    roomKey = getKey("room_key.txt");
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);
    login();

    pthread_mutex_init(&mutex, NULL);
    pthread_t tidRead, tidWrite;
    int readErr = pthread_create(&tidRead, NULL, (void *)chatRead, NULL);
    int writeErr = pthread_create(&tidWrite, NULL, (void *)chatWrite, NULL);

    pthread_join(tidRead, NULL);
    pthread_join(tidWrite, NULL);

    delwin(OutputWnd);
    delwin(InputWnd);
    delwin(UserWnd);
    delwin(AppWnd);
    endwin();

    pthread_mutex_destroy(&mutex);
    return 0;
}