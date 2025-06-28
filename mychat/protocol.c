#include <stdio.h>
#include <string.h>
#include "protocol.h"

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
    
    int matched = sscanf(input, "/%s %s %s", cmd, arg1, arg2);

    if (strcmp(cmd, "w") == 0 && matched >= 3) {
        result.type = CMD_WHISPER; 
        strncpy(result.target, arg1, MAX_NAME_LEN - 1);
        strncpy(result.msg, arg2, MAX_MSG_LEN - 1);
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