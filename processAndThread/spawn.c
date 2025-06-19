#include <stdio.h>
#include <sys/wait.h>
#include <spawn.h>

extern char **environ;

int system(const char *cmd) //fork(), exec(), waitpid() 함수 사용
{
	pid_t pid;
	int status;
	char *argv[] = {"sh", "-c", cmd, NULL};
	
	posix_spawn (&pid, "/bin/sh", NULL, NULL, argv, environ);

	waitpid(pid, &status, 0);

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
