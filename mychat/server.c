#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "server.h"
#include "debug.h"
#include "protocol.h"

#define SERVER_PORT 5432
#define BUFSIZE 1024

int from_child_to_parent[2];
int from_parent_to_child[2];
int client_socket;

/* zombie 방지 시그널 핸들러 */
void sig_child (){
    int status;
    pid_t pid;
    while ( (pid = waitpid(-1, &status, WNOHANG)) > 0){ //자원 회수 : pid에 종료된 자식 pid 담김
        int idx = find_user_idx_by_pid(pid);

        if (idx>=0) { //idx번째 구조체 값 초기화로 release 
            memset(&users[idx], 0, sizeof(ClientInfo));
            dprint("%d번째 cient 해제\n", idx);
        } else {eprint("no child pid %d\n", pid);}
    }
}
/* sig_usr1 : 부모 server do */
void sig_usr1(int signo, siginfo_t *info, void *context) { 
    dprint("parent server do sig_usr1\n");

    pid_t sender_pid = info->si_pid;
    int sender_idx = find_user_idx_by_pid(sender_pid);
    if (sender_idx < 0) {
        eprint("cannot find sender pid\n");
        return;
    }
    
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    int n = read(users[sender_idx].from_child_to_parent[PIPE_READ], buf, BUFSIZE-1);
    //자식이 쓴 걸 읽으면 n
    if (n<=0){
        eprint("noting to read from pipe\n");
    } else if (n > 0) {
        buf[n] = '\0';

        /* 첫 수신 메시지는 닉네임으로 처리 */
        if (users[sender_idx].nickname[0] == '\0'){
            /*중복 확인 로직 시작*/
        for (int i = 0; i < MAX_CLIENT; i++){
            if (i != sender_idx && users[i].is_activated && strcmp(users[i].nickname, buf)==0){
                dprint("닉네임 중복\n");
                char reject_msg[] = "[Info] This nickname is already exists. Plz enter a different one.\n";
                write (users[sender_idx].from_parent_to_child[PIPE_WRITE], reject_msg, strlen(reject_msg));
                kill(users[sender_idx].pid, SIGUSR2);
                return;
            }
        }
        dprint("nickname set : nickname [%s]\n", buf);
        strncpy(users[sender_idx].nickname, buf, MAX_NAME_LEN -1);
        dprint("user[%d].nickname is %s\n", sender_idx, users[sender_idx].nickname);
        
        return;
        }
        /* *************************** */

        ParsedCommand cmd = parse_command(buf); //parse_command : cmd 종류 나누기
        execute_command(sender_idx, cmd);
    } 
}

void sig_usr2(){ // 자식 server do
    dprint("client server do sig_usr2\n");
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    dprint("now user_idx %d\n", user_idx);
    int n = read (from_parent_to_child[PIPE_READ], buf, BUFSIZE-1);
    if (n<=0){
        eprint("read erorr\n");
    } else {
        int n = send (client_socket, buf, strlen(buf), 0);
        if (n<0){
            perror("send error");
        } else {
            dprint("send to client : success\n");
        }
    }
}

