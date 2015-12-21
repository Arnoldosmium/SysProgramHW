/* THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
 *
 * A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yichu Lin
 *
 * */

#include "perfect.h"

int next_perfect = 0;
int compute_num = 0;

int get_entry(Proc* ptable){
	int i = 0;
	for(; i < PTABLE_SIZE; i++)
		if(ptable[i].pid <= 0)
			return i;
	return -1;
}

void new_compute(Msg* recv, Msg* cnst){
	cnst->type = (long)recv->pid;
	cnst->pid = getpid();
	int idx = get_entry((Proc*)&panel->procs);
	if( (cnst->msg = idx) > -1){
		memset(&panel->procs[idx], 0, sizeof(Proc));
		panel->procs[idx].pid = recv->pid;
		compute_num++;
	}
}

void reply_new_manage(Msg* recv, Msg* cnst){
	cnst->type = (long)recv->pid;
	cnst->pid = getpid();
	cnst->msg = -1;
}

#define constr_send(func)	func(&recv, &send); \
                                if(msgsnd(msgd, &send, sizeof(Msg), IPC_NOWAIT) == -1) \
        	                               perror("Msg send"); 
#define REPORT_ON_EXIT -50
void exit_hdl(int signum){
	if(shmid < 0 || msgd < 0)	return;
	if(signum == REPORT_ON_EXIT ){
		if(!fork()){	//Child
			execl("./report","report", NULL);
			exit(0);
		}
	}
	Msg buf;
	while(msgrcv(msgd, &buf, sizeof(Msg), (long)TERM_SIG, IPC_NOWAIT) != -1);	//Flush out all 
	int i = 0;
	for(compute_num = 0; i < PTABLE_SIZE; i++){
		pid_t proc;
		if( (proc = panel->procs[i].pid) > 0){
			if(! kill(proc, SIGINT))
				compute_num++;
		}
	}
	while(compute_num > 0){
		msgrcv(msgd, &buf,sizeof(Msg), (long)TERM_SIG, 0);
		compute_num--;
	}
	wait(&i);
	msgctl(msgd, IPC_RMID, NULL);
	shmdt(panel);
	shmctl(shmid, IPC_RMID, NULL);
	exit(signum);
}

int main(int argc, void** argv){
	// Check if another manage is already running.
	msgd = msgget(KEY, 0660 | IPC_CREAT | IPC_EXCL);
	Msg send, recv;
	if(msgd == -1){
		if(errno == EEXIST){	// The message queue is already there
			printf("Detected the existance of MsgQ.\n");
			// Send check message
			msgd = msgget(KEY, 0660);
			send.type = MNGR_INQ;
			send.pid = getpid();
			send.msg = (int)send.pid;
			if(msgsnd(msgd, &send, sizeof(Msg), IPC_NOWAIT) == -1){
				perror("SEND Message");
				exit(-1);
			}
			sleep(5);
			if(msgrcv(msgd, &recv, sizeof(Msg), (long)getpid(), IPC_NOWAIT) == -1 || recv.msg != -1){
				printf("Removing the abandoned queue...\n");
				msgctl(msgd, IPC_RMID, NULL);
				msgd = msgget(KEY, 0660 | IPC_CREAT);
				if(msgd == -1){
					perror("MessageQ Request");
					exit(-1);
				}
			}else{
				printf("Another manage is running! pid = %d\n", recv.pid);
				exit(-1);
			}
		}else{
			perror("MessageQ Request");
			exit(-1);
		}
	}
	
	//Creat and map share memory
	shmid = shmget(KEY, sizeof(Shmseg), 0660 | IPC_CREAT);
	if(shmid == -1){
		perror("Shared Mem Request");
		msgctl(msgd, IPC_RMID, NULL);
		exit(-1);
	}
	panel = (Shmseg*) shmat(shmid, NULL, 0);
	panel->manager = getpid();
	memset(panel->bitmap, 0, BITMAP_SIZE);

	//Set up signal mask & action -- POSIX API
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGQUIT);
	sigaddset(&sigmask, SIGHUP);

	memset(&sigact, 0, sizeof(struct sigaction) );
	sigact.sa_handler = &exit_hdl;
	sigact.sa_mask = sigmask;

	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
	
	while( msgrcv(msgd, &recv, sizeof(Msg), (long)-ARCH_NUM, 0) > -1 && next_perfect < PTABLE_SIZE){ //Keep recieving msg
		switch(recv.type){
			case COMP_REQ:			//New Compute born request
				constr_send(new_compute);
				break;

			case MNGR_INQ:			//New Manage born inquery
				printf("Recieve a born of new manage: %d\n", recv.pid);
				constr_send(reply_new_manage);
				break;

			case PEFT_REQ:			//New Perfect Number request
				panel->perfect[next_perfect++] = recv.msg;
				break;
			case TERM_SIG:			//Lost of a compute "signal"
				compute_num --;
				break;
			default:
				printf("what? Messgnum = %d\n", recv.type );
				break;
		}
	}
	if(errno != EINTR)		//The program left because a regular break out.
		exit_hdl( REPORT_ON_EXIT );
}
