CC = gcc # 컴파일러
LDFLAGS=-lncursesw # 라이브러리

all: read write # 생성할 실행파일

key := $(shell cat room_key.txt)

read: chat_read.c # chat_read.c 컴파일
	$(CC) -o read chat_read.c $(LDFLAGS)

write: chat_write.c # chat_write.c 컴파일
	$(CC) -o write chat_write.c $(LDFLAGS)

clean: # 실행파일 삭제
	ipcrm -M $(key)
	rm -f read write
