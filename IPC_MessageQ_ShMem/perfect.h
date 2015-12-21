/* Homework for CS450
 * Fall 2015 - Yichu Lin
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <setjmp.h>

#define KEY 	(key_t)34663	// Last 5 digit of phone number
#define COMP_REQ 	3
//#define RPRT_REQ	2
//#define KILL_REQ	3	// Report w/ kill
#define MNGR_INQ	1
#define PEFT_REQ	2
#define TERM_SIG	4

#define ARCH_NUM	4

typedef struct msg_type{
	long type;
	pid_t pid;
	int msg;
}	Msg;

typedef struct process_entry {
	pid_t pid;
	int test;
	int skip;
	int found;
}	Proc;

#define PTABLE_SIZE 20
#define PERF_ARR_SIZE 20
#define BITMAP_SIZE 32000
typedef struct share_mem_seg{
	pid_t manager;
	char bitmap[BITMAP_SIZE];
	int perfect[PERF_ARR_SIZE];
	Proc procs[PTABLE_SIZE];
}	Shmseg;

sigset_t sigmask;
//sigjmp_buf jmpenv;
struct sigaction sigact;
int msgd = -1;
int shmid = -1;
Shmseg* panel;

int split(char *s, char *argv[], int maxargs)
{
        int i = 0;

        while (*s && i < maxargs)
        {
                while (*s && isspace(*s))
                        *s++ = 0;

                if (!*s)
                        break;

                argv[i++] = s;

                while (*s && !isspace(*s))
                        s++;
        }

        return i;
}

int find_pid(Proc* ptable, pid_t target){
	int i;
	for(i = 0; i < PTABLE_SIZE; i++){
		if(ptable[i].pid == target)
			return i;
	}
	return -1;
}