int main (int argc, char **argv){
    int listen_socket; //listen_socket은 연결대기용, client_socket(전역)은 client와 통신용
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    char buf [BUFSIZE];
    pid_t pid;
    
    /* rooms 구조체 배열 room_idx 수정 */
    for (int i = 0; i < MAX_ROOM; i++){
            rooms[i].room_idx = -1;
    }

    listen_socket = socket (AF_INET, SOCK_STREAM, 0); //tcp socket 통신 할게요
    if (listen_socket < 0){
        perror ("socket error : ");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* bind로 주소 설정 */
    if (bind (listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error : ");
        return -1;
    }

    /* 동시 접속 클라이언트 처리 대기 큐 listen */
    listen (listen_socket, MAX_CLIENT); 
    printf("[Server Info] Server is Listening on Port : %d ... \n", SERVER_PORT);
    
    while (1) {
        /* accept : 클라이언트가 요청해서 생긴 socket이기에 client_socket이라 명명 */
        /* accept하면 서버:클라이언트 1:1 관계를 만들어야 하니까 fork() 해야됨 */
        client_socket = accept (listen_socket, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0){
            if (errno == EINTR) continue;  // 시그널 때문에 끊긴 건 무시하고 다시 시도
            perror ("accept error");
            continue;
        }
        
        user_idx = find_empty_user_slot();

        if ( user_idx == -1 ) {
            printf("[Info] Connection Rejected : Too many clients\n");
            close (client_socket);
            continue;
        }
        
        //pipe 생성
        pipe (from_child_to_parent); // 자식 -> 부모 : 자식 write, 부모 read
        pipe (from_parent_to_child); // 부모 -> 자식 : 부모 write, 자식 read

        pid = fork(); // 부모 - 자식 나눠짐

        if (pid == 0){ //자식 프로세스
            signal (SIGUSR2, sig_usr2); // sig_usr2는 자식do
            close (listen_socket); //자식은 listen_socket 필요 없음 : accept은 부모만 한다
    
            //자식은 부모한테 write만 하면 됨, read 필요 없음
            close (from_child_to_parent[PIPE_READ]); // 자식은 부모한테 write only, read 필요X
            close (from_parent_to_child[PIPE_WRITE]); // 부모한테 read only, write 필요X

            while(1) {
                memset(buf, 0, BUFSIZE);
                int n = recv (client_socket, buf, BUFSIZE-1, 0);
                if (n<=0) {
                    eprint("recv error : nothing to read\n");
                    break;
                } else {
                    buf[n] = '\0'; //문자열 처리
                    dprint("recv msg from client\nmsg : %s\n", buf);
                    write (from_child_to_parent[PIPE_WRITE], buf, strlen(buf));
                    dprint("send msg from child server to parent server\n");
                    kill (getppid(), SIGUSR1); //쓰고 나서 부모한테 SIGUSR1 전달 
                }
            }
            exit(0);
        } else if (pid > 0){ // 부모 프로세스

            /* sigaction등록 */
            struct sigaction sa;
            sa.sa_sigaction = sig_usr1;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);

            signal (SIGCHLD, sig_child); 
            
            close (from_parent_to_child[PIPE_READ]); //부모는 자식한테 write, read필요 없음
            close (from_child_to_parent[PIPE_WRITE]); //부모는 자식으로부터 read only, write 필요 X
            
            /* UserInfo 구조체 users setter */
            users[user_idx].pid = pid; //자식pid를 부모가 갖고 있다
            dprint("users[%d] = %d\n", user_idx, pid);
            users[user_idx].client_socket_fd = client_socket;
            users[user_idx].from_parent_to_child[PIPE_WRITE] = from_parent_to_child[PIPE_WRITE];
            users[user_idx].from_child_to_parent[PIPE_READ] = from_child_to_parent[PIPE_READ];
            users[user_idx].is_activated = 1; 
            users[user_idx].room_idx = -1; //rid -1로 초기화
            
            printf("[INFO] New Client [# %d] is Connected.\n", user_idx);
            dprint("PID : %d, idx : %d\n", users[user_idx].pid, user_idx);
            
        } else {
            perror ("fork error : ");
            close (client_socket);
        }
    }

    close (listen_socket);
    return 0;
}

void execute_command(int sender_idx, ParsedCommand cmd){
    switch (cmd.type){
        case CMD_BROADCAST:
            handle_broadcast(sender_idx, cmd.msg);
            break;
        case CMD_WHISPER:
            handle_whisper(sender_idx, cmd.target, cmd.msg);
            break;
        case CMD_JOIN:
            handle_join(sender_idx, cmd.target, cmd.msg);
            break;
        case CMD_LEAVE:
            handle_leave(sender_idx);
            break;
        case CMD_RM:
            handle_rm (sender_idx, cmd.target);
            break;
        case CMD_ADD:
            handle_add (sender_idx, cmd.target);
            break;
        case CMD_LIST:
            handle_list(sender_idx);
            break;
        case CMD_USERS:
            handle_users(sender_idx);
            break;
        default: CMD_UNKNOWN;
            handle_unknown(sender_idx);
            break;
    }
}

void handle_broadcast(int sender_idx, const char *msg){
    // broadcast: 나 빼고 모두에게
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (users[i].is_activated && (i != sender_idx) /* && users[i].room_idx == user[send_idx].room_idx*/) {
            char msg_with_nick[BUFSIZE];
            memset(msg_with_nick, 0, BUFSIZE);
            //닉네임이랑 메시지 붙이기
            snprintf(msg_with_nick, BUFSIZE, "[%s]: %s\n", users[sender_idx].nickname, msg);
            write(users[i].from_parent_to_child[PIPE_WRITE], msg_with_nick, strlen(msg_with_nick));
            kill(users[i].pid, SIGUSR2);
        }
    }
}

void handle_whisper(int sender_idx, const char *target, const char *msg){
    int recv_idx = find_user_idx_by_nickname(target);
    
    if (recv_idx == -1){ //못찾은 경우
        char whisper_err[BUFSIZE];
        memset(whisper_err, 0, BUFSIZE);
        snprintf(whisper_err, BUFSIZE, "[ERR] No User named [%s]\n", target);
        write (users[sender_idx].from_parent_to_child[PIPE_WRITE], whisper_err, strlen(whisper_err));
        kill (users[sender_idx].pid, SIGUSR2);
        return;
    } else if (recv_idx == sender_idx){ // 귓속말 보내는 상대가 나인 경우
        char whisper_err[] = "[ERR] Cannot /whisper to yourself\n";
        write (users[sender_idx].from_parent_to_child[PIPE_WRITE], whisper_err, strlen(whisper_err));
        kill (users[sender_idx].pid, SIGUSR2);
        return;
    }
    
    char whisper_msg[BUFSIZE];
    memset(whisper_msg, 0, BUFSIZE);
    snprintf(whisper_msg, BUFSIZE, "[%s님의귓속말]: %s\n", users[sender_idx].nickname, msg);
    dprint("[%s님의귓속말]: %s\n", users[sender_idx].nickname, msg);
    write (users[recv_idx].from_parent_to_child[PIPE_WRITE], whisper_msg, strlen(whisper_msg));
    kill(users[recv_idx].pid, SIGUSR2);
    return;
}

void handle_join(int sender_idx, const char *room_name, const char *msg){
    /* 방 있는지 확인 */
    for (int i = 0; i< MAX_ROOM; i++){
        if (rooms[i].is_activated && (strcmp (rooms[i].room_name, room_name) == 0)){
            users[sender_idx].room_idx = i; //user의 room_idx 해당값으로 변경
            rooms[i].mem_cnt++; //mem_cnt 증가
            char join_msg[BUFSIZE];
            memset(join_msg, 0, BUFSIZE);
            snprintf(join_msg, BUFSIZE, "You joined room named [%s].\n", rooms[i].room_name); 
            dprint("user [%s] joined room [%s].\n",users[sender_idx].nickname, rooms[i].room_name);
            write (users[sender_idx].from_parent_to_child[PIPE_WRITE], join_msg, strlen(join_msg));
            kill (users[sender_idx].pid, SIGUSR2);
            return;
        }
    }
    char join_err[BUFSIZE];
    memset(join_err, 0, BUFSIZE);
    snprintf(join_err, BUFSIZE, "[ERR] No Room named [%s]\n", room_name);
    write (users[sender_idx].from_parent_to_child[PIPE_WRITE], join_err, strlen(join_err));
    kill (users[sender_idx].pid, SIGUSR2);
    return;
}
void handle_leave(int sender_idx){

    dprint("not implemented yet\n");
}
/*방 추가하는 /add [방이름] 함수*/
void handle_add(int sender_idx, const char *room_name){
    
    /* 방 이름 중복 검사 */
    for (int i = 0; i< MAX_ROOM; i++){
        if (rooms[i].is_activated && (strcmp (rooms[i].room_name, room_name) == 0)){
            char add_err[] = "[ERR] Room Already Exists.\n";
            write (users[sender_idx].from_parent_to_child[PIPE_WRITE], add_err, strlen(add_err));
            kill (users[sender_idx].pid, SIGUSR2);
            return;
        }
    }

    /* 빈 방 찾기 */
    for (int i = 0; i < MAX_ROOM; i++){
        if (rooms[i].is_activated != 1){ //활성화 안돼있으면
            /* 방 정보 등록 : rooms setter*/
            strncpy(rooms[i].room_name, room_name, sizeof(rooms[i].room_name)-1 );
            rooms[i].room_idx = i;
            rooms[i].mem_cnt = 0; // join 할 때 cnt++ 할거임
            rooms[i].is_activated = 1; //활성화
            dprint("방 생성 완료\n");
            char msg[BUFSIZE];
            memset(msg, 0, BUFSIZE);
            snprintf(msg, BUFSIZE, "room named [%s] is created.\n", rooms[i].room_name);
            write(users[sender_idx].from_parent_to_child[PIPE_WRITE], msg, strlen(msg));
            kill(users[sender_idx].pid, SIGUSR2);
            return;
        }
    }
}

/* 방을 remove하는 /rm [방이름] 함수 */
void handle_rm(int sender_idx, const char *room_name){
    for (int i = 0; i < MAX_ROOM; i++){
        if (strcmp (rooms[i].room_name, room_name) == 0 && rooms[i].is_activated){

            int removed_room_idx = i;

            for (int j = 0; j < MAX_CLIENT; j++){
                if (users[j].room_idx == removed_room_idx){ //room_idx와 users[i].room_idx가 일치할때
                    users[j].room_idx = -1;
                }
            }
            char msg[BUFSIZE];
            memset(msg, 0, BUFSIZE);
            snprintf(msg, BUFSIZE, "room named [%s] is removed.\n", rooms[i].room_name);
            /* rooms[i] 초기화 */
            memset(&rooms[i], 0, sizeof(RoomInfo)); //rooms[i] = {0}; C++스타일의 초기화, C에서는 불가능
            rooms[i].room_idx = -1;           
            write(users[sender_idx].from_parent_to_child[PIPE_WRITE], msg, strlen(msg));
            kill(users[sender_idx].pid, SIGUSR2);
            return;
        }
    }

    //방을 못찾은 경우
    char rm_err[BUFSIZE];
    snprintf(rm_err, BUFSIZE, "[ERR] No room named [%s].\n", room_name);
    write(users[sender_idx].from_parent_to_child[PIPE_WRITE], rm_err, strlen(rm_err));
    kill(users[sender_idx].pid, SIGUSR2);
    return;
}

/* 방 목록 보여주는 handle_list */
void handle_list(int sender_idx){
    char list_msg[BUFSIZE];
    memset (list_msg, 0, BUFSIZE);
    int found = 0;
    strncat (list_msg, "[Room List]\n", BUFSIZE - strlen(list_msg) - 1);

    for (int i = 0; i < MAX_ROOM; i++){
        if (rooms[i].is_activated == 1)
            {
                strncat(list_msg, "- ", BUFSIZE - strlen(list_msg) -1);
                strncat(list_msg, rooms[i].room_name, BUFSIZE - strlen(list_msg) -1);
                strncat(list_msg, "\n", BUFSIZE - strlen(list_msg) -1);
                
                found = 1;
            }
    }
    
    if (!found){
        strncat(list_msg, "No active rooms.\n", BUFSIZE - strlen(list_msg) -1);
    }

    write (users[sender_idx].from_parent_to_child[PIPE_WRITE], list_msg, strlen(list_msg));
    kill (users[sender_idx].pid, SIGUSR2);
    return;
}

/* 해당 방에 있는 모든 사용자 목록 보여주는 /users 함수 */
void handle_users(int sender_idx){
    dprint("현재 방에 있는 users를 보여줍니다\n");
    int now_room = find_room_idx_by_sender_idx(sender_idx);
    for (int i = 0; i < MAX_CLIENT; i++){
        if (users[i].is_activated && (users[i].room_idx == now_room)){
        
        }
    }

}

void handle_unknown(int sender_idx){
    char unknown_msg[] = "[Err] Wrong Command\n";
    dprint("unknown cmd\n");
    write (users[sender_idx].from_parent_to_child[PIPE_WRITE], unknown_msg, strlen(unknown_msg));
    kill(users[sender_idx].pid, SIGUSR2);
    return;
}
