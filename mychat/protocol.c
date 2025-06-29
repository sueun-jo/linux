#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "debug.h"

ParsedCommand parse_command (const char *input) {
    
    /* 구조체 초기화 */
    ParsedCommand result;
    memset (&result, 0, sizeof(result));
    result.type = CMD_UNKNOWN;
    /* *********** */

    if (input[0] != '/'){ // '/'로 시작하지 않으면 
        result.type = CMD_BROADCAST; // 일반 채팅
        strncpy(result.msg, input, MAX_MSG_LEN -1 ); //result.msg에 복사함
        return result;
    }

    char cmd[16] = {0};
    char arg1[32] = {0};
    char arg2[1024] = {0};
    
    int matched = sscanf(input, "/%s %s", cmd, arg1);
    char *start = strstr(input, arg1); //arg1(target)이 시작하는 위치
    if (!start) return result; 
    
    start += strlen(arg1); //arg1만큼 다음칸으로 감
    while (*start == ' ') start++; //공백이면 한 칸 더 감

    if ( (strcmp(cmd, "whisper") == 0 || strcmp(cmd, "w") == 0) && matched >= 2) {
        dprint("cmd: %s, target: %s, msg: %s\n", cmd, arg1, arg2);
        result.type = CMD_WHISPER;
        strncpy(result.target, arg1, MAX_NAME_LEN - 1);
        strncpy(result.msg, start, MAX_MSG_LEN - 1); //start 메시지 시작 지점
        dprint("parsing result : %s / %s / %s / matched:%d\n",cmd, arg1, start, matched);
    } else if (strcmp(cmd, "add") == 0 && matched >= 2) {
        result.type = CMD_ADD; 
        strncpy(result.target, arg1, MAX_NAME_LEN - 1);
    } else if (strcmp(cmd, "join") == 0 && matched >= 2) {
        result.type = CMD_JOIN;        
        strncpy(result.target, arg1, MAX_NAME_LEN - 1);
    } else if (strcmp(cmd, "leave") == 0) {
        result.type = CMD_LEAVE;       
    } else if (strcmp(cmd, "rm") == 0 && matched >= 2) {
        result.type = CMD_RM;         
        strncpy(result.target, arg1, MAX_NAME_LEN - 1);
    } else if (strcmp(cmd, "list") == 0) {
        result.type = CMD_LIST;
    } else if (strcmp(cmd, "users") == 0) {
        result.type = CMD_USERS;
    } else {
        result.type = CMD_UNKNOWN;
    }
    return result;
}