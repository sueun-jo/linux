#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h> //directory 조작 함수 헤더파일
#include <pwd.h> // getpwuid() 함수: uid를 이용해 사용자 이름 get
#include <grp.h> // getgrgid() 함수 : gid를 이용해 그룹 이름 get
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

int listDir(char *arg)
{
	DIR *pdir; //directory 조작을 위한 stream
	struct dirent *dirt; //directory 항목을 위한 struct
	struct stat statBuf; //파일 정보를위한 struct
	struct passwd *username; //사용자 이름 출력을 위한 변수
	struct group *groupname; //그룹 이름 출력을 위한 변수
	struct tm *t; //시간 출력을 위한 변수

	int i = 0, count = 0;
	char *dirName[255], buf [255], permission[11], mtime[20];

	memset(dirName, 0, sizeof(dirName)); //변수 초기화
	memset(&dirt, 0, sizeof(dirt));
	memset(&statBuf, 0, sizeof(statBuf));

	if ((pdir = opendir(arg))<=0) { // 해당 directory stream 열기
		perror("opendir"); 
		return -1;
	}

	chdir(arg); //directory로 이동
	getcwd(buf, 255); //현재 directory의 절대 경로 가져오고 표시
	printf("\n%s: Directory\n", arg);

	while ((dirt = readdir(pdir)) != NULL)
	{
		lstat(dirt->d_name, &statBuf); //현재 dir에 대한 정보 가져오기

		//파일 종류 검사 
		if(S_ISDIR(statBuf.st_mode))
			permission[0] = 'd';
		else if (S_ISLNK(statBuf.st_mode))		
			permission[0] = 'l';
		else if (S_ISCHR(statBuf.st_mode))
			permission[0] = 'c';
		else if (S_ISBLK(statBuf.st_mode))
			permission[0] = 'b';
		else if (S_ISSOCK(statBuf.st_mode))
			permission[0] = 's';
		else if (S_ISFIFO(statBuf.st_mode))
			permission[0] = 'P';
		else
			permission[0] = '-';

		//사용자에 대한 권한 검사
		permission[1] = statBuf.st_mode&S_IRUSR? 'r' : '-';	
		permission[2] = statBuf.st_mode&S_IWUSR? 'w' : '-';
		permission[3] = statBuf.st_mode&S_IXUSR? 'x' :
			statBuf.st_mode&S_ISUID? 'S' : '-';

		//그룹에 대한 권한 검사
		permission[4] = statBuf.st_mode&S_IRGRP? 'r' : '-';	
		permission[5] = statBuf.st_mode&S_IWGRP? 'w' : '-';
		permission[6] = statBuf.st_mode&S_IXGRP? 'x' :
			statBuf.st_mode&S_ISGID? 'S' : '-';

		//다른 사용자에 대한 권한 검사
		permission[7] = statBuf.st_mode&S_IROTH? 'r' : '-';	
		permission[8] = statBuf.st_mode&S_IWOTH? 'w' : '-';
		permission[9] = statBuf.st_mode&S_IXOTH? 'x' : '-';

		if(statBuf.st_mode & S_IXOTH)
			permission[9] = statBuf.st_mode & S_ISVTX? 't' : 'x';
		else permission[9] = statBuf.st_mode&S_ISVTX? 'T' : '-';

		permission[10] = '\0';

		if (S_ISDIR (statBuf.st_mode)==1) { //디렉토리인지 검사
			if (strcmp (dirt->d_name, ".") && strcmp (dirt->d_name, "..")){
				dirName[count] = dirt->d_name;
				count = count + 1;
			}
		}

		username = getpwuid (statBuf.st_uid); //uid에서 사용자 이름 획득
		groupname = getgrgid(statBuf.st_gid); //gid에서 그룹 이름 획득
		t = localtime(&statBuf.st_mtime); //최근 수정 시간 출력 

		//출력을 위한 서식화: tm구조체는 뒤에서
		sprintf(mtime, "%04d-%02d-%02d %02d:%02d:%02d",
				t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec);

		printf("%s %2d %s %s %9ld  %s  %s\n", permission, statBuf.st_nlink,
				username->pw_name, groupname->gr_name,
				statBuf.st_size, mtime, dirt->d_name);		
	}

	for (i=0; i<count; i++) //다른 디렉토리에 대한 재귀 호출
	{
		if (listDir(dirName[i]) == -1) break;
	}

	printf ("\n");
	closedir(pdir); //열었던 directory stream 닫는다
	chdir(".."); //원래 dir로 이동

	return 0;
}

int main (int argc, char **argv)
{
	if (argc<2){
		fprintf(stderr, "Usage: %s directory_name.\n", argv[0]); 
		return -1;
	}

	listDir(argv[1]);

	return 0;
}
