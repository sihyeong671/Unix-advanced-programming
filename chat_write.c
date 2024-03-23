#include <ncurses.h>
#include "chat_info.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int chatShmId, roomShmId;
    int shmbufindex, readmsgcount;
    char userID[20];
    CHAT_INFO *chatInfo = NULL;

    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechatwrite UserID \n");
        exit(0);
    }

    strcpy(userID, argv[1]);

    void *chatShmAddr = (void *)0;

    chatShmId = shmget((key_t)2019, 10 * sizeof(CHAT_INFO),
                       0666 | IPC_CREAT | IPC_EXCL);

    if (chatShmId < 0)
    {
        chatShmId = shmget((key_t)2019, 10 * sizeof(CHAT_INFO),
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