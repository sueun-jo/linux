#include <stdio.h>
#include <sys/wait.h>
#include <spawn.h>

extern char **environ;

int system(const char *cmd) //fork(), exec(), waitpid() 함수 사용
{
	pid_t pid;
	int status;

	//posix_spqwn() 함수 속성을 이용하면waitpid()없이
	//비동기적 프로그래밍 가능
	posix_spawn_file_actions_t actions; 
	posix_spawnattr_t attrs;
	char *argv[] = {"sh", "-c", cmd, NULL};
	
	posix_spawn_file_actions_init(&actions);
	posix_spawnattr_init(&attrs);
	posix_spawnattr_setflags(&attrs, POSIX_SPAWN_SETSCHEDULER);

	posix_spawn (&pid, "/bin/sh", &actions, &attrs, argv, environ);

	return status;
}

int main (int argc, char **argv, char **envp)
{
	while (*envp)
		printf("%s\n", *envp++);

	system ("who"); //who utility 수행
	system ("nocommand"); // 오류 사항의 수행
	system ("cal"); //cal utility 수행

	return 0;
}
