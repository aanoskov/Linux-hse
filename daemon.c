#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>

int sig_flag = 0;
int exit_flag = 0;
int child_flag = 0;
sem_t *sem;

void sigint_handler(int signum)
{
        sig_flag = signum;
        //printf("It's daemon handler.\n");
}

void sigterm_handler(int signum)
{
	exit_flag = 1;	
}

void sigchld_handler(int signum)
{
	child_flag = 1;
}

void writeLog(char *text)
{
	int ld = open("log.txt", O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
	write(ld, text, strlen(text));
	close(ld);
}

void execute(char **com)
{
	int fd_out = open("output.txt", O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
	dup2(fd_out, 1);		
	close(fd_out);
	execv(com[0], com);
}

int Daemon(char *argv[])
{
	int shmem;
	if ((shmem = shm_open("newshm", O_CREAT|O_RDWR, S_IRWXU)) == -1)
	{
		writeLog("Shared memory object creation error\n");
		exit(1);
	}

	if (ftruncate(shmem, sizeof(sem_t)) < 0)
	{
		writeLog("ftruncate error\n");
		exit(1);
	}
		
	sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, shmem, 0);
	if(sem_init(sem, 0, 1) == -1)
	{
		writeLog("Semaphore initialization error\n");
		exit(1);
	}
        int tmp;
	signal(SIGINT, sigint_handler); //sig 2
	signal(SIGTERM, sigterm_handler);//sig 15
	signal(SIGCHLD, sigchld_handler);
        
	
	
	while(!exit_flag)
	{
		int tmp = pause();
		if(sig_flag == 2){
			writeLog("Daemon got signal 2(sigint)\n");
			int count;
			char buf[65535] = "";
			int fd_input = open(argv[1], O_RDWR, S_IRWXU);
			count = read(fd_input, buf, sizeof(buf));
			
			//char* delimiter = "\n";
			char* current = strtok(buf, "\n");
			char *programs[255];
			int progs_size = 0;
			do{
				programs[progs_size] = current;
				programs[progs_size][strlen(current)] = '\0';
				progs_size++;
			}while((current=strtok(NULL, "\n")) != NULL);
			programs[progs_size] = NULL;
				
			for(int i = 0; i < progs_size; i++)
			{
				pid_t par_pid;
				if ((par_pid = fork()) == 0)
				{
					char *line[255];
					int length = 0;
					char* arg;
					//char *delimiter2 = " ";
					
					arg = strtok(programs[i], " ");
					do{
						line[length] = arg;
						line[length][strlen(arg)]='\0';				
						length++;
					}while((arg=strtok(NULL, " ")) != NULL);
					line[length] = NULL;					
						
					if(sem_wait(sem) == -1)
						writeLog("Error at semaphore decrement operation\n");	
					else
					{
						char msg[255] = "Command execution completed(";
						strcat(msg, line[0]);
						strcat(msg, ")\n");
						writeLog(msg);
						execute(line);
					}
					 
				}
				else if (par_pid > 0)
				{
					while (1)
					{
						pause();

						if (child_flag)
						{
							waitpid(-1, NULL, 0);
							sem_post(sem);
							child_flag = 0;
							break;
						}
					}
				}
				else 
				{
					writeLog("Fork error\n");
				}
			}
			close(fd_input);
			sig_flag = 0;
		}
	}
	//close(fd_input);
	shm_unlink("newshm");
	sem_destroy(sem);
	writeLog("Daemon stopped working\n");
	exit(0);
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

