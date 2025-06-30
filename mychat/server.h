/* server.h는 server에서 관리하는 자원들이 들어있다^_^ */

#ifndef SERVER_H
#define SERVER_H

#define _POSIX_C_SOURCE 200809L
#include <unistd.h> 
#include <sys/types.h>
#include <sys/signal.h> 
#include <signal.h> 
#include <stdio.h>
#include "protocol.h"

#define MAX_ROOM 16 //방은 최대 16개
#define MAX_CLIENT 24 //최대 client 수는 20명
#define MAX_NAME_LEN 64
#define BUFSIZE 1024

/* pipe fd read and write */
#define PIPE_READ 0 
#define PIPE_WRITE 1

/* 유저 정보 구조체 */
typedef struct {
    char nickname[MAX_NAME_LEN]; // 중복 불가
    int room_idx;//room_number (0~9)
    pid_t pid; //자식 프로세스 pid
    int from_parent_to_child[2]; // 부모->자식 : 부모는 write, 자식은 read
    int from_child_to_parent[2]; // 자식->부모 : 자식이 write, 부모는 read
    int client_socket_fd; // client socket fd : child mainly uses
    int is_activated; // 1이면 사용 중
} ClientInfo;

/* 방 정보 구조체 */
typedef struct {
    char room_name [64];
    int room_idx;
    int mem_cnt; //현재 참여 중인 member 수
    int is_activated; //사용 중인지
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
        if (users[i].pid == child_pid) return i; //일치하는게 있으면 해당 idx return
    }
    return -1;
}

int find_user_idx_by_nickname(const char *nickname){
    for (int i=0; i < MAX_CLIENT; i++){
        if (users[i].is_activated && strcmp(users[i].nickname, nickname) == 0){
            return i;
        }
    }
    return -1;
}

/* sender_idx로 속해있는 room_idx 찾기*/
int find_room_idx_by_sender_idx(int sender_idx){
    for (int i = 0; i < MAX_ROOM; i++){
        if (users[sender_idx].room_idx != -1 && (users[sender_idx].room_idx == rooms[i].room_idx)){
            return i;
        } 
    }
    return -1;
}

void execute_command(int sender_idx, ParsedCommand cmd);
void handle_broadcast(int sender_idx, const char *msg);
void handle_whisper(int sender_idx, const char *target, const char *msg);
void handle_join(int sender_idx, const char *room, const char *msg);
void handle_leave(int sender_idx);
void handle_add(int sender_idx, const char *room);
void handle_rm(int sender_idx, const char *room);
void handle_list(int sender_idx);
void handle_users(int sender_idx);
void handle_unknown(int sender_idx);


#endif