// #include <ncurses.h>
#include <ncursesw/curses.h> // -lncursesw
#include "chat_info.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_LEN 100

// 파일에 저장된 키를 불러옴
// 입력: 파일 경로
// 출력: 키
int getKey(char *file_path)
{
    FILE *fs;
    fs = fopen(file_path, "r");
    char str[MAX_LEN];
    fgets(str, MAX_LEN, fs);
    int num = atoi(str);
    return num;
}

// 공유 메모리 설정
// 입력: 키, 할당할 공유 메모리의 크기, 공유 메모리 포인터
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
    void *roomShmAddr = (void *)0;
    // 파일로부터 키를 받아온다
    int roomKey = getKey("room_key.txt");

    // 공유 메모리를 할당하고, roomShmAddr이 가리키게 한다
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);

    // 입출력을 위한 윈도우 초기화
    initscr();

    WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    WINDOW *InputWnd = subwin(stdscr, 3, 42, 13, 0);
    WINDOW *UserWnd = subwin(stdscr, 10, 20, 0, 43);
    WINDOW *AppWnd = subwin(stdscr, 6, 20, 10, 43);

    // 나눠진 윈도우 영역 가시화
    box(OutputWnd, 0, 0);
    box(InputWnd, 0, 0);
    box(UserWnd, 0, 0);
    box(AppWnd, 0, 0);

    mvwprintw(OutputWnd, 0, 2, "Output");
    mvwprintw(InputWnd, 0, 2, "Input");
    mvwprintw(UserWnd, 0, 2, "User");
    mvwprintw(AppWnd, 0, 2, "App name");
    mvwprintw(AppWnd, 3, 3, "Unix Chat app");
    refresh();

    int i;
    int pre_cnt = 0;
    while (1)
    {
        // 새로 메세지가 들어온 경우 출력이 남아있는 문제를 해결하기 위해 화면을 지우고 다시 씀
        // 동시에 입력이 들어오는 경우 에러가 발생할 수 있기 때문에 추후 세마포어를 사용할 필요 있음
        // 1. 무한 clear 문제 2. clear 안하는 문제
        if (((ROOM_INFO *)roomShmAddr)->chatFlag > 0)
        {
            ((ROOM_INFO *)roomShmAddr)->chatFlag--;
            wclear(OutputWnd);
            box(OutputWnd, 0, 0);
            mvwprintw(OutputWnd, 0, 2, "Output");
        }

        // 메세지 출력
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
                    ((ROOM_INFO *)roomShmAddr)->chats[i].message);
            }
        }
        wrefresh(OutputWnd);

        // 접속한 유저 출력
        for (i = 0; i < ((ROOM_INFO *)roomShmAddr)->userCnt; i++)
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
        // 접속한 유저에 변화가 있는 경우 출력이 남아있는 문제를 해결하기 위해 화면을 지우고 다시 씀
        if (pre_cnt != ((ROOM_INFO *)roomShmAddr)->userCnt)
        {
            pre_cnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
            wclear(UserWnd);
            box(UserWnd, 0, 0);
            mvwprintw(UserWnd, 0, 2, "User");
        }
        wrefresh(UserWnd);
    }
    endwin();

    return 0;
}