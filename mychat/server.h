/* serve.h는 server에서 관리하는 자원들이 들어있다^_^ */

#ifndef SERVER_H
#define SERVER_H

#include <unistd.h> 
#include <sys/types.h> 
#include <signal.h> 
#include <stdio.h>

#define MAX_ROOM 10 //방은 최대 10개
#define MAX_CLIENT 20 //최대 client 수는 20명
#define MAX_NAME_LEN 50
#define BUFSIZE 1024

/* pipe fd read and write */
#define PIPE_READ 0 
#define PIPE_WRITE 1

/* 유저 정보 구조체 */
typedef struct {
    char nickname[MAX_NAME_LEN]; // 중복 불가
    int room_number;//room_number (0~9)
    pid_t pid; //자식 프로세스 pid
    int from_parent_to_child[2]; // 부모->자식 : 부모는 write, 자식은 read
    int from_child_to_parent[2]; // 자식->부모 : 자식이 write, 부모는 read
    int client_socket_fd; // client socket fd : child mainly uses
    int is_activated; // 1이면 사용 중
} ClientInfo;

/* 방 정보 구조체 */
typedef struct { 
    int room_number; // 방 number
    int mem_cnt; //현재 참여 중인 member 수
    pid_t mem_pids[MAX_CLIENT]; //현재 참여 중인 member들의 자식 pid 
} RoomInfo;

int user_idx; //유저 idx
ClientInfo users[MAX_CLIENT] = {0}; // ClientInfo 구조체 초기화
RoomInfo rooms[MAX_ROOM] = {0}; //room 구조체 초기화


int find_empty_user_slot(){
    for (int i=0; i<MAX_CLIENT; i++){
        if (users[i].is_activated == 0) return i; //비어있으면 해당 자리 return
    }
    return -1;
}

int find_user_idx_by_pid(int child_pid){
    for (int i=0; i<MAX_CLIENT; i++){
        if (users[i].pid == child_pid) return i;
    }
}


#endif