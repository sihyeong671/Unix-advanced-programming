CC = gcc# 컴파일러
LDFLAGS=-lncurses -lpthread# 라이브러리

all: chat # 생성할 실행파일

key := $(shell cat room_key.txt)

chat: csechat.c # chat_read.c 컴파일
	$(CC) -o chat csechat.c $(LDFLAGS)

clean: # 실행파일 삭제
	ipcrm -M $(key)
	rm -f chat
