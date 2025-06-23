/* resource.h는 필요한 자원들이 들어있다^_^ */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <unistd.h> //pid_t
#include <signal.h> //사용자 정의 signal
#include <stdio.h>

#define MAX_ROOM 10
#define MAX_CLIENT 20
#define MAX_NAME_LEN 50
#define BUFSIZE 1024

/* pipe fd read and write */
#define PIPE_READ 0 
#define PIPE_WRITE 1

/* 사용자 정의 시그널 */
#define SIGUSR1_PARENT_NOTIFY SIGUSR1 // 자식-> 부모 메시지 도착
#define SIGUSR2_CHILD_NOTIFY SIGUSR2 // 부모 -> 자식 메시지 있음

/* 유저 정보 구조체 */
typedef struct {
    char nickname[MAX_NAME_LEN]; // 중복 불가
    int rid = -1; // -1이면 미참여, 0~9 사이의 방 번호 존재
    pid_t pid; //자식 프로세스 pid
    int pipe_to_child[2]; // 부모->자식 : 부모는 write, 자식은 read
    int pipe_to_parent[2]; // 자식->부모 : 자식이 write, 부모는 read

} UserInfo;

/* 방 정보 구조체 */
typedef struct {
    int rid = -1;
    int mem_cnt = 0; //현재 참여 중인 member 수
    pid_t mem_pids[MAX_CLIENT]; //현재 참여 중인 member들의 자식 pid 
} RoomInfo;

#endif