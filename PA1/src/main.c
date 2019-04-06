#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h> // open()
// file redirection: >
int fileOut(char * file, char* string) { 
	// O_TRUNC 옵션으로 덮어쓰기 
    int fdout = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
    dup2(fdout, STDOUT_FILENO); // file에 stdout 역할 할당
	if(strlen(string) > 0) {
		printf("%s\n", string); // file에 string 쓰기
	}
    close(fdout);
}
// file redirection: <
int fileIn(char *file) {
	int fdin = open(file, O_RDONLY | O_CREAT, 0644);
	dup2(fdin, STDIN_FILENO); // file에 stdin 역할 할당 (파일 읽기)
	close(fdin);
}
int main()
{
	pid_t pid; 
	char project_path[4096]; 
	getcwd(project_path, 4096); // 프로젝트 경로 저장(python script 실행위해 쓰임)
	char buf[4096];
	while (fgets(buf, 4096, stdin)) {
		buf[strlen(buf)-1] = '\0'; // fgets로 받으면 '\n' 개행문자까지 받기에 개행문자 지우는 역할
		if (strncmp(buf, "pwd", 3) == 0) {
			fflush(stdin); // 입력 버퍼 비워주기
			pid = fork();
			int status;
			if (pid == -1) {
				fprintf(stderr, "Error occured during process creation\n");
				exit(EXIT_FAILURE);
			} else if (pid == 0) {
				char working_directory_name[4096];
				getcwd(working_directory_name, 4096); // 현재 디렉터리 경로 얻기
				if(strcmp(buf, "pwd") == 0) {
					printf("%s\n", working_directory_name);
				} else if(strncmp(buf, "pwd >", 5) == 0) {
					char *token = strtok(buf, " >\n");
					token = strtok(NULL, " >\n");
					fileOut(token, working_directory_name);
				}
				exit(EXIT_SUCCESS);
			} else {
				wait(&status);
			}
		} else if (strncmp(buf, "ls", 2) == 0 || strncmp(buf, "/bin/ls", 7) == 0) {
			fflush(stdin);
			pid = fork();
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				if (strncmp(buf, "ls >", 4) == 0) {
					char *token = strtok(buf, " >\n");
					token = strtok(NULL, " >\n");
					fileOut(token, "");
				} 
				execlp("/bin/ls", "ls", NULL);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL); // 자식 프로세스 기다리기
			}
		} else if (strncmp(buf, "cd", 2) == 0) {
			char *token = strtok(buf, " \n");
			token = strtok(NULL, " \n");
			if(strcmp(token, "~") == 0) {
				strcpy(token, getenv("HOME")); // 환경변수 HOME의 값 얻기
			} else if(strncmp(token, "~", 1) == 0) {
				char path[4096];
				strcpy(path, getenv("HOME"));
				// ~를 HOME 경로로 바꿔주고 기존 경로에 붙이는 작업
				int len = strlen(token) + strlen(path);
				int path_len = strlen(path);
				for (int i = 0; i < len - 1; i++)
				{
					path[path_len + i] = token[i + 1];
				}
				path[path_len + len] = '\0';
				strcpy(token, path);
			}
			if (chdir(token)) // 현재 경로 변경하기
			{
				fprintf(stderr, "Change error\n");
			}
		} else if (strncmp(buf, "~/", 2) == 0) {
			fflush(stdin);
			pid = fork();
			int status;
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				char path[4096];
				strcpy(path, getenv("HOME"));
				// ~를 HOME 경로로 바꿔주고 기존 경로에 붙이는 작업
				int len = strlen(buf) + strlen(path);
				int path_len = strlen(path);
				for (int i = 0; i < len - 1; i++)
				{
					if(buf[i+1] != '\n') {
					  path[path_len + i] = buf[i + 1];
					}
				}
				path[path_len + len] = '\0';
				execl(path, path, NULL);
				exit(EXIT_SUCCESS);
			} else {
				waitpid(pid, &status, 0);
			}
		} else if (strncmp(buf, "echo", 4) == 0) {
			fflush(stdin);
			pid = fork();
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				char value[500];
				char *token;
				if(strncmp(buf, "echo $", 6) == 0) {	// 환경변수 값 처리			
					token = strtok(buf, " \n");
					token = strtok(NULL, "$\n");
					strcpy(value, getenv(token));
				} else {
					char *find_ch = strchr(buf, '>');
					token = strtok(buf, " \n");
					token = strtok(NULL, "\"\n"); // ""가 포함된 문자열에서 "" 제거
					strcpy(value, token);
					if(find_ch != NULL) {
						token = strtok(NULL, "> \n");
						fileOut(token, value);
					}
				}
				execl("/bin/echo", "echo", value, NULL);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL);
			}
		} else if(strncmp(buf, "make", 4) == 0) {
			fflush(stdin);
			pid = fork();
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				if(strcmp(buf, "make") == 0) {
					execl("/usr/bin/make", "make", NULL);
				} else if(strcmp(buf, "make run") == 0) {
					execl("/usr/bin/make", "make", "run", NULL);
				} else if(strcmp(buf, "make test") == 0) {
					execl("/usr/bin/make", "make", "test", NULL);
				}
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL); 
			}
		} else if(strncmp(buf, "grep", 4) == 0) {
			fflush(stdin);
			pid = fork();
			char *token;
			char value[1024];
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				char *find_ch = strchr(buf, '<');
				token = strtok(buf, " \n");
				token = strtok(NULL, " \n");
				strcpy(value, token);
				if(find_ch != NULL) {
					token = strtok(NULL, "< \n");
					fileIn(token);
				}
				execlp("/bin/grep", "grep", value, NULL);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL);
			}
		} else if(strcmp(buf, "ready-to-score ./2019-1-PA0/") == 0) {
			fflush(stdin);
			pid = fork();
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {	
				strcat(project_path, "/scripts/ready-to-score.py");
				execlp("python3", project_path, project_path, "./2019-1-PA0", NULL);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL);
			}
		} else if(strcmp(buf, "auto-grade-pa0 ./2019-1-PA0/") == 0) {
			fflush(stdin);
			pid = fork();
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {	
				strcat(project_path, "/scripts/auto-grade-pa0.py");
				execlp("python3", project_path, project_path, "./2019-1-PA0", NULL);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL);
			}
		} else if(strcmp(buf, "report-grade ./2019-1-PA0/") == 0) {
			fflush(stdin);
			pid = fork();
			if(pid == -1) {
				fprintf(stderr, "Error occured\n");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {	
				strcat(project_path, "/scripts/report-grade.py");
				execlp("python3", project_path, project_path, "./2019-1-PA0", NULL);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL);
			}
		} else if(strncmp(buf, "exit", 4) == 0) { // 종료 명령어
			exit(0);
		}
		else {
			printf("I don't know what you said: %s\n", buf);
		}
	}
	return 0;
}
