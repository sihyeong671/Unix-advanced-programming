#include <ncurses.h>
// #include <ncursesw/curses.h> // -lncursesw
#include "chat_info.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

int main(void)
{
    int chatShmId, roomShmId;
    int shmbufindex, readmsgcount;
    CHAT_INFO *chatInfo = NULL;
    void *chatShmAddr = (void *)0;
    void *roomShmAddr = (void *)0;
    int shmKey = getKey("chat_key.txt");
    int roomKey = getKey("room_key.txt");


    setShmAddr(shmKey, 10 * sizeof(CHAT_INFO), &chatShmAddr);
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);

    initscr();

    WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    WINDOW *InputWnd = subwin(stdscr, 3, 42, 13, 0);
    WINDOW *UserWnd = subwin(stdscr, 10, 20, 0, 43);
    WINDOW *AppWnd = subwin(stdscr, 6, 20, 10, 43);

    box(OutputWnd, 0, 0);
    box(InputWnd, 0, 0);
    box(UserWnd, 0, 0);
    box(AppWnd, 0, 0);

    mvwprintw(OutputWnd, 0, 2, "Output");
    mvwprintw(InputWnd, 0, 2, "Input");
    mvwprintw(UserWnd, 0, 2, "User");
    mvwprintw(AppWnd, 0, 2, "App name");
    
    int i;
    while (1)
    {
        refresh();

        for (i = 0; i < 10; i++)
        {
            if ((strcmp(((CHAT_INFO *)chatShmAddr + i)->userID, "")))
            {
                // printf("%s\n", ((CHAT_INFO *)shmaddr + i)->message);
                mvwprintw(OutputWnd, i + 1, 1, "[%s]: %s", ((CHAT_INFO *)chatShmAddr + i)->userID, ((CHAT_INFO *)chatShmAddr + i)->message);
                // mvwprintw(OutputWnd, i, 0, "%s", "here to print");
            }
        }
        wrefresh(OutputWnd);

        for (i = 0; i < ((ROOM_INFO *)roomShmAddr)->userCnt; i++)
        {
            mvwprintw(UserWnd, i + 1, 1, "%s", ((ROOM_INFO *)roomShmAddr)->userIDs[i]);
        }
        wrefresh(UserWnd);
    }
    endwin();

    // TODO
    // 채팅이랑 유저 정보 공유메모리 분리
    // 커서 flush
    return 0;
}