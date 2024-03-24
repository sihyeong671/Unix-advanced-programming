# 유닉스 응용 프로그래밍

## TODO
- [ ] 강제종료시 공유 메모리 삭제(필요한지 논의해볼 것)
- [ ] 윈도우 함수 따로 만들어서 
- [ ] makefile 만들기
- [ ] 코드 최적화

## How To use
```sh
gcc -o read chat_read.c -lncursesw
gcc -o write chat_write.c -lncursesw

# read 프로세스 한 개
# write 프로세스 세 개
```

## 참고 사항
```sh
ipcs # 공유 메모리 세그먼트 확인
ipcrm -m $(shmid) # 공유메모리 삭제

kill -9 $(pid) # 프로세스 kill
```