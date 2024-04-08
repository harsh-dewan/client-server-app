#include <netinet/in.h> //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //for socket APIs
#include <sys/types.h>
#include <string.h> 
#include <ctype.h>

// Function to trim leading and trailing whitespace characters
char *trim(char *str) {
	// Trim leading spaces
	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0)  // All spaces?
		return str;

	// Trim trailing spaces
	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	// Null-terminate the trimmed string
	*(end + 1) = '\0';

	return str;
}


int main(int argc, char *argv[]){

	//E.g., 1, client

	char message[100];
	int server, portNumber;
	socklen_t len;
	struct sockaddr_in servAdd;

	if(argc != 3){
		printf("Call model:%s <IP> <Port#>\n",argv[0]);
		exit(0);
	}

	if ((server=socket(AF_INET,SOCK_STREAM,0))<0){ //socket()
		fprintf(stderr, "Cannot create socket\n");
		exit(1);
	}

	servAdd.sin_family = AF_INET; //Internet 
	sscanf(argv[2], "%d", &portNumber);
	servAdd.sin_port = htons((uint16_t)portNumber);//Port number

	if(inet_pton(AF_INET, argv[1],&servAdd.sin_addr) < 0){
		fprintf(stderr, " inet_pton() has failed\n");
		exit(2);
	}

	if(connect(server, (struct sockaddr *) &servAdd,sizeof(servAdd))<0){//Connect()
		fprintf(stderr, "connect() failed, exiting\n");
		exit(3);
	}


	if ( read(server, message, 100) < 0 ) {//read()
		fprintf(stderr, "read() error\n");
		exit(3);
	}
	fprintf(stdout, "%s\n", message);

	char user_input[1024] = {'\0'};
	char command[100]={'\0'};
	while(1) {
		memset(command,'\0',100);
		memset(user_input,'\0',1024);
		printf("client24$ ");
		fgets(user_input,1024,stdin);
		char *trim_user_input = trim(user_input);
		//memset(user_input,'\0',strlen(user_input));
		//memcpy(user_input,trim_user_input,strlen(trim_user_input));
		//printf("user_input <%s>\n",user_input);
		//printf("trim_user_input <%s>\n",trim_user_input);
		if( strlen(trim_user_input) == 0 ) continue;
		else if( memcmp(trim_user_input,"quitc",strlen("quitc")) == 0 ) {
			printf("Terminating this session, bye!\n");
			exit(0);
		}
		else {
			printf("user_input <%s>\n",trim_user_input);
			if( memcmp(trim_user_input,"dirlist",strlen("dirlist")) == 0 ) 
			{
				int count = 0, total_spaces=0;
				while(count < strlen(trim_user_input) ) {
					if( memcmp(trim_user_input+count," ",1) == 0 ) total_spaces+=1;
					count++;
				}
				if( total_spaces > 1 || total_spaces < 1 ) {
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [dirlist -a] or [dirlist -t]\n");
					continue;
				}
				if( strlen(trim_user_input) == strlen("dirlist -a") && 
				    memcmp(trim_user_input,"dirlist -a",strlen("dirlist -a")) == 0 ) {
					write(server, "adirlist", strlen("adirlist"));
				}
				else if( strlen(trim_user_input) == strlen("dirlist -t") &&
					 memcmp(trim_user_input,"dirlist -t",strlen("dirlist -t")) == 0 ) {
					write(server, "tdirlist", strlen("tdirlist"));
				}
				else {
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [dirlist -a] or [dirlist -t]\n");
					continue;	
				}
				if ( read(server, message, 100) < 0 ) {
					//read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);
			}
			else if( memcmp(trim_user_input,"w24fn",strlen("w24fn")) == 0 ) {

				int count = 0, total_spaces=0;
				while(count < strlen(trim_user_input) ) {
					if( memcmp(trim_user_input+count," ",1) == 0 ) total_spaces+=1;
					count++;
				}
				if( total_spaces > 1 || total_spaces < 1 ) {
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24fn filename]\n");
					continue;
				}
				memset(command,'\0',strlen(command));
				memcpy(command,trim_user_input,5);
				memcpy(command+5,"_",1);
				memcpy(command+6,trim_user_input+6,strlen(trim_user_input+5));
				write(server, command, strlen(command));
				if ( read(server, message, 100) < 0 ) {//read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);

			}
			else if( memcmp(trim_user_input,"w24fz",strlen("w24fz")) == 0 ) {
				char size1[10]={'\0'};
				char size2[10]={'\0'};
				int space_index = 6;
				int count = 0, total_spaces=0;
				while(count < strlen(trim_user_input) ) {
					if( memcmp(trim_user_input+count," ",1) == 0 ) total_spaces+=1;
					count++;
				}
				if( total_spaces > 2 || total_spaces < 2 ) {
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24fz size1 size2]\n");
					continue;
				}
				while(space_index-1 >= 0 && space_index+1 < strlen(trim_user_input) ) {
					if( memcmp(trim_user_input+space_index-1," ",1) != 0 && 
							memcmp(trim_user_input+space_index," ",1) == 0 &&
							memcmp(trim_user_input+space_index+1," ",1) != 0 ) {
						break;
					}
					space_index++;
				}
				//printf("space_index <%d>\n",space_index);
				memcpy(size1,trim_user_input+6,space_index-6);
				//printf("size1 <%s>\n",size1);
				memcpy(size2,trim_user_input+space_index+1,strlen(trim_user_input+space_index+1));
				//printf("size1 <%s> and size2 <%s>\n",size1, size2);
				int index = 0, flag = 0;
				while(index < strlen(size1)) {
					char ch = size1[index];
					if( ch < '0' || ch > '9' ) {
						printf("incorrect size1, please try again\n");
						flag = 1;
						break;
					}	
					index++;
				}
				if( flag == 1) continue;
				index = 0;
				while(index < strlen(size2)) {
					char ch = size2[index];
					if( ch < '0' || ch > '9' ) {
						printf("incorrect size2, please try again\n");
						flag = 1;
						break;
					}	
					index++;
				}
				if( flag == 1 ) continue;
				memset(command,'\0',100);
				memcpy(command,"w24fz_",6);
				memcpy(command+6,size1,strlen(size1));
				memcpy(command+strlen(command),"_",1);
				memcpy(command+strlen(command),size2,strlen(size2));
				//int s1 = atoi(size1);
				//int s2 = atoi(size2);
				//printf("s1 <%d> and s2 <%d>\n",s1,s2);
				write(server, command, strlen(command));
				if ( read(server, message, 100) < 0 ) {//read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);

			}
			else if( memcmp(trim_user_input,"w24ft",strlen("w24ft")) == 0 ) {

			}
			else if( memcmp(trim_user_input,"w24fdb",strlen("w24fdb")) == 0 ) {

			}
			else if( memcmp(trim_user_input,"w24fda",strlen("w24fda")) == 0 ) {

			}
			else {
				printf("Incorrect input, please enter correct command\n");
				continue;
			}
			//write(server,trim_user_input,strlen(trim_user_input));
		}
	}
	exit(0);
}
