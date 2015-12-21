/* Homework for CS450
 * Fall 2015 - Yichu Lin
 */
#include "perfect.h"

#define LAZY_CACHE_SIZE 256
int cntbuf[LAZY_CACHE_SIZE];
int get_bits(unsigned int abyte){
	if(cntbuf[abyte] >= 0) return cntbuf[abyte];
	int sum = 0;
	unsigned int idx = abyte;
	for(; abyte != 0; abyte >>= 1)
		sum += (abyte & 1);
	cntbuf[idx] = sum;
	return sum;
}


int main(int argc, char** argv){
	int am_killer = 0;
	memset(&cntbuf, -1, LAZY_CACHE_SIZE *sizeof(int));
	shmid = shmget(KEY, sizeof(Shmseg), 0660);
	if(shmid == -1){
		perror("Shared Mem get");
		exit(-1);
	}
	if(!(panel = (Shmseg*) shmat(shmid, NULL, 0))){
		perror("Shared Mem attach");
		exit(-1);
	}
	if(argc > 1){
		if( !strcasecmp("-k", argv[1]))
			am_killer = 1;
	}

	Proc* ptable = panel->procs;

	printf("REPORT OF PERFECT NUMBERS\n");
	//Print Perfect
	printf("Found:\n\t");
	int i;
	for(i = 0; i < PERF_ARR_SIZE; i++){
		if(!panel->perfect[i]) 	break;
		printf("%d ", panel->perfect[i]);
	}
	int count = 0;
	printf("\nCompute process table:\npid\ttested\tskipped\tfound\n");
	for(i = 0; i < PTABLE_SIZE; i++){
		if(ptable[i].pid <= 0) continue;
		printf("%d\t%d\t%d\t%d\n", ptable[i].pid, ptable[i].test, ptable[i].skip, ptable[i].found);
		count++;
	}
	int num_tested = 0;
	for(i = 0; i < BITMAP_SIZE; i++)
		num_tested += get_bits((unsigned char)panel->bitmap[i]);
	printf("===============\nCount:%2d\tTotal Num Tested: %d\n", count, num_tested);

	if(am_killer){
		printf("%d\n", panel->manager);
		if(kill(panel->manager, SIGINT) == -1){	// Manager is not there?
			shmctl(shmid, IPC_RMID, NULL);
			if((msgd = msgget(KEY, 0660) ) != -1)
				msgctl(msgd, IPC_RMID, NULL);
		}
		
	}
	shmdt(panel);
}
