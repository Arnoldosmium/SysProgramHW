/* Homework for CS450
 * Fall 2015 - Yichu Lin
 */

#include "perfect.h"

Proc* my_entry;
int my_idx;
char* bitmap;

Msg buff;

pid_t my_pid;

int isPerfect(int n){
	int i=2;
	int sum = 1;
        for (;i <= n/2+1;i++)		
                if (!(n%i)) sum+=i;

        return (sum==n);
}

void exit_hdl(int signal){
	if(panel){
		memset(&panel->procs[my_idx], 0, sizeof(Proc));
	}
	if(shmid != -1)	
		shmdt(panel);
	if(msgd != -1){
		buff.type = TERM_SIG;
		buff.pid = getpid();
		buff.msg = -1;
		msgsnd(msgd, &buff, sizeof(Msg), IPC_NOWAIT);
	}
	exit(signal);
}

#define NUM_SIZE (8*sizeof(char))

int main(int argc, char** argv){
	if(argc != 2){
		printf("USAGE: %s <compute_from 2~256001>\n", argv[0]);
		exit(-1);
	}
	int start = atoi(argv[1]);
	if(start < 2){
		printf("The starting number is too small.\n");
		exit(-1);
	}
	if(start > BITMAP_SIZE*sizeof(char)*8 + 1){
		printf("The starting number is too big.\n");
		exit(-1);
	}

	//Send request to our manager
	if(( msgd = msgget(KEY, 0660) ) == -1){
		printf("Please run manage first.\n");
		exit(-1);
	}
	buff.type = COMP_REQ;
	buff.pid = my_pid = getpid();
	buff.msg = 1;

	if(msgsnd(msgd, &buff, sizeof(Msg), IPC_NOWAIT) == -1){
		perror("Report Birth");
		exit(-1);
	}
	if(msgrcv(msgd, &buff, sizeof(Msg), (long)my_pid, 0) == -1){
		perror("Manager Reply");
		exit(-1);
	}

	if(buff.msg == -1){	// There are no enough proc space
		printf("Please kill some other compute before initiate a new one.\n");
		exit(-1);
	}

	if((shmid = shmget(KEY, sizeof(Shmseg), 0660)) == -1){
		perror("Share Mem get");
		exit_hdl(-1);
	}

        sigemptyset(&sigmask);
        sigaddset(&sigmask, SIGINT);
        sigaddset(&sigmask, SIGQUIT);
        sigaddset(&sigmask, SIGHUP);

        memset(&sigact, 0, sizeof(struct sigaction) );
        sigact.sa_handler = exit_hdl;
        sigact.sa_mask = sigmask;
	sigact.sa_flags = 0;

        sigaction(SIGINT, &sigact, NULL);
        sigaction(SIGQUIT, &sigact, NULL);
        sigaction(SIGHUP, &sigact, NULL);

	panel = (Shmseg*) shmat(shmid, NULL, 0);
	
	char* bitmap = panel->bitmap;
	Proc* my_entry = &panel->procs[my_idx = buff.msg];

	//Now start playing with numbers
	int i = start-2;
	for(; i < 256000; i++){
		char flag = 1 << i%NUM_SIZE;
		//Test the bit
		if(bitmap[i/NUM_SIZE] & flag){	//The bit is set
			my_entry->skip++;
			continue;
		}
		//Mark the bit
		bitmap[i/NUM_SIZE] |= flag;
		//Do the computation
		if(isPerfect(i+2)){	//Report
			buff.type = PEFT_REQ;
			buff.pid = my_pid;
			buff.msg = i+2;
			my_entry->found++;
			if(msgsnd(msgd, &buff, sizeof(Msg), 0) == -1){
				perror("Report perfect");
				exit_hdl(-1);
			}
		}
		my_entry->test++;
	}
	exit_hdl(0);
}
