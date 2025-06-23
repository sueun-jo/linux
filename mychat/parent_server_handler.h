#ifndef PARENT_SERVER_HANDLER_H
#define PARENT_SERVER_HANDLER_H

#include "resource.h"

void handle_sigusr1(int sig);
int find_user_idx_by_pid(pid_t pid);
void run_parent_handler();

#endif
