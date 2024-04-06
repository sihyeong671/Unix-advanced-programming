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

void initWindow()
{
    initscr();

    // TODO
    // 위치 지정 변수 설정
    // 아래 변수를 전역으로
    // WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    InputWnd = subwin(stdscr, 3, 42, 13, 0);
    UserWnd = subwin(stdscr, 10, 20, 0, 43);
    AppWnd = subwin(stdscr, 6, 20, 10, 43);

    box(OutputWnd, 0, 0);
    box(InputWnd, 0, 0);
    box(UserWnd, 0, 0);
    box(AppWnd, 0, 0);

    mvwprintw(OutputWnd, 0, 2, "Output");
    mvwprintw(InputWnd, 0, 2, "Input");
    mvwprintw(UserWnd, 0, 2, "User");
    mvwprintw(AppWnd, 0, 2, "App name");
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

            if(!strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].receiverID, "ALL"))
                {
                    mvwprintw(
                        OutputWnd,
                        line--,
                        1,
                        "[%s]: %s",
                        ((ROOM_INFO *)roomShmAddr)->chats[i].senderID,
                        ((ROOM_INFO *)roomShmAddr)->chats[i].message);
                }
            else if ((strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].message, "")))
            {
                if((!strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].receiverID, userID))){
                    mvwprintw(
                        OutputWnd,
                        line--,
                        1,
                        "[%s] >> [%s]: %s",
                        ((ROOM_INFO *)roomShmAddr)->chats[i].senderID,
                        ((ROOM_INFO *)roomShmAddr)->chats[i].receiverID,
                        ((ROOM_INFO *)roomShmAddr)->chats[i].message);
                }
                else if(!strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].senderID, userID))
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
            if (line == 0){
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

    char inputStr[40];
    while (1)
    {
        pthread_mutex_lock(&mutex);
        mvwgetstr(InputWnd, 1, 1, inputStr);

        if (!strcmp(inputStr, "")) {
            pthread_mutex_unlock(&mutex);
            sleep(0);
            continue;
        }
        
        if (!strcmp(inputStr, "\\quit"))
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
            pthread_mutex_unlock(&mutex);
            quit = true;
            break;
        }
        
        char *pch;
        pch = strtok(inputStr, " ");
        bool isWhisper = !strcmp(pch, "/stalk");
        // A B C D E
        // strtok 1 pch == A
        // strtok 2 pch == B
        // while (pch != NULL)
        // {  
        //     printf ("%s\n",pch);
        //     pch = strtok (NULL, " ");
        // }
        for (int i = 0; i < MAX_CAPACITY; i++)
        {
            // 왼쪽의 주소에 오른쪽 주소를 담음
            memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i + 1), sizeof(CHAT_INFO));
        }

        // /stalk ID명 메시지
        if (isWhisper){
            pch = strtok(NULL, " "); // receiverID
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY-1].receiverID, pch);
            pch = strtok(NULL, " "); // message
            // TODO
            // stalk id
            // if (pch == NULL){
            //     pch = "null";
            // }
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY-1].message, pch); // while loop 돌려서 해결
        }
        else
        {
            strcat(inputStr, "\0");
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY-1].receiverID, "ALL");
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY-1].message, inputStr);
        }
        strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY-1].senderID, userID);
        wrefresh(InputWnd);


        
        //((ROOM_INFO *)roomShmAddr)->chatFlag += ((ROOM_INFO *)roomShmAddr)->userCnt;

        // 입력한 문자만 clear하기
        wclear(InputWnd);
        box(InputWnd, 0, 0);
        mvwprintw(InputWnd, 0, 2, "Input");
        pthread_mutex_unlock(&mutex);
        sleep(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechatwrite UserID \n");
        exit(0);
    }
    strcpy(userID, argv[1]);

    // int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    // fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    pthread_t tidRead, tidWrite;
    initWindow();
    roomKey = getKey("room_key.txt");
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);
    login();

    wtimeout(InputWnd, 3000);

    pthread_mutex_init(&mutex, NULL);
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