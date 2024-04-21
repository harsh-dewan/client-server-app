/*
 * 1. test all cases of user input
 * 2. adding comments
 * 3. section 3, alternate server and mirror servers
 * 4. complete run
 * 5. check date input validation and other user input validation
 * 6. update the code for socket part
 * 7. all files returned from server must be saved in a folder named w24project in home directory
 * 8. try with mulitple clients
 * 9. print all commands in case of wrong input
 * 10. check all the error handling part, if correct message is displayed
 * 11. validate the size in the input w24fz
 * 12. filename illegal characters in filename w24fz
 */

#include <netinet/in.h> //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //for socket APIs
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

/*
 * to check if the year is leap year or not
 * used in validating date
 */
bool isLeapYear(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
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
	printf("date received in the isDateValid function <%s>\n", inputDate);
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
		// printf("char: <%c>\n",ch);
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
	// printf("year <%s> month <%s> day <%s>\n",year, month, day);
	int dyear = atoi(year);
	int dmonth = atoi(month);
	int dday = atoi(day);
	// printf("dyear <%d> dmonth <%d> dday <%d>\n",dyear, dmonth, dday);
	if (checkYearMonthDay(dyear, dmonth, dday))
		return 0;
	return 1;
}

int isSizeValid(char *inputsize)
{

	int index = 0, strlength = strlen(inputsize);
	while (index < strlength)
	{
		char ch = inputsize[index];
		if (ch < '0' || ch > '9')
			return 1;
		index++;
	}
	return 0;
}

int isFilenameValid(char *filename)
{

	int index = 0, filenamelength = strlen(filename);
	while (index < filenamelength)
	{
		char ch = filename[index];
		if (ch == '/')
			return 1;
		index++;
	}
	return 0;
}

// Function to trim leading and trailing whitespace characters
char *trim(char *str)
{
	// Trim leading spaces
	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0) // All spaces?
		return str;

	// Trim trailing spaces
	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
		end--;

	// Null-terminate the trimmed string
	*(end + 1) = '\0';

	return str;
}

int main(int argc, char *argv[])
{

	// E.g., 1, client

	char message[1024];
	int server, portNumber;
	socklen_t len;
	struct sockaddr_in servAdd;

	if (argc != 3)
	{
		printf("Call model:%s <IP> <Port#>\n", argv[0]);
		exit(0);
	}

	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{ // socket()
		fprintf(stderr, "Cannot create socket\n");
		exit(1);
	}

	servAdd.sin_family = AF_INET; // Internet
	sscanf(argv[2], "%d", &portNumber);
	servAdd.sin_port = htons((uint16_t)portNumber); // Port number

	if (inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0)
	{
		fprintf(stderr, " inet_pton() has failed\n");
		exit(2);
	}

	if (connect(server, (struct sockaddr *)&servAdd, sizeof(servAdd)) < 0)
	{ // Connect()
		fprintf(stderr, "connect() failed, exiting\n");
		exit(3);
	}

	/*if ( read(server, message, 100) < 0 ) {//read()
	  fprintf(stderr, "read() error\n");
	  exit(3);
	  }
	  fprintf(stdout, "%s\n", message);*/

	char user_input[1024] = {'\0'};
	char command[100] = {'\0'};
	char date[15] = {'\0'};
	char size[15] = {'\0'};
	char filename[100] = {'\0'};
	while (1)
	{
		memset(command, '\0', 100);
		memset(user_input, '\0', 1024);
		printf("client24$ ");
		fgets(user_input, 1024, stdin);
		char *trim_user_input = trim(user_input);
		if (strlen(trim_user_input) == 0)
			continue;
		else if (memcmp(trim_user_input, "quitc", strlen("quitc")) == 0)
		{
			printf("Terminating this session, bye!\n");
			exit(0);
		}
		else
		{
			// printf("user_input <%s>\n",trim_user_input);
			if (memcmp(trim_user_input, "dirlist", strlen("dirlist")) == 0)
			{
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
					printf("	Correct Usage [dirlist -a] or [dirlist -t]\n");
					continue;
				}
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
				char buffer[1025] = {'\0'};
				if (read(server, buffer, 1024) < 0)
				{ // read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}

				char *token = strtok(buffer, "\n");
				while (token != NULL)
				{
					printf("dirlist: Directory: %s\n", token);
					token = strtok(NULL, "\n");
				}

				/*int index = 0;
				while (dirlist[index] != '\0')
				{
					printf("dirlist <%s>\n", dirlist[index]);
					index++;
				}*/
				// fprintf(stdout, "%s\n", message);
			}
			else if (memcmp(trim_user_input, "w24fn", strlen("w24fn")) == 0)
			{

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
				write(server, trim_user_input, strlen(trim_user_input));
				memset(message, '\0', 1024);
				if (read(server, message, 1024) < 0)
				{ // read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}

				fprintf(stdout, "%s\n", message);
			}
			else if (memcmp(trim_user_input, "w24fz", strlen("w24fz")) == 0)
			{

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
					printf("	Correct Usage [w24fz size1 size2]\n");
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
				if (isSizeValid(size1) != 0 || isSizeValid(size2) != 0)
				{
					printf("	Incorrect size values, please try again!\n");
					printf("	Correct Usage [w24fz size1 size2]\n");
					continue;
				}
				write(server, trim_user_input, strlen(trim_user_input));
				if (read(server, message, 100) < 0)
				{ // read()
					fprintf(stdout, "read() error\n");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);
			}
			else if (memcmp(trim_user_input, "w24ft", strlen("w24ft")) == 0)
			{

				// Usage[w24ft ext1 ext2 ext3]
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
				if (read(server, message, 100) < 0)
				{ // read()
					fprintf(stdout, "read() error");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);
			}
			else if (memcmp(trim_user_input, "w24fdb", strlen("w24fdb")) == 0)
			{

				// Usage [w24fdb date]
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
				if (read(server, message, 100) < 0)
				{ // read()
					fprintf(stdout, "read() error");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);
			}
			else if (memcmp(trim_user_input, "w24fda", strlen("w24fda")) == 0)
			{
				// Usage [w24fda date]
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
					printf("	Correct Usage [w24fdb date, date format YYYY-MM-DD]\n");
					continue;
				}
				write(server, trim_user_input, strlen(trim_user_input));
				if (read(server, message, 100) < 0)
				{ // read()
					fprintf(stdout, "read() error");
					exit(3);
				}
				fprintf(stdout, "%s\n", message);
			}
			else
			{
				printf("	Incorrect input, please enter correct command\n");
				continue;
			}
		}
	}
	exit(0);
}