// #include <ncurses.h>
#include <ncursesw/curses.h> // -lncursesw
#include "chat_info.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LEN 100

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

int main(int argc, char *argv[])
{
    int chatShmId, roomShmId;
    int shmbufindex, readmsgcount;
    char userID[20];
    void *roomShmAddr = (void *)0;
    int roomKey = getKey("room_key.txt");

    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechatwrite UserID \n");
        exit(0);
    }

    strcpy(userID, argv[1]);

    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);


    int currentUserCnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
    if (currentUserCnt < 3)
    {
        memcpy(((ROOM_INFO *)roomShmAddr)->userIDs[currentUserCnt], userID, sizeof(userID));
        ((ROOM_INFO *)roomShmAddr)->userCnt++;
    } else {
        perror("too much users in room");
        exit(0);
    }

    initscr();

    WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    WINDOW *InputWnd = subwin(stdscr, 3, 42, 13, 0);
    WINDOW *UserWnd = subwin(stdscr, 5, 20, 0, 43);
    WINDOW *AppWnd = subwin(stdscr, 6, 20, 10, 43);

    box(OutputWnd, 0, 0);
    box(InputWnd, 0, 0);
    box(UserWnd, 0, 0);
    box(AppWnd, 0, 0);

    mvwprintw(OutputWnd, 0, 2, "Output");
    mvwprintw(InputWnd, 0, 2, "Input");
    mvwprintw(UserWnd, 0, 2, "User");
    mvwprintw(AppWnd, 0, 2, "App name");
    refresh();


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
    return 0;
}