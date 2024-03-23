#include <ncurses.h>
// #include <ncursesw/curses.h> // -lncursesw
#include "chat_info.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_LEN 100

int get_key(char* file_path){
    FILE* fs;
    fs = fopen(file_path, "r");
    char str[MAX_LEN];
    fgets(str, MAX_LEN, fs);
    int num = atio(str);
    return num;
}

int main(void)
{
    int chatShmId, roomShmId;
    int shmbufindex, readmsgcount;
    CHAT_INFO *chatInfo = NULL;
    void *chatShmAddr = (void *)0;
    int shm_key = get_key("key.txt");

    chatShmId = shmget((key_t)shm_key, 10 * sizeof(CHAT_INFO),
                       0666 | IPC_CREAT | IPC_EXCL);

    if (chatShmId < 0)
    {
        chatShmId = shmget((key_t)shm_key, 10 * sizeof(CHAT_INFO),
                           0666);
        chatShmAddr = shmat(chatShmId, (void *)0, 0666);
        if (chatShmAddr < 0)
        {
            perror("shmat attach is failed : ");
            exit(0);
        }
    }
    chatShmAddr = shmat(chatShmId, (void *)0, 0666);

    void *roomShmAddr = (void *)0;

    roomShmId = shmget((key_t)24561, sizeof(ROOM_INFO),
                       0666 | IPC_CREAT | IPC_EXCL);

    if (roomShmId < 0)
    {
        roomShmId = shmget((key_t)24561, sizeof(ROOM_INFO),
                           0666);
        roomShmAddr = shmat(roomShmId, (void *)0, 0666);
        if (roomShmAddr < 0)
        {
            perror("shmat attach is failed : ");
            exit(0);
        }
    }
    roomShmAddr = shmat(roomShmId, (void *)0, 0666);

    initscr();

    WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    WINDOW *InputWnd = subwin(stdscr, 3, 42, 13, 0);
    WINDOW *UserWnd = subwin(stdscr, 5, 20, 0, 43);

    box(OutputWnd, 0, 0);
    box(InputWnd, 0, 0);
    box(UserWnd, 0, 0);

    mvwprintw(OutputWnd, 0, 2, "Output");
    mvwprintw(InputWnd, 0, 2, "Input");
    mvwprintw(UserWnd, 0, 2, "User");

    int i;

    // printf("reached before inf loop\n");

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