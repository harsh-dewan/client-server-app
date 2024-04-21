/*
   ASP COURSE PROJECT
   SERVER
   */

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

typedef struct
{
	char dir[256];
	time_t when;
} dated_dirs;

int dir_compare(const void *d1, const void *d2)
{
	const dated_dirs *a = (const dated_dirs *)d1;
	const dated_dirs *b = (const dated_dirs *)d2;
	if (a->when < b->when)
	{
		return -1;
	}
	else if (a->when > b->when)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int dirpref(const void *d1, const void *d2)
{
	return strcmp(*(const char **)d1, *(const char **)d2);
}

void child(int socket)
{
	char client_cmd[256] = {'\0'};
	while (1)
	{
		memset(client_cmd, '\0', strlen(client_cmd));
		if (!read(socket, client_cmd, 255))
		{
			close(socket);
			fprintf(stderr, "Client is dead, wait for a new client\n");
			exit(0);
		}

		if (strcmp("dirlist -a", client_cmd) == 0)
		{
			int count = 0;
			char *dirlist[1025];
			char *homedir = getenv("HOME");
			struct dirent *dir;
			struct dirent *d = opendir(homedir);
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}
			while ((dir = readdir(d)) != NULL && count < 1024)
			{
				if (dir->d_type == DT_DIR)
				{
					if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
					{
						dirlist[count++] = dir->d_name;
						printf("->%s\n", dirlist[count - 1]);
					}
				}
			}

			qsort(dirlist, count, sizeof(char *), dirpref);
			char dirnames[1024] = {'\0'};
			for (int i = 0; i < count; i++)
			{
				memcpy(dirnames + strlen(dirnames), dirlist[i], strlen(dirlist[i]));
				memcpy(dirnames + strlen(dirnames), "\n", 1);
			}
			if (write(socket, dirnames, strlen(dirnames)) < 0)
				printf("write failed\n");
			printf("sent to client\n");
		}

		if (strcmp("dirlist -t", client_cmd) == 0)
		{
			int count = 0;
			dated_dirs dirlist[1024];
			char *dirlist_final[1024];
			char *homedir = getenv("HOME");
			DIR *d = opendir(homedir);
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}

			struct dirent *dir;
			while ((dir = readdir(d)) != NULL)
			{
				if (dir->d_type == DT_DIR)
				{
					if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
					{
						char path[1024];
						snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
						struct stat statbuf;
						if (stat(path, &statbuf) == 0)
						{
							strncpy(dirlist[count].dir, dir->d_name, sizeof(dirlist[count].dir));
							dirlist[count].when = statbuf.st_ctime;
							count++;
						}
					}
				}
			}
			qsort(dirlist, count, sizeof(dated_dirs), dir_compare);
			for (int i = 0; i < count; i++)
			{
				dirlist_final[i] = dirlist[i].dir;
				printf("dirname <%s> and time <%d>\n", dirlist[i].dir, dirlist[i].when);
			}
			// write(socket, dirlist_final, sizeof(dirlist_final));
			char dirnames[1024] = {'\0'};
			for (int i = 0; i < count; i++)
			{
				memcpy(dirnames + strlen(dirnames), dirlist_final[i], strlen(dirlist_final[i]));
				memcpy(dirnames + strlen(dirnames), "\n", 1);
			}
			if (write(socket, dirnames, strlen(dirnames)) < 0)
				printf("write failed\n");
			printf("sent to client\n");
		}

		char *fname_check;
		if ((fname_check = strstr(client_cmd, "w24fn")) != NULL)
		{
			int exists = 0;
			char final_details[4040] = {'\0'};
			char fname[256] = {'\0'};
			printf("clinet_cmd <%s>: \n", client_cmd);
			printf("%s\n", fname);
			memcpy(fname, client_cmd + 6, strlen(client_cmd + 6));
			// strcpy(fname, client_cmd + 6);
			printf("%s\n", fname);
			int count = 0;
			char *fileinfo[1024];
			char fileinfo_final[2048] = {'\0'};
			char *homedir = getenv("HOME");
			DIR *d = opendir(homedir);
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}

			struct dirent *dir;
			char perms[10] = {'\0'};
			memset(final_details,'\0',strlen(final_details));
			while ((dir = readdir(d)) != NULL)
			{
				if (dir->d_type == DT_REG)
				{
					if (!strcmp(dir->d_name, fname))
					{
						printf("%s\n", dir->d_name);
						exists = 1;
						char path[1024];
						snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
						printf("->%s\n", path);
						struct stat statbuf;
						if (stat(path, &statbuf) == 0)
						{
							memcpy(final_details+strlen(final_details),dir->d_name, strlen(dir->d_name));
							memcpy(final_details+strlen(final_details)," ",1);

							char size_in_string[256] = {'\0'};
							snprintf(size_in_string, sizeof(size_in_string), "%ld", (long)statbuf.st_size);
							memcpy(final_details+strlen(final_details), size_in_string, strlen(size_in_string));
							memcpy(final_details+strlen(final_details)," ",1);
							
							struct tm *time = localtime(&statbuf.st_ctime);
							char when[256] = {'\0'};
							strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", time);
							memcpy(final_details+strlen(final_details), when, strlen(when));
							memcpy(final_details+strlen(final_details)," ",1);
							
							if ((statbuf.st_mode & S_IRUSR))
							{
								perms[0] = 'r';
							}
							else
							{
								perms[0] = '-';
							}
							if ((statbuf.st_mode & S_IWUSR))
							{
								perms[1] = 'w';
							}
							else
							{
								perms[1] = '-';
							}
							if ((statbuf.st_mode & S_IXUSR))
							{
								perms[2] = 'x';
							}
							else
							{
								perms[2] = '-';
							}
							if ((statbuf.st_mode & S_IRGRP))
							{
								perms[3] = 'r';
							}
							else
							{
								perms[3] = '-';
							}
							if ((statbuf.st_mode & S_IWGRP))
							{
								perms[4] = 'w';
							}
							else
							{
								perms[4] = '-';
							}
							if ((statbuf.st_mode & S_IXGRP))
							{
								perms[5] = 'x';
							}
							else
							{
								perms[5] = '-';
							}
							if ((statbuf.st_mode & S_IROTH))
							{
								perms[6] = 'r';
							}
							else
							{
								perms[6] = '-';
							}
							if ((statbuf.st_mode & S_IWOTH))
							{
								perms[7] = 'w';
							}
							else
							{
								perms[7] = '-';
							}
							if ((statbuf.st_mode & S_IXOTH))
							{
								perms[8] = 'x';
							}
							else
							{
								perms[8] = '-';
							}
							memcpy(final_details+strlen(final_details), perms, strlen(perms));
							memcpy(final_details+strlen(final_details),"\n",1);
						}
					}
				}
			}
			if (exists)
			{
				if (write(socket, final_details, strlen(final_details)) < 0) printf("write failed\n");
				printf("sent to client <%s>\n", final_details);
			}
			else
			{
				if (write(socket, "CODE404", 7) < 0)
					printf("write failed\n");
				printf("sent to client\n");
			}
		}
	}
}

int main(int argc, char const *argv[])
{

	while (1)
	{
		// to store the port number and socket once we get clients connected
		int port, connectedsocket;

		// invalid parameters
		if (argc != 2)
		{
			printf("Usage: %s <Port number>\n", argv[0]);
			exit(0);
		}

		// creating socket for server (SOCK_STREAM to specify TCP connection)
		int serversocket = socket(AF_INET, SOCK_STREAM, 0);

		// socket creation failed
		if (serversocket < 0)
		{
			printf("Error in creating socket\n");
			exit(1);
		}

		// server address
		struct sockaddr_in serveraddress;

		// AF_INET domain for address family
		serveraddress.sin_family = AF_INET;

		// reading the port number
		sscanf(argv[1], "%d", &port);
		serveraddress.sin_port = htons((uint16_t)port);

		// getting the IP address internally
		serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);

		// bind socket to the IP address and port number
		bind(serversocket, (struct sockaddr *)&serveraddress, sizeof(serveraddress));

		// listening for client connections
		listen(serversocket, 10);

		while (1)
		{
			connectedsocket = accept(serversocket, (struct sockaddr *)NULL, NULL);
			if (connectedsocket < 0)
			{
				printf("Error in connecting to client!\n");
				exit(2);
			}
			printf("got a client\n");
			if (fork() == 0)
				child(connectedsocket);
			close(connectedsocket);
		}
	}
	return 0;
}
