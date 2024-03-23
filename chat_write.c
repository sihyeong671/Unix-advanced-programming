#include <ncurses.h>
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
    void *chatShmAddr = (void *)0;
    void *roomShmAddr = (void *)0;
    int shmKey = getKey("chat_key.txt");
    int roomKey = getKey("room_key.txt");

    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechatwrite UserID \n");
        exit(0);
    }

    strcpy(userID, argv[1]);
    printf("here\n");

    setShmAddr(shmKey, 10 * sizeof(CHAT_INFO), &chatShmAddr);
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);
    printf("here\n");

    if (((ROOM_INFO *)roomShmAddr)->userCnt < 3)
    {
        memcpy(((ROOM_INFO *)roomShmAddr)->userIDs[((ROOM_INFO *)roomShmAddr)->userCnt], userID, sizeof(userID));
        ((ROOM_INFO *)roomShmAddr)->userCnt++;
    }

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
    char inputstr[40];

    // printf("reached before inf loop\n");
    while (1)
    {
        refresh();

        mvwscanw(InputWnd, 1, 1, "%s", inputstr);
        wrefresh(InputWnd);
        for (i = 0; i < 10 - 1; i++)
        {
            // printf("move forward\n");
            //  *((CHAT_INFO *)shmaddr + (i)) = *((CHAT_INFO *)shmaddr + (i + 1));
            memcpy((CHAT_INFO *)chatShmAddr + (i), (CHAT_INFO *)chatShmAddr + (i + 1), sizeof(CHAT_INFO));
            // printf("move forward end\n");
        }
        strcat(inputstr, "\0");
        strcpy(((CHAT_INFO *)chatShmAddr + (10 - 1))->message, inputstr);
        strcpy(((CHAT_INFO *)chatShmAddr + (10 - 1))->userID, userID);
        // usleep(100000);
    }
    endwin();
    return 0;
}