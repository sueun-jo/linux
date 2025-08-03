#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdio.h>
#include <string.h>

#define MAX_CMD_LEN 8
#define MAX_NAME_LEN 64
#define MAX_MSG_LEN 1024

#define NUM_CMD 11

/* ENUM 구조체 Command */
typedef enum { 
    
    CMD_UNKNOWN,     //0
    CMD_BROADCAST,   //1
    CMD_WHISPER,     //2
    CMD_ADD,         //3
    CMD_JOIN,        //4
    CMD_LEAVE,       //5
    CMD_RM,          //6
    CMD_LIST,        //7
    CMD_USERS,       //8
    CMD_WHERE,       //9
    CMD_QUIT,        //10
    CMD_HELP         //11
    
} Command;

typedef struct {
    Command type;               // cmd_type
    char target[MAX_NAME_LEN];  // target :user or room
    char msg[MAX_MSG_LEN];      // real 메시지 본문
} ParsedCommand;


extern const char* commandStr[];
extern const char* commandMan[];

ParsedCommand parse_command(const char *input); //return타입이 PasredCommand겠죠?

#endif  