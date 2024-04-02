#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include <sys/shm.h>
#include <ncursesw/curses.h>

#include "chat_info.h"

#define MAX_LEN 100


int getKey(char *file_path);
void initWindow();
void setShmAddr(int key, int size, void **shmAddr);
void chatRead();
void chatWrite();

WINDOW *OutputWnd, *InputWnd, *UserWnd, *AppWnd;

int chatShmId, roomShmId;
int shmbufindex, readmsgcount;
CHAT_INFO *chatInfo = NULL;
void *roomShmAddr = (void *)0;
int roomKey;
char userID[20];


void initWindow() {
    initscr();

    // TODO
    // 위치 지정 변수 설정
    // 아래 변수를 전역으로
    //WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
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

void chatRead(){
    int i;
    int pre_cnt = 0;
    while (1)
    {

        if (((ROOM_INFO *)roomShmAddr)->chatFlag > 0){
            ((ROOM_INFO *)roomShmAddr)->chatFlag--;
            wclear(OutputWnd);
            box(OutputWnd, 0, 0);
            mvwprintw(OutputWnd, 0, 2, "Output");
        }

        for (i = 0; i < 10; i++)
        {
            if ((strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].message, "")))
            {
                mvwprintw(
                    OutputWnd,
                    i + 1, 
                    1, 
                    "[%s]: %s", 
                    ((ROOM_INFO *)roomShmAddr)->chats[i].userID,
                    ((ROOM_INFO *)roomShmAddr)->chats[i].message
                );
            }
        }
        wrefresh(OutputWnd);
        
        for (i = 0; i < ((ROOM_INFO *)roomShmAddr)->userCnt; i++)
        {
            if ((strcmp(((ROOM_INFO *)roomShmAddr)->userIDs[i], ""))) {
                mvwprintw(
                    UserWnd, 
                    i + 1, 
                    1, 
                    "%s", 
                    ((ROOM_INFO *)roomShmAddr)->userIDs[i]);
            }
        }
        if (pre_cnt != ((ROOM_INFO *)roomShmAddr)->userCnt){
            pre_cnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
            wclear(UserWnd);
            box(UserWnd, 0, 0);
            mvwprintw(UserWnd, 0, 2, "User");
        }
        wrefresh(UserWnd);

    }
    endwin();
}


void chatWrite(){

    int currentUserCnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
    if (currentUserCnt < 3)
    {
        memcpy(((ROOM_INFO *)roomShmAddr)->userIDs[currentUserCnt], userID, sizeof(userID));
        ((ROOM_INFO *)roomShmAddr)->userCnt++;
    } else {
        perror("too much users in room");
        exit(0);
    }

    int i;
    char inputstr[40];

    while (1)
    {
        // TODO
        // 종료 조건 만들기
        // 띄워쓰기 문제 해결
        // 
        mvwscanw(InputWnd, 1, 1, "%s", inputstr);
        if (!strcmp(inputstr, "quit")) {
            int index = 0;
            for(i = 0; i < 3; i++) {
                if (!strcmp(((ROOM_INFO *)roomShmAddr)->userIDs[i], userID)) {
                    index = i;
                }
            }
            for(i = index; i < 3; i++) {
                memset(((ROOM_INFO *)roomShmAddr)->userIDs + i, 0, 20 * sizeof(char));
                memcpy(((ROOM_INFO *)roomShmAddr)->userIDs + i, ((ROOM_INFO *)roomShmAddr)->userIDs + (i+1), 20 * sizeof(char));
            }
            ((ROOM_INFO *)roomShmAddr)->userCnt--;
            break;
        } 
        wrefresh(InputWnd);

        for (i = 0; i < 9; i++)
        {
            // 왼쪽의 주소에 오른쪽 주소를 담음
            memset(((ROOM_INFO *)roomShmAddr)->chats + (i), 0, sizeof(CHAT_INFO));
            memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i+1), sizeof(CHAT_INFO));
            // memcpy(((ROOM_INFO *)roomShmAddr)->chats[i], ((ROOM_INFO *)roomShmAddr)->chats[i+1], sizeof(CHAT_INFO));
            // memcpy
        }
        strcat(inputstr, "\0");
        strcpy(((ROOM_INFO *)roomShmAddr)->chats[9].message, inputstr);
        strcpy(((ROOM_INFO *)roomShmAddr)->chats[9].userID, userID);
        ((ROOM_INFO *)roomShmAddr)->chatFlag++;
        // 입력한 문자만 clear하기 
        wclear(InputWnd);
        box(InputWnd, 0, 0);
        mvwprintw(InputWnd, 0, 2, "Input");
    }
    endwin();
}

int main(int argc, char *argv[]){
    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechatwrite UserID \n");
        exit(0);
    }
    strcpy(userID, argv[1]);


    pthread_t tidRead, tidWrite;
    initWindow();
    roomKey = getKey("room_key.txt");

    
    pthread_create(&tidRead, NULL, chatRead, NULL);
    pthread_create(&tidWrite, NULL, chatWrite, NULL);

    ptherad_join(tidRead, NULL);
    ptherad_join(tidWrite, NULL);

    return 0;
}