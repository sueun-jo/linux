#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "resource.h"
#include "debug.h"


extern UserInfo users[MAX_CLIENT];
extern RoomInfo rooms[MAX_ROOM];

char parent_buf [BUFSIZE];

/*parent_server_handler는 자식->부모 USR1을 받으면 처리 해야 되는 게 있다*/
/* 그리고 나서 자식한테 USR2 시그널을 보내줘야함 */

// SPECIFIC USER IDX RETURN 
int find_user_idx_by_pid(pid_t pid) {
    for (int i=0; i<MAX_CLIENT; i++){
        if (users[i].is_activated && users[i].pid == pid) return i;
    }
    return -1;
}

//부모가 SIGUSR1 (자식->부모) 시그널을 받으면 : 자식이 pipe로 메시지 전달한 거 읽음
void handle_sigusr1 (int sig){
    dprint("sigusr1\n"); //sentence for debug
    for (int i=0; i<MAX_CLIENT; i++){
        if (!users[i].is_activated) continue; //is_activated가 없으면 pass

        memset (parent_buf, 0, BUFSIZE); //parent_buf 초기화 
        int n = read(users[i].pipe_to_parent[PIPE_READ], parent_buf, BUFSIZE-1 );
        if (n>0){ //읽을 게 있으면
            parent_buf[n] = '\0'; //문자열 끝 처리 
            dprint("[parent server] Received from child %d : %s", users[i].pid, parent_buf);

            //to do : 명령어 파싱 및 처리
            //ex /join 2, /w sueun hi 

            //test용 echo 
            write(users[i].pipe_to_child[PIPE_WRITE], parent_buf, n);
            dprint("make sigusr2");
            kill (users[i].pid, SIGUSR2); //child한테 메시지 보냈으니까 읽으라고 함
        }
    }
}

void run_parent_handler(){
    while(1) pause(); //시그널 올때까지 대기
}