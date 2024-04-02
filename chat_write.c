// #include <ncurses.h>
#include <ncursesw/curses.h> // -lncursesw
#include "chat_info.h"
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int main(int argc, char *argv[])
{
    int chatShmId, roomShmId;
    int shmbufindex, readmsgcount;
    char userID[20];
    void *roomShmAddr = (void *)0;
    // 파일로부터 키를 받아온다
    int roomKey = getKey("room_key.txt");

    // 실행 시 UserID를 넘겨주지 않은 경우
    if (argc < 2)
    {
        fprintf(stderr, "[Usage]: ./csechatwrite UserID \n");
        exit(0);
    }

    strcpy(userID, argv[1]);

    // 공유 메모리를 할당하고, roomShmAddr이 가리키게 한다
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);

    // 유저의 수를 증가
    int currentUserCnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
    if (currentUserCnt < 3)
    {
        memcpy(((ROOM_INFO *)roomShmAddr)->userIDs[currentUserCnt], userID, sizeof(userID));
        ((ROOM_INFO *)roomShmAddr)->userCnt++;
    }
    else // 유저는 3명까지만
    {
        perror("too much users in room");
        exit(0);
    }

    // 입출력을 위한 윈도우 초기화
    initscr();

    WINDOW *OutputWnd = subwin(stdscr, 12, 42, 0, 0);
    WINDOW *InputWnd = subwin(stdscr, 3, 42, 13, 0);
    WINDOW *UserWnd = subwin(stdscr, 5, 20, 0, 43);
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
    char inputstr[40];

    while (1)
    {
        mvwgetstr(InputWnd, 1, 1, inputstr);

        // \quit을 입력하면 채팅창에서 나가도록 구현
        if (!strcmp(inputstr, "\\quit"))
        {
            int index = 0;
            for (i = 0; i < 3; i++)
            {
                if (!strcmp(((ROOM_INFO *)roomShmAddr)->userIDs[i], userID))
                {
                    index = i;
                }
            }
            for (i = index; i < 3; i++)
            {
                memcpy(((ROOM_INFO *)roomShmAddr)->userIDs + i, ((ROOM_INFO *)roomShmAddr)->userIDs + (i + 1), 20 * sizeof(char));
            }
            ((ROOM_INFO *)roomShmAddr)->userCnt--;
            break;
        }
        wrefresh(InputWnd);

        // 채팅 입력 시 슬라이딩 구현
        for (i = 0; i < 9; i++)
        {
            // 왼쪽의 주소에 오른쪽 주소를 담음
            memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i + 1), sizeof(CHAT_INFO));
        }
        strcat(inputstr, "\0");
        strcpy(((ROOM_INFO *)roomShmAddr)->chats[9].message, inputstr);
        strcpy(((ROOM_INFO *)roomShmAddr)->chats[9].userID, userID);

        // read 프로세스에게 새 메세지가 들어왔음을 알려줌
        ((ROOM_INFO *)roomShmAddr)->chatFlag++;

        // 입력 윈도우 clear하기
        wclear(InputWnd);
        box(InputWnd, 0, 0);
        mvwprintw(InputWnd, 0, 2, "Input");
    }
    endwin();
    return 0;
}