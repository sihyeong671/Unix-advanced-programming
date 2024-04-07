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

    // mvwgetstr이 non-blocking 함수라 입력이 없을 시 write 쓰레드에서 멈추어 있음
    // InputWnd에 timeout을 걸어 3초 간 입력이 없을 시 read 쓰레드로 전환
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
    // 채팅방에는 3명까지만
    int currentUserCnt = ((ROOM_INFO *)roomShmAddr)->userCnt;
    if (currentUserCnt < 3)
    {
        // 유저 ID를 채팅방에 추가
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
    // 유저 ID를 채팅방에서 제거
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
        // ncurses가 멀티 쓰레드 환경에서 동작하는 경우 출력이 깨져보이는 문제 발생
        // 이를 해결하기 위해 mutex를 이용하여 한 시점에서 하나의 ncurses가 동작하도록 보장
        pthread_mutex_lock(&mutex);

        wclear(OutputWnd);
        box(OutputWnd, 0, 0);
        mvwprintw(OutputWnd, 0, 2, "Output");

        // 10개까지만 출력
        int line = 10;

        for (int i = MAX_CAPACITY - 1; i >= 0; i--)
        {
            bool msgEmpty = !(strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].message, ""));
            // 빈 메세지가 아닌 경우 출력
            if (!msgEmpty)
            {
                bool isAllMsg = !strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].receiverID, "ALL");
                bool sendWhisper = !strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].senderID, userID);
                bool receiveWhisper = !strcmp(((ROOM_INFO *)roomShmAddr)->chats[i].receiverID, userID);
                
                // 전체 메세지 출력
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
                
                // 귓속말 출력
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

        // 접속한 유저 출력
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

        // sleep(0)을 이용하여 쓰레드가 멈추지 않고 다른 우선순위가 같은 write 쓰레드로 전환 보장
        pthread_mutex_unlock(&mutex);
        sleep(0);
    }
}

void chatWrite()
{
    while (1)
    {
        // ncurses가 멀티 쓰레드 환경에서 동작하는 경우 출력이 깨져보이는 문제 발생
        // 이를 해결하기 위해 mutex를 이용하여 한 시점에서 하나의 ncurses가 동작하도록 보장
        pthread_mutex_lock(&mutex);
        mvwgetstr(InputWnd, 1, 1, inputStr);

        // 공백이 입력되거나 타임아웃이 발생한 경우 read 쓰레드로 전환
        bool isEmptyMsg = !strcmp(inputStr, "");
        if (isEmptyMsg)
        {
            pthread_mutex_unlock(&mutex);
            sleep(0);
            continue;
        }

        // 사용자가 /quit을 입력한 경우 종료
        bool isQuitMsg = !strcmp(inputStr, "/quit");
        if (isQuitMsg)
        {
            logout();
            pthread_mutex_unlock(&mutex);
            quit = true;
            break;
        }

        // 귓속말 구현을 위해 strtok을 이용하여 공백을 기준으로 파싱
        // strtok은 동작 과정 중 inputStr의 space를 '\0'으로 변경시킴
        // 즉, 귓속말이 아닐 때 그대로 inputStr을 출력할 경우 공백 이후의 문자가 출력되지 않음
        // 이를 해결하기 위해 전체 메세지는 귓속말 구현을 위한 strtok을 사용하기 이전의 inputStr을 복사하여 사용
        strcpy(allMsg, inputStr);

        pch = strtok(inputStr, " ");
        bool isWhisper = !strcmp(pch, "/stalk");
        // 귓속말인 경우
        if (isWhisper)
        {
            pch = strtok(NULL, " ");
            // 귓속말 수신자를 받아옴
            if (pch != NULL)
            {
                strcpy(receiverID, pch);
            }
            // 수신자가 명시되지 않은 경우 read 쓰레드로 전환
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
            // 메세지를 받아옴
            pch = strtok(NULL, " ");
            if (pch != NULL)
            {
                strcpy(whisperMsg, pch);
            }
            // 메세지가 없는 경우 read 쓰레드로 전환
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
            // 공백 기준으로 파싱된 메세지를 다시 합쳐줌
            pch = strtok(NULL, " ");
            while (pch != NULL)
            {
                strcat(whisperMsg, " ");
                strcat(whisperMsg, pch);
                pch = strtok(NULL, " ");
            }
            // 새 귓속말을 공유 메모리에 전달
            for (int i = 0; i < MAX_CAPACITY - 1; i++)
            {
                memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i + 1), sizeof(CHAT_INFO));
            }
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].senderID, userID);
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].receiverID, receiverID);
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].message, whisperMsg);
        }
        else
        {
            // 새 전체 메세지를 공유 메모리에 전달
            for (int i = 0; i < MAX_CAPACITY - 1; i++)
            {
                memcpy(((ROOM_INFO *)roomShmAddr)->chats + i, ((ROOM_INFO *)roomShmAddr)->chats + (i + 1), sizeof(CHAT_INFO));
            }
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].senderID, userID);
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].receiverID, "ALL");
            strcpy(((ROOM_INFO *)roomShmAddr)->chats[MAX_CAPACITY - 1].message, allMsg);
        }

        wclear(InputWnd);
        box(InputWnd, 0, 0);
        mvwprintw(InputWnd, 0, 2, "Input");
        wrefresh(InputWnd);

        // sleep(0)을 이용하여 쓰레드가 멈추지 않고 다른 우선순위가 같은 read 쓰레드로 전환 보장
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
    // 키 값을 파일로부터 읽어와 공유 메모리 설정
    roomKey = getKey("room_key.txt");
    setShmAddr(roomKey, sizeof(ROOM_INFO), &roomShmAddr);
    login();

    // ncurses의 멀티 쓰레드 환경에서 출력 오류 문제를 해결하기 위해 mutex 사용
    pthread_mutex_init(&mutex, NULL);
    pthread_t tidRead, tidWrite;

    int readErr = pthread_create(&tidRead, NULL, (void *)chatRead, NULL);
    int writeErr = pthread_create(&tidWrite, NULL, (void *)chatWrite, NULL);

    // join을 이용하여 main보다 먼저 종료되지 않도록 보장
    pthread_join(tidRead, NULL);
    pthread_join(tidWrite, NULL);

    delwin(OutputWnd);
    delwin(InputWnd);
    delwin(UserWnd);
    delwin(AppWnd);
    endwin();

    // mutex 해제
    pthread_mutex_destroy(&mutex);
    return 0;
}