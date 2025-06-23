#ifndef CHILD_SERVER_HANDLER_H
#define CHILD_SERVER_HANDLER_H

#include "resource.h"

void run_child_handler(int client_socket_fd, int pipe_from_parent, int pipe_to_parent);
void handle_sigusr2 (int sig);
#endif
