/*
 * 1. test all cases of user input
 * 2. adding comments
 * done  3. section 3, alternate server and mirror servers
 * 4. complete run
 * done 5. check date input validation and other user input validation
 * 6. update the code for socket part
 * 7. all files returned from server must be saved in a folder named w24project in home directory
 * 8. try with mulitple clients
 * done 9.  print all commands in case of wrong input
 * 10. check all the error handling part, if correct message is displayed
 * done 11. validate the size in the input w24fz
 * done 12. filename illegal characters in filename w24fz
 * 13. check size2 >= size1 in w24fz
 */

/*including all required header files
*/
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

char message[100];
char command[100];
socklen_t len;
//creating file descriptor for server, mirror1 and mirror2 
int server, mirror1, mirror2, server_portNumber, portNumber;
//variables for port numbers
int mirror1_portNumber = 8090;
int mirror2_portNumber = 8091;
struct sockaddr_in servAdd;
//creating strings for user_input, date from user, size and filename for various commands
char user_input[1024] = {'\0'};
char date[15] = {'\0'};
char size[15] = {'\0'};
char filename[100] = {'\0'};


/*
 * function to receive tar file from the server
 * this functionis using read system call for reading from sockets
 * this creates a new temp.tar.gz file in client side and write content into it from socket
 */
void receive_file(int socket_fd, char *output_filename)
{
	int file_fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (file_fd < 0)
	{
		perror("Failed to open file for writing");
		return;
	}
	// Read the size of the file
	off_t file_size = 0;
	int socketread = read(socket_fd, &file_size, sizeof(file_size));
	// Receive the file data
	char buffer[4096] = {'\0'};
	ssize_t bytes_read;
	off_t total_bytes_read = 0;
	while (total_bytes_read < file_size && (bytes_read = read(socket_fd, buffer, sizeof(buffer))) > 0)
	{
		if (bytes_read == 0)
			break;
		write(file_fd, buffer, bytes_read);
		total_bytes_read += bytes_read;
	}
	//printf("Received %ld of %ld bytes (%.2f%%)\n", total_bytes_read, file_size, (total_bytes_read * 100.0 / file_size));
	//printf("received file okay\n");
	close(file_fd);
}

/*
 * to check if the year is leap year or not
 * used in validating date
 */
bool isLeapYear(int user_year)
{
	return (user_year % 4 == 0 && user_year % 100 != 0) || (user_year % 400 == 0);
}

/*
 * validathe the year, month and day in the date
 */
bool checkYearMonthDay(int year, int month, int day)
{
	if (year < 0 || month < 1 || month > 12 || day < 1)
		return false;
	int daysInMonth[] = {31, 28 + isLeapYear(year), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	return day <= daysInMonth[month - 1];
}

/*
 * used to check if the date is correct or not
 */
int isDateValid(char *inputDate)
{
	if (strlen(inputDate) != 10)
	{
		printf("Incorrect date length, returning 1 from isDateValid func\n");
		return 1;
	}
	if (memcmp(inputDate + 4, "-", 1) != 0 || memcmp(inputDate + 7, "-", 1) != 0)
	{
		printf("Incorrect - placed in the date, returning 1 from isDateValid func\n");
		return 1;
	}
	char arr[11] = {'\0'};
	memcpy(arr, inputDate, strlen(inputDate));
	for (int i = 0; i < strlen(inputDate); i++)
	{
		char ch = arr[i];
		if (ch == '-')
			continue;
		if (ch >= '0' && ch <= '9')
			continue;
		else
		{
			printf("Incorrect date, found non-digit character other than -\n");
			return 1;
		}
	}
	char year[5] = {'\0'};
	char month[3] = {'\0'};
	char day[3] = {'\0'};
	memcpy(year, inputDate, 4);
	memcpy(month, inputDate + 5, 2);
	memcpy(day, inputDate + 8, 2);
	int dyear = atoi(year);
	int dmonth = atoi(month);
	int dday = atoi(day);
	if (checkYearMonthDay(dyear, dmonth, dday)) return 0;
	return 1;
}

/*
 * validating the size entered by user is correct or not
 * it should contain digits only and no other characters;
 */
int isSizeValid(char *inputsize)
{

	int index = 0, strlength = strlen(inputsize);
	if (strlength == 0)
		return 1;
	while (index < strlength)
	{
		char ch = inputsize[index];
		if (ch < '0' || ch > '9')
			return 1;
		index++;
	}
	return 0;
}

/*
 * To validate the filename entered by user is correct or not
 * it should not contains '/'(forward slash) in the file name
 */
int isFilenameValid(char *filename)
{
	int index = 0, filenamelength = strlen(filename);
	if (filenamelength == 0)
		return 1;
	while (index < filenamelength)
	{
		char ch = filename[index];
		if (ch == '/')
			return 1;
		index++;
	}
	return 0;
}

/*
 * Function to trim the leading and trailing white spaces in the  user input
 */
char *trim(char *str)
{
	// to trim leading spaces
	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0) // All spaces?
		return str;

	// to trim trailing spaces
	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	// Null-terminate the trimmed string
	*(end + 1) = '\0';

	return str;
}

/*
 * function to connect to server
 * this will be used to connect to server to ip and port number 
 */
int connect_to_server(int port, char *ip_address)
{

	// creating the socket, if not possible printing the error message and returning
	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Unable to create socket, please try again!\n");
		exit(1);
	}
	// Internet
	servAdd.sin_family = AF_INET;
	// port number from user
	// sscanf(argv[2], "%d", &server_portNumber);
	servAdd.sin_port = htons((uint16_t)port); // updating port number in the struct

	// inet_pton(), if fails returning from the  code
	if (inet_pton(AF_INET, ip_address, &servAdd.sin_addr) < 0)
	{
		printf("inet_pton() has failed\n");
		exit(2);
	}

	/*
	 * trying to connect to server, if fails, exiting from code with message
	 */
	if (connect(server, (struct sockaddr *)&servAdd, sizeof(servAdd)) < 0)
	{ // Connect()
		printf("connect() failed when connecting to server from client, exiting\n");
		exit(3);
	}

	/*
	 * read() for reading from server, the first message upon connection
	 */
	if (read(server, message, 100) < 0)
	{
		printf("unable to read from server, read() error\n");
		exit(3);
	}
	return 0;
}



/*
   handling for main function*
   */
int main(int argc, char *argv[])
{

	if (argc != 3)
	{
		printf("	Incorrect usgae, please specify the IP and Port number\n");
		printf("	Usage [%s <IP> <Port#>]\n", argv[0]);
		exit(0);
	}
	sscanf(argv[2], "%d", &server_portNumber);
	//after receiving ip and port number, connecting with client
	connect_to_server(server_portNumber, argv[1]);
	if (memcmp(message, "SERVER", strlen("SERVER")) == 0)
	{
		//when connected to server only, printing below message
		printf("	Connected to Server\n");
	}
	else
	{
		/*
		 * request is to be served by mirror1 or mirror2, depending upn the message from  server
		 * if server sends "MIRROR1" message, client will connect to mirror1
		 * if server sends "MIRROR2" message, client will connect to mirror2
		 */
		if (memcmp(message, "MIRROR1", strlen("MIRROR1")) == 0)
		{
			// updating port number to 8090 for connecting to mirror1
			portNumber = mirror1_portNumber;
		}
		else
		{
			//updating port number to 8091 for connecting to mirror2
			portNumber = mirror2_portNumber;
		}
		close(server);
		connect_to_server(portNumber, argv[1]); // connecting to mirror1 or mirror based on message form server
		fprintf(stdout, "	%s\n", message);
	}
	while (1)
	{
		//taking the user input until quitc is received
		memset(user_input, '\0', 1024);
		printf("client24$ ");
		fgets(user_input, 1024, stdin); //user input
		char *trim_user_input = trim(user_input) ; //triming the white spaces both leading and trailing
		if (strlen(trim_user_input) == 0)
			continue;
		else if (memcmp(trim_user_input, "quitc", strlen("quitc")) == 0)
		{
			//terminating session based on "quitc" message
			printf("Terminating this session, bye!\n");
			exit(0);
		}
		else
		{
			/* handling for first command [dirlist -a , dirlist -t]
			 * if user input is dirlist -a  or dirlist -t
			 */
			if (memcmp(trim_user_input, "dirlist", strlen("dirlist")) == 0)
			{
				//validarting the user input, if more command is passed then required, printing below message
				int count = 0, total_spaces = 0;
				while (count < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + count, " ", 1) == 0)
						total_spaces += 1;
					count++;
				}
				//if not correct input
				if (total_spaces > 1 || total_spaces < 1)
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [dirlist -a] or [dirlist -t]\n");
					continue;
				}
				//based on dirlist -a or dirlist -t, writing the user input to server
				if (strlen(trim_user_input) == strlen("dirlist -a") &&
						memcmp(trim_user_input, "dirlist -a", strlen("dirlist -a")) == 0)
				{
					write(server, "dirlist -a", strlen("dirlist -a"));
				}
				else if (strlen(trim_user_input) == strlen("dirlist -t") &&
						memcmp(trim_user_input, "dirlist -t", strlen("dirlist -t")) == 0)
				{
					write(server, "dirlist -t", strlen("dirlist -t"));
				}
				else
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [dirlist -a] or [dirlist -t]\n");
					continue;
				}
				//reading the output from the server and then tokenizing based on '\n'
				char buffer[1025] = {'\0'};
				if (read(server, buffer, 1024) < 0)
				{ // read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}
				//tokening the server output and printing
				char *token = strtok(buffer, "\n");
				while (token != NULL)
				{
					printf("%s\n", token);
					token = strtok(NULL, "\n");
				}
			}
			/*
			 * handling for w24fn user input
			 * first validating the user input if it is according to the [w24fn filename]
			 * if not correct input supplied,then asking user to input again
			 * second, writing the user input to server and reading server ouput and printing on screen
			 */
			else if (memcmp(trim_user_input, "w24fn", strlen("w24fn")) == 0)
			{

				//validarting the user input
				int count = 0, total_spaces = 0;
				while (count < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + count, " ", 1) == 0)
						total_spaces += 1;
					count++;
				}
				if (total_spaces > 1 || total_spaces < 1)
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24fn filename]\n");
					continue;
				}
				memset(filename, '\0', 100);
				memcpy(filename, trim_user_input + 6, strlen(trim_user_input + 6));
				if (isFilenameValid(filename) != 0)
				{
					printf("	Incorrect filename, please try again!\n");
					printf("	Correct Usage [w24fn filename]\n");
					continue;
				}
				//after validating writing it to server
				write(server, trim_user_input, strlen(trim_user_input));
				memset(message, '\0', 100);

				//reading output from server for w24fn command
				if (read(server, message, 100) < 0)
				{ // read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);
			}
			/*
			 * handling user input for w24fz command
			 * first it validates the user input, then writes command to server and  reads a tar file from user
			 * moving the tar file to w24project/ folder in the home directory
			 */
			else if (memcmp(trim_user_input, "w24fz", strlen("w24fz")) == 0)
			{

				//validating the user input for w24fz, size1 and size2 
				char size1[10] = {'\0'};
				char size2[10] = {'\0'};
				int space_index = 6;
				int count = 0, total_spaces = 0;
				while (count < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + count, " ", 1) == 0)
						total_spaces += 1;
					count++;
				}
				if (total_spaces > 2 || total_spaces < 2)
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24fz size1 size2 (size1 <= size2) ]\n");
					continue;
				}
				while (space_index - 1 >= 0 && space_index + 1 < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + space_index - 1, " ", 1) != 0 &&
							memcmp(trim_user_input + space_index, " ", 1) == 0 &&
							memcmp(trim_user_input + space_index + 1, " ", 1) != 0)
					{
						break;
					}
					space_index++;
				}
				memcpy(size1, trim_user_input + 6, space_index - 6);
				memcpy(size2, trim_user_input + space_index + 1, strlen(trim_user_input + space_index + 1));
				// printf("size1 <%s> and size2 <%s>\n",size1, size2);
				if (isSizeValid(size1) != 0 || isSizeValid(size2) != 0 || atoi(size2) < atoi(size1))
				{
					printf("	Incorrect size values, please try again!\n");
					printf("	Correct Usage [w24fz size1 size2 (size1 <= size2)]\n");
					continue;
				}
				//once the user input is validated, writing to server
				write(server, trim_user_input, strlen(trim_user_input));
				//printf("AFTER WRITE\n");
				char tarname[20] = {'\0'};
				memcpy(tarname, "temp.tar.gz", strlen("temp.tar.gz"));
				//receiveing the tar file
				receive_file(server, tarname);
				system("tar -xvzf temp.tar.gz 2>/dev/null 1>/dev/null");
				FILE *fp = fopen("empty.txt", "r");
				if (fp == NULL) {
					//moving the tar file to w24project folder
					memset(command,'\0',strlen(command));
					memcpy(command,"mv temp.tar.gz w24project/",strlen("mv temp.tar.gz w24project/"));
					if(system(command) != 0 ) printf("unable to copy file to w24project\n");
				}
				else
				{
					//if the no files are present as expected from the command, we are printing message to user
					char c = fgetc(fp);
					if (c == 'x')
					{
						printf("File not found.\n");
						system("rm empty.txt");
						system("rm temp.tar.gz");
					}
				}
			}

			/*
			 * handling user input for w24ft command
			 * first it validates the user input, then writes command to server and  reads a tar file from user
			 * moving the tar file to w24project/ folder in the home directory
			 */
			else if (memcmp(trim_user_input, "w24ft", strlen("w24ft")) == 0)
			{

				// validating the user input
				int count = 0, total_spaces = 0;
				while (count < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + count, " ", 1) == 0)
						total_spaces += 1;
					count++;
				}
				if (total_spaces == 0 || total_spaces > 3)
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24ft ext1 ext2 ext3 - upto 3 extensions]\n");
					continue;
				}
				write(server, trim_user_input, strlen(trim_user_input));
				char tarname[20] = {'\0'};
				memcpy(tarname, "temp.tar.gz", strlen("temp.tar.gz"));
				//receiving tar file from server
				receive_file(server, tarname);
				system("tar -xvzf temp.tar.gz 2>/dev/null 1>/dev/null");
				FILE *fp = fopen("empty.txt", "r");
				if (fp == NULL) {

					//moving the file to w24project/ folder
					memset(command,'\0',strlen(command));
					memcpy(command,"mv temp.tar.gz w24project/",strlen("mv temp.tar.gz w24project/"));
					if(system(command) != 0 ) printf("unable to copy file to w24project\n");
				}
				else
				{
					//if no files are found as expected from the command, printing the user below message
					char c = fgetc(fp);
					if (c == 'x')
					{
						printf("File not found.\n");
						system("rm empty.txt");
						system("rm temp.tar.gz");
					}
				}
			}

			/*
			 * handling user input for w24fdb command
			 * first it validates the user input, then writes command to server and  reads a tar file from user
			 * moving the tar file to w24project/ folder in the home directory
			 */
			else if (memcmp(trim_user_input, "w24fdb", strlen("w24fdb")) == 0)
			{

				//validating the user input
				int count = 0, total_spaces = 0;
				while (count < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + count, " ", 1) == 0)
						total_spaces += 1;
					count++;
				}
				if (total_spaces < 1 || total_spaces > 1)
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24fdb date, date format YYYY-MM-DD]\n");
					continue;
				}
				memset(date, '\0', 15);
				memcpy(date, trim_user_input + 7, strlen(trim_user_input));
				// date validation
				if (isDateValid(date) != 0)
				{
					printf("	Invalid date, please try again!\n");
					printf("	Correct Usage [w24fdb date, date format YYYY-MM-DD]\n");
					continue;
				}
				write(server, trim_user_input, strlen(trim_user_input));
				char tarname[20] = {'\0'};
				memcpy(tarname, "temp.tar.gz", strlen("temp.tar.gz"));
				//receiving the file from server	
				receive_file(server, tarname);
				memset(command,'\0',strlen(command));
				memcpy(command,"mv temp.tar.gz w24project/",strlen("mv temp.tar.gz w24project/"));
				if(system(command) != 0 ) printf("unable to copy file to w24project\n");
				/*system("tar -xvf temp.tar");
				  sleep(60);
				  FILE *fp = fopen("empty.txt", "r");
				  if (fp == NULL) {
				//moving the file to w24project
				memset(command,'\0',strlen(command));
				memcpy(command,"mv temp.tar w24project/",strlen("mv temp.tar w24project/"));
				if(system(command) != 0 ) printf("unable to copy file to w24project\n");
				}
				else
				{
				// if no files are returned by the server, printing the user below message
				char c = fgetc(fp);
				if (c == 'x')
				{
				printf("File not found.\n");
				system("rm empty.txt");
				system("rm temp.tar.gz");
				}
				}*/
			}

			/*
			 * handling user input for w24fda command
			 * first it validates the user input, then writes command to server and  reads a tar file from user
			 * moving the tar file to w24project/ folder in the home directory
			 */
			else if (memcmp(trim_user_input, "w24fda", strlen("w24fda")) == 0)
			{
				// validating the user input
				int count = 0, total_spaces = 0;
				while (count < strlen(trim_user_input))
				{
					if (memcmp(trim_user_input + count, " ", 1) == 0)
						total_spaces += 1;
					count++;
				}
				if (total_spaces > 1 || total_spaces < 1)
				{
					printf("	Wrong Usage, please try again!\n");
					printf("	Correct Usage [w24fda date, date format YYYY-MM-DD]\n");
					continue;
				}
				// date validation
				memset(date, '\0', 15);
				memcpy(date, trim_user_input + 7, strlen(trim_user_input));
				if (isDateValid(date) != 0)
				{
					printf("	Invalid date, please try again!\n");
					printf("	Correct Usage [w24fda date, date format YYYY-MM-DD]\n");
					continue;
				}
				write(server, trim_user_input, strlen(trim_user_input));
				char tarname[20] = {'\0'};
				memcpy(tarname, "temp.tar.gz", strlen("temp.tar.gz"));
				//receiving the file from server
				receive_file(server, tarname);
				memset(command,'\0',strlen(command));
				memcpy(command,"mv temp.tar.gz w24project/",strlen("mv temp.tar.gz w24project/"));
				if(system(command) != 0 ) printf("unable to copy file to w24project\n");
				/*system("tar -xvzf temp.tar.gz");
				FILE *fp = fopen("empty.txt", "r");
				if (fp == NULL) {
					//moving the file to w24project/
					memset(command,'\0',strlen(command));
					memcpy(command,"mv temp.tar.gz w24project/",strlen("mv temp.tar.gz w24project/"));
					if(system(command) == 0 ) printf("unable to copy file to w24project\n");
				}
				else
				{
					//if no file found, printing below message to user
					char c = fgetc(fp);
					if (c == 'x')
					{
						printf("File not found.\n");
						system("rm empty.txt");
						system("rm temp.tar.gz");
					}
				}*/
			}
			else
			{
				//if not correct input from user, printing below message and all commands
				printf("	Incorrect input, please enter one of the following commands\n");
				printf("	1. Correct Usage [dirlist -a]\n");
				printf("	2. Correct Usage [dirlist -t]\n");
				printf("	3. Correct Usage [w24fn filename]\n");
				printf("	4. Correct Usage [w24fz size1 size2]\n");
				printf("	5. Correct Usage [w24ft ext1 ext2 ext3 - upto 3 extensions]\n");
				printf("	6. Correct Usage [w24fdb date, date format YYYY-MM-DD]\n");
				printf("	7. Correct Usage [w24fda date, date format YYYY-MM-DD]\n");
				printf("        8. quitc to exit\n");
				continue;
			}
		}
	}
	exit(0);
}
