/* THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING

A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yichu Lin

EXTRA CREDIT finished:
	1. delete
	2. print verbose
	3. all functions (q,x,d) with multiple argument
		=== But x didn't have the ability to extract all if no more args are provided.
*/

#include <ar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <errno.h>
#include <string.h>
#include <utime.h>
#include <time.h>
#include <dirent.h>

/* Possible Options for myar */
#define OPT_SIZE 7 
char *opt = "qxtvdAh";

typedef struct ar_hdr Header;

/* Get the option inputed by argument */
char getOpt(char* arg){

	//Special case for tv option.
	if(strlen(arg) == 2){
		if(!(strncmp("tv", arg, 2)))
			return 'v';
	}	
	int i;
	for(i = 0; i < OPT_SIZE; i++){
		if(arg[0] == opt[i])
			return arg[0];
	}

	//By default, return the help file if a incorrect input is received.
	printf("ERROR: Option not recognized.\n");
	return 'h';
}

/* Get the correct argument numbers for inputed option */
int argNum(char m){
	switch(m){
		case 'q': case 'x': case 'd':
			return 1;
		default:
			return 0;
	}
}

/* Help File Note Here */
char* hnote =   "\nOptions:\n"
		"\tq arch fileName [...]    quickly append named files to archive\n"
		"\tx arch fileName [...]    extract named files\n"
		"\tt arch                   print a concise table of contents of the archive\n"
		"\tv arch                   print a verbose table of contents of the archive\n"
		"\td arch fileName [...]    delete named files from archive\n"
		"\tA arch                   quickly append all ordinary files in the current directory\n"
		"\th                        print this message\n";

/*Define Global Variables */
struct stat arch;

#define BUFSIZE 128
char buf[BUFSIZE];



/*** Auxillary Functions ***/
/* Replace dest str n bytes with src string. copy n bytes, return the pointer to next byte.*/
char*  strnrep(char* dest, char* src, size_t n){
	size_t i;
	for(i = 0; i < n && src[i] != '\0'; i++){
		dest[i] = src[i];
	}
	return &dest[i];
}

/* Get mode of the file */
mode_t atomd(char* asc){
	int rtn = 0;
	for(; *asc >= '0' && *asc <= '7' ; asc++){
		rtn *= 8;
		rtn += (*asc-'0');
	}
	return (mode_t) rtn;
}

/* Open Archive for write */
int openWrt(char* arch, int exists){
	if(exists)
		return open(arch, O_WRONLY|O_APPEND, 0644);
	
	int fd = open(arch, O_CREAT|O_WRONLY|O_TRUNC, 0644);
	if(fd == -1){
		printf("ERROR in creat(\"%s\")",arch);
		perror(":");
		exit(0);
	}
	if( write(fd, ARMAG, SARMAG) == -1){
		printf("ERROR in write(\"%s\")",arch);
		perror(":");
		close(fd);
		exit(0);
	}
	printf("myar: create archive file \"%s\"\n",arch);
	return fd;
}

/* Open Archive for read */
int openRd(char* arch){	//Hopefully the existance of archive has already check previously.
	int fd;
	if( (fd = open(arch, O_RDONLY)) == -1){
		printf("ERROR in open archive \"%s\"", arch);
                perror(":");
                exit(0);		
	}
	char buff[SARMAG+1];
	if( read(fd, buff, SARMAG) != SARMAG){
		printf("ERROR: Archive file format unrecognized.\n");
		close(fd);
		exit(0);
	}
	if( strncmp(buff, ARMAG, SARMAG) ){
		printf("ERROR: Unrecognizable archive file, may be destroied.\n");
		close(fd);
		exit(0);
	}
	return fd;

}

/* Write one file into archive */
int apdFile(int archfd, char* aFile){
	struct stat fileStat;
	if( stat(aFile, &fileStat) == -1){
		printf("ERROR in reaching file \"%s\"", aFile);
		perror(":");
		return -1;
	}

	// Constructing Header	
	Header* hdr = (Header*) malloc(sizeof(Header));
	memset(hdr, ' ', sizeof(Header));
	*strnrep(hdr->ar_name, aFile, 15) = '/';

	sprintf(buf,"%d",fileStat.st_mtime);
	strnrep(hdr->ar_date, buf, 12);

	sprintf(buf,"%d",fileStat.st_uid);
	strnrep(hdr->ar_uid, buf, 6);
	
	sprintf(buf,"%d",fileStat.st_gid);
	strnrep(hdr->ar_gid, buf, 6);

	sprintf(buf,"%d",fileStat.st_mtime);
	strnrep(hdr->ar_date, buf, 12);
		
	sprintf(buf,"%o",fileStat.st_mode);
	strnrep(hdr->ar_mode, buf, 8);

	sprintf(buf,"%d",fileStat.st_size);
	strnrep(hdr->ar_size, buf, 10);
	
	strnrep(hdr->ar_fmag, ARFMAG, 2);

	// Write Header
	if(write(archfd,(char*)hdr,sizeof(Header)) == -1){
		perror("ERROR in writing header:");
		free(hdr);
		return -1;
	}

	int filefd;
	if( (filefd = open(aFile,O_RDONLY)) == -1){
		printf("ERROR in open(\"%s\")", aFile);
		perror(":");
		free(hdr);
		return -1;
	}
	
	// Write file content
	char* buff = (char*) malloc( fileStat.st_blksize );
	
	int readB, rtnFlag = 0;
	while( (readB = read(filefd, buff, (size_t)fileStat.st_size) )){
		if(readB == -1){	
			printf("ERROR in read(\"%s\")", aFile);
			perror(":");
			rtnFlag = -1;
			break;
		}
		if(readB & 1){	// Test if read in bytes are odd.  ASSUMPTION: all st_blksize are even
			buff[readB++] = '\n';
		}
		if( write(archfd,buff,readB) == -1){
			perror("ERROR in appending file:");
			rtnFlag = -1;
			break;
		}
	}
	free(buff);
	free(hdr);
	close(filefd);	
	if(!rtnFlag)
		printf("SUCCESS! %s appended\n", aFile);
	return rtnFlag;
}

/* TWO archive file dealer */
/* int DealerFunc(int arfd, int newarfd, Header* fileHdr) */
typedef int (*ArfDealer)(int,int,Header*);

/* Skip unused bytes - used in extrating etc */
int arfSkiper(int arfd, int not_been_used, Header* fHdr){
	long fileSize = atol(fHdr->ar_size);
	if( lseek(arfd, (off_t) (fileSize + (fileSize & 1)), SEEK_CUR) == -1){
		printf("ERROR in seeking next file");
		perror(":");
		return -1;
	}
	return 0;
}

/* Write the archive file into another archive - used in delete*/
int arfCopier(int arfd, int narfd, Header* fHdr){
	if( write(narfd, (char*) fHdr, sizeof(Header)) == -1){
		perror("Archive Header construction error");
		return -1;
        }

	size_t fileSize = (size_t)atol(fHdr->ar_size);
	fileSize = fileSize + (fileSize & 1);		// Read even number of bytes.
	size_t blkSize = (size_t)arch.st_blksize;
	char* buff = (char*) malloc(blkSize);
	ssize_t readB;
	size_t readRem = fileSize;

	int rtnflag = 0;
	
	while( readRem ){
		if( (readB = read(arfd,buff, (blkSize < readRem) ? blkSize : readRem) ) == -1){
			perror("Archive File read error");
			rtnflag = -1;
			break;
		}
		if( readB == 0){
			printf("ERROR: end of archive reached before reaching size of file.\n");
			break;
		}
		if( write(narfd, buff, readB) == -1){
			perror("Archive construction error");
			rtnflag = -1;
			break;
		}
		readRem = (readRem < (size_t) readB)? 0 : readRem - readB;	//Taking care of unsigned types
	}
	free(buff);
	return rtnflag;
}

/* Locate the next first file with name in archive*/
Header* arSeek(int arfd, char* name, ArfDealer dealerFunc, int isFind){
	//isFind = 0 if return next; isFind = fd if doing deleting.

	//ASSUMPTION: the file has already arrived the correct starting point to find file.

	int nameL = strlen(name);
	if(nameL > 15){
		printf("Exception: You said no filename is longer than 15 bytes though....\n");
		return NULL;
	}

	int readB;
	Header* fHdr = (Header*) malloc(sizeof(Header));
	while( (readB = read(arfd, fHdr, sizeof(Header))) ){
		if(readB == -1){
			perror("ERROR in reading archive:");
			free(fHdr);
			return NULL;
		}
		if(readB != sizeof(Header)){
			printf("ERROR: Unexpected form of header.\n");
			free(fHdr);
			return NULL;
		}
		if(!isFind || !(strncmp(fHdr->ar_name, name, nameL)) )	// return if finding matches or just getting next one.
			return fHdr;
		if(dealerFunc(arfd, isFind, fHdr)){
			free(fHdr);
			return NULL;
		}
	}
	if(strlen(name))
		printf("ERROR: No such file in archive.\n");
	free(fHdr);
	return NULL;
}

/* Two stat printer */
/* void StatPrinterFunc(Header* hdr) */
typedef void(*StatPrn)(Header*);

/* Just print file names */
void prnNameOnly(Header* hdr){
	char* p = (char*) hdr;
	for(; p < hdr->ar_date; p++){
		if(*p == '/' && *(p+1) == ' ')	break;
		printf("%c",*p);
	}
}

/* Verbose table of files*/
int modeTest[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
char* modeExp = "rwx";
typedef struct _ctimeReturnStringDecomposer {
				/* Examples */
	char dayOfWeek[4];	/* Fri(space) */
	char month[4];		/* Nov(space) */
	char day[3];		/* (space)2(space) */
	char hh_mm_ss[9];	/* 11:11:11(space) */
	char year[4];		/* 2015 */
} tStrDecmp;						//Decomposer for string returned from ctime()

void prnVerbose(Header* hdr){
	mode_t md = atomd(hdr->ar_mode);
	int i = 0;
	for(; i < 9; i++)
		printf("%c", (md & modeTest[i]) ? modeExp[i%3] : '-');
	printf(" %d/%d %10u  ", atoi(hdr->ar_uid), atoi(hdr->ar_gid), atol(hdr->ar_size));
	
	time_t fTimeT = (time_t)atol(hdr->ar_date);
	tStrDecmp* fTime = (tStrDecmp*) ctime(&fTimeT);
	printf("%.12s %.4s    ", fTime->month, fTime->year);

	prnNameOnly(hdr);
}




/*** Function for Options ***/
/* Quickly Append */
void quickApd(char* archName, int exists, char* files[]){
	int archfd;
	if( (archfd = openWrt(archName, exists)) == -1){
		printf("ERROR in open archive \"%s\"", archName);
		perror(":");
		exit(0);
	}
	int i = 0;
	while(files[i]){
		apdFile(archfd, files[i++]);
	}
	close(archfd);
}

/* Extract File from archive */
void extrFile(char* arcName, char* files[]){
	int arfd = openRd(arcName);
	char** p = files;
	for(;*p;p++){
		if( lseek(arfd, SARMAG , SEEK_SET) == -1){
			perror("ERROR in seeking header: ");
			exit(0);
		}
		Header* fileHdr;
		if(!(fileHdr = arSeek(arfd, *p, &arfSkiper, 1))){		//Continue loop (skip this file name argument) if no match file found.
			continue;
		}
		//Now seeker should return the pointer to header and set file position to the first byte of the file.
		int ffd;
		mode_t md;
		if( (ffd = open(*p, O_CREAT|O_WRONLY|O_TRUNC, ( ( (md = atomd(fileHdr->ar_mode)) == 0) ? 0644 : md )) ) == -1){
			printf("ERROR in creat(\"%s\")", *p);
			perror(":");
			continue;
		}
		
		//Extracting File
		size_t buffSize = (size_t)arch.st_blksize;
		char* buff = (char*) malloc( buffSize );
		size_t readRem = (size_t)atol(fileHdr->ar_size);
		ssize_t readB;
		while(readRem > 0U){
			if( (readB = read(arfd, buff, (buffSize < readRem)? buffSize : readRem)) == -1){
				perror("ERROR in reading archive:");
				break;
			}
			if(readB == 0){
				//Archive is running out.
				char* nullB = (char*) malloc(readRem);
				memset(nullB, 0, readRem);
				if(write(ffd, nullB, readRem) == -1){
					printf("ERROR in write(\"%s\")", *p);
					perror(":");
				}else{
					printf("WARNING: marked size of \"%s\" is larger than contained in archive.\n\tThus padding with %d null bytes.\n",p,readRem);
				}
				free(nullB);
				readRem = 0;
			}else{
				if(write(ffd, buff, (readRem > buffSize)? buffSize: readRem) == -1){
					printf("ERROR in write(\"%s\")", *p);
                                        perror(":");
					break;
				}
				readRem = ( (ssize_t)readRem - readB < 0)? 0 : readRem - (size_t)readB;
			}
		}
		if(!readRem){	//if not jumped out because of error
			//Change aTime and mTime here
			time_t atime = (time_t) atol(fileHdr->ar_date);
			struct utimbuf* timebuf = (struct utimbuf*)malloc(sizeof(struct utimbuf));
			timebuf->actime = atime;
			timebuf->modtime = atime;
			if(utime(*p, timebuf) == -1){
				perror("ERROR in changing timestamps");
			}
			free(timebuf);
			printf("SUCCESS! %s retrieved\n",  *p);
		}
		close(ffd);
		free(buff);
		free(fileHdr);
	}
	close(arfd);
}

/* Delete Files */
void doFlush(int,int);
void delFile(char* arcName, char* file){
        //ASSUMPTION: file != NULL
        int fdr = openRd(arcName);
	if( lseek(fdr, SARMAG , SEEK_SET) == -1){
		perror("ERROR in seeking header: ");
		exit(0);
	}

	unlink(arcName);
	int fdw = openWrt(arcName,0);
	Header* fileHdr;
	if(!(fileHdr = arSeek(fdr, file, &arfCopier, fdw))){	//No matching was found...
		printf("\t===Thus fail to delete \"%s\"\n", file);
	}else{
                //Now seeker should return the pointer to header of the file that we wanna skip.
		if(arfSkiper(fdr, -1, fileHdr) == -1)
			printf("ERROR: fail to skip the file.\n");
		else
			printf("\"%s\" is removed from archive\n", file);
	}	

	doFlush(fdr, fdw);
}

/* Flush the tail of the archive*/
void doFlush(int fdr, int fdw){
		size_t buffSize = (size_t) arch.st_blksize;
		char* buff = (char*)malloc(buffSize);
		ssize_t readB;
		while( (readB = read(fdr, buff, buffSize)) > 0){
			if( write(fdw,buff,readB) == -1)
				break;
		}	
		free(buff);
		close(fdr);
		close(fdw);
}

/* Deleting multiple files. */
/* Yeah my approach is really costing. */
void delFiles(char* archFile, char** files){
	for(;*files;files++)
		delFile(archFile,*files);
}

/* Append all ordinary files */
void apdAll(char* archName, int exists){
	int archfd;
        if( (archfd = openWrt(archName, exists)) == -1){
                printf("ERROR in open archive \"%s\"", archName);
                perror(":");
                exit(0);
        }

	DIR* dirp;
	struct dirent* entryp;
	struct stat fileS;

	if( !( dirp = opendir(".") )){
		perror("ERROR in open current directory");
		close(archfd);
		exit(0);
	}

	while( (entryp = readdir(dirp)) ){
		if(strlen(entryp->d_name) > 15){
			printf("Skipping \"%s\": A long name\n",entryp->d_name);
			continue;
		}
		if(stat(entryp->d_name,&fileS) == -1){
			printf("ERROR in reaching \"%s\"\n", entryp->d_name);
			perror("stat");
			continue;
		}
		if(!S_ISREG(fileS.st_mode)){
			printf("Skipping \"%s\": Not a Regular / Ordinary file\n",entryp->d_name);
			continue;
		}
		if(fileS.st_mode & (0111)){	//Any Executable entry was set
			printf("Warning: Executable \"%s\" appended.\n",entryp->d_name);
		}
		if(!strcmp(entryp->d_name, archName)){
			printf("Skipping \"%s\": The archive file itself\n",entryp->d_name);
                        continue;
		}
		if(apdFile(archfd, entryp->d_name) == -1)
			printf("\t\t===>FAILED\n");
			
	}
	closedir(dirp);
	close(archfd);
}

/* print a list */
void printAll(char* arcName, StatPrn printFunc, int elemPerRow){
        int arfd = openRd(arcName);

	Header* hdr;
	int cnt = 0;
	while( hdr = arSeek(arfd, "", &arfSkiper, 0) ){
		printFunc(hdr);
		cnt++;
		if(!(cnt % elemPerRow))
			printf("\n");
		else
			printf("    ");
		arfSkiper(arfd, -1, hdr);
	}
	if(cnt % elemPerRow)
		printf("\n");
}



/*** MAIN ***/
#define ELEM_PER_ROW 5
void main(int argc, char** argv){

	//Check options
	char m = 'h';
	if(argc == 1){
		printf("USAGE: %s [%s] archive_name [file_name]\n", argv[0],opt);
	}else{
		m = getOpt(argv[1]);
	}

	//Deal with help function
	if(m == 'h'){
		printf(hnote);
		exit(0);
	}	

	//Check arguments
	if(argc < argNum(m) + 3){
		printf("ERROR: Not Enough Arguments for option %c - %d needed, %d received.\n", m, (3+argNum(m)), argc);
		exit(-1);
	}

	//Check Archive File
	int ar_exist = stat(argv[2], &arch);
	if(ar_exist == -1 && m != 'q' &&  m != 'A'){
		printf("ERROR: The Archive File \"%s\" does not exist.\n", argv[2]);
		exit(-1);
	}
	
	switch(m){
		case 'q':
			quickApd(argv[2], ar_exist+1, &argv[3]);
			break;
		case 'A':
			apdAll(argv[2], ar_exist+1);
			break;
		case 'x':
			extrFile(argv[2], &argv[3]);
			break;
		case 't':
			printAll(argv[2], &prnNameOnly, ELEM_PER_ROW);
			break;
		case 'v':
			printAll(argv[2], &prnVerbose, 1);
			break;
		case 'd':
			delFiles(argv[2],&argv[3]);
			break;
				
	}
	exit(0);
}
