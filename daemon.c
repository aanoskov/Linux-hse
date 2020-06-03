#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>


int flag = 0;
int exit_flag = 0;

void sigint_handler(int signum)
{
        flag = signum;
        printf("It's daemon handler.\n");
}

void sigterm_handler(int signum)
{
	exit_flag = 1;	
}

int Daemon(char *argv[])
{
        int tmp;
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigterm_handler);//sig 15
        //tmp = pause();
       	
	int fd, count;
	char buf[256] = "";
	pid_t par_pid;
	fd = open(argv[1], O_RDWR, S_IRWXU);
	count = read(fd,buf, sizeof(buf));
	buf[count-2] = '\0';
	close(1);
	dup2(fd, 1);
	//lseek(fd, 0, SEEK_END);
	//close(fd);
	while(exit_flag!=1){
		tmp = pause();
		if (flag == 2){
			if((par_pid = fork()) == 0)
				execve(buf, argv + 1, NULL);
			//printf("Wrote : %s\n", buf);
			flag = 0;
			//tmp = pause();
			//exit(0);
		}
	
        }
	close(fd);
	return 0;

}

int main(int argc, char *argv[])
{
        pid_t pid;
	
        if ((pid = fork())>0){
                exit(7);
        }
        else if(pid ==0){
		printf("Pid = %i\n", getpid());
                setsid();//--перевод нашего дочернего процесса в новую сесию
                Daemon(argv);
        }
        else{
                perror("can not fork process\n");
        }
        return 0;
}

