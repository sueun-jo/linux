#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdio.h>
#include <string.h>

#define MAX_CMD_LEN 8
#define MAX_NAME_LEN 64
#define MAX_MSG_LEN 1024

/* ENUM 구조체 Command */
typedef enum { 
    
    CMD_UNKNOWN,     //0
    CMD_BROADCAST,   //1
    CMD_WHISPER,     //1
    CMD_ADD,         //2
    CMD_JOIN,        //3
    CMD_LEAVE,       //4
    CMD_RM,          //5
    CMD_LIST,        //6
    CMD_USERS        //7

} Command;

typedef struct {
    Command type;               // cmd_type
    char target[MAX_NAME_LEN];  // target :user or room
    char msg[MAX_MSG_LEN];      // real 메시지 본문
} ParsedCommand;

ParsedCommand parse_command(const char *input); //return타입이 PasredCommand겠죠?

#endif