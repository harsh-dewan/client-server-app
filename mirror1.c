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
#include <fcntl.h>
#include <stdbool.h>

/*
   dated_dirs structure that stores the name and date/time(used to sort based on date/time in "dirlist -t" ) for each directory
*/
typedef struct
{
	// stores directory name
	char dir[256];
	// stores date/time for that directory
	time_t when;
} dated_dirs;

/*
   sends tar archive to client
*/
void send_file(int socket, char *file)
{
	// file descriptor for file to be sent to the client
	int f = open(file, O_RDONLY);
	// in case we fail to open the file
	if (f < 0)
	{
		printf("Error opening file\n");
		exit(3);
	}
	// sending the file size first
	off_t file_size = lseek(f, 0, SEEK_END);
	write(socket, &file_size, sizeof(file_size));
	lseek(f, 0, SEEK_SET);
	// sending the actual file data in chunks of 4096 bytes
	char buffer[4096];
	ssize_t bytes_read;
	while ((bytes_read = read(f, buffer, sizeof(buffer))) > 0)
	{
		write(socket, buffer, bytes_read);
	}
	// closing the file after use
	close(f);
}

/*
   compares directories based on date/time (used in "dirlist -t")
   */
int dir_compare(const void *d1, const void *d2)
{
	// d1 and d2 are generic pointers and we're casting them to our struct(date_dirs) type
	const dated_dirs *a = (const dated_dirs *)d1;
	const dated_dirs *b = (const dated_dirs *)d2;
	// if a or d1 is older
	if (a->when < b->when)
	{
		return -1;
	}
	// if b or d2 is older
	else if (a->when > b->when)
	{
		return 1;
	}
	// they're the same age
	else
	{
		return 0;
	}
}

/*
   compares directories and prioritizes alphabetically
*/
int dirpref(const void *d1, const void *d2)
{
	// d1 and d2 are generic pointers and we're casting them to const char ** because they're double pointers(i.e. pointers to string(which is pointer to a char))
	return strcasecmp(*(const char **)d1, *(const char **)d2);
}

/*
handles command requested by client
*/
void crequest(int socket)
{
	struct dirent *dir;
	struct tm tmVar;
	time_t timeVar, timefile;
	char path[1024] = {'\0'};
	char user_date[40] = {'\0'};
	char tarcommand[1024] = {'\0'};
	char when[256] = {'\0'};
	char message[255];
	char *fname_check;
	char *homedir = getenv("HOME");
	DIR *d = opendir(homedir);
	memset(message, '\0', 255);
	memcpy(message,"Connected to Mirror1", strlen("Connected to Mirror1"));
	write(socket, message, strlen(message));
	// forver loop dedicated for a client until the client sends "quitc"
	while (1)
	{
		// stores the client command
		char client_cmd[256] = {'\0'};
		// reading client command
		if (!read(socket, client_cmd, 255))
		{
			close(socket);
			fprintf(stderr, "Client is dead, wait for a new client\n");
			exit(0);
		}
		//printf("client_cmd recieved is <%s>\n", client_cmd);

		// if client enters "dirlist -a"
		if (strcmp("dirlist -a", client_cmd) == 0)
		{
			int count = 0;
			// stores directory list
			char *dirlist[1025];
			// stores path to ~/
			char *homedir = getenv("HOME");
			struct dirent *dir;
			// opening ~/ in dirent variable d
			struct dirent *d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}
			// as long as we're able to read directories (cap of 1024 to avoid overload)
			while ((dir = readdir(d)) != NULL && count < 1024)
			{
				// if the type is DT_DIR(directory) then we need to add it
				if (dir->d_type == DT_DIR)
				{
					// the directory name should not be "." or ".." because these are just references to current and parent directory
					if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
					{
						dirlist[count++] = dir->d_name;
						// printf("->%s\n", dirlist[count - 1]);
					}
				}
			}
			// sorting directories alphabetically
			qsort(dirlist, count, sizeof(char *), dirpref);
			// to store sorted directory names
			char dirnames[1024] = {'\0'};
			// iterating through all sorted directories
			for (int i = 0; i < count; i++)
			{
				// copying directory names to a single string dirnames separated by a new line
				memcpy(dirnames + strlen(dirnames), dirlist[i], strlen(dirlist[i]));
				memcpy(dirnames + strlen(dirnames), "\n", 1);
			}
			// sending the names to client
			if (write(socket, dirnames, strlen(dirnames)) < 0)
				printf("write failed\n");
			printf("sent to client\n");
		}

		// if client enters "dirlist -t"
		if (strcmp("dirlist -t", client_cmd) == 0)
		{
			// counter for dirlist structure
			int count = 0;
			// structure to store directory names and date/times
			dated_dirs dirlist[1024];
			// stores list of directories sorted on date/time
			char *dirlist_final[1024];
			// stores path to ~/
			char *homedir = getenv("HOME");
			// opening ~/ in dirent variable d
			DIR *d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}

			struct dirent *dir;
			// as long as we're able to read directories (cap of 1024 to avoid overload)
			while ((dir = readdir(d)) != NULL && count < 1024)
			{
				// if the type is DT_DIR(directory) then we need to add it
				if (dir->d_type == DT_DIR)
				{
					// the directory name should not be "." or ".." because these are just references to current and parent directory
					if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
					{
						// to get the date and time we need to run stat which accepts path of directory as argument
						char path[1024];
						// storing directory path in path string
						snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
						// running stat
						struct stat statbuf;
						if (stat(path, &statbuf) == 0)
						{
							// dir->d_name gives the name of directory and st_ctime from stat gives the time
							strncpy(dirlist[count].dir, dir->d_name, sizeof(dirlist[count].dir));
							dirlist[count].when = statbuf.st_ctime;
							count++;
						}
					}
				}
			}
			// sorting based on time (older directory first)
			qsort(dirlist, count, sizeof(dated_dirs), dir_compare);
			// storing the sorted directory list to dirlist_final list
			for (int i = 0; i < count; i++)
			{
				dirlist_final[i] = dirlist[i].dir;
				// printf("dirname <%s> and time <%d>\n", dirlist[i].dir, dirlist[i].when);
			}
			// to store sorted directory names
			char dirnames[1024] = {'\0'};
			// iterating through all sorted directories
			for (int i = 0; i < count; i++)
			{
				// copying directory names to a single string dirnames separated by a new line
				memcpy(dirnames + strlen(dirnames), dirlist_final[i], strlen(dirlist_final[i]));
				memcpy(dirnames + strlen(dirnames), "\n", 1);
			}
			// sending the names to client
			if (write(socket, dirnames, strlen(dirnames)) < 0)
				printf("write failed\n");
			printf("sent to client\n");
		}

		// if client enters "w24fn" command
		if (strncmp(client_cmd, "w24fn", 5) == 0)
		{
			// flag for checking if file exists
			int exists = 0;
			// getting filename from client input
			char *fname = malloc(strlen(client_cmd) - 5);
			strcpy(fname, client_cmd + 6);
			// int count = 0;
			// stores path to ~/
			char *homedir = getenv("HOME");
			// opening ~/ in dirent variable d
			DIR *d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}
			struct dirent *dir;
			// stores permissions for file
			char perms[10] = {'\0'};
			// as long as we're able to read files
			while ((dir = readdir(d)) != NULL)
			{
				// checking if the type is DT_REG(a file)
				if (dir->d_type == DT_REG)
				{
					// if the current file name matches client input
					if (!strcmp(dir->d_name, fname))
					{
						// raise the flag that file is found
						exists = 1;
						// getting the path and running stat to get file information
						char path[1024];
						snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
						struct stat statbuf;
						if (stat(path, &statbuf) == 0)
						{
							// storting file time by converting ctime to human readable format
							char size_in_string[256] = {'\0'};
							snprintf(size_in_string, sizeof(size_in_string), "%ld", (long)statbuf.st_size);
							struct tm *time = localtime(&statbuf.st_ctime);
							char when[256] = {'\0'};
							strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", time);
							// using st_mode to store file permissions in perms string
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
							// storing required file infomation to fileinfo string
							char *fileinfo = malloc(strlen(dir->d_name) + strlen(size_in_string) + strlen(when) + strlen(perms) + 4);
							fileinfo[0] = '\0';
							strcat(fileinfo, dir->d_name);
							strcat(fileinfo, " ");
							strcat(fileinfo, size_in_string);
							strcat(fileinfo, " ");
							strcat(fileinfo, when);
							strcat(fileinfo, " ");
							strcat(fileinfo, perms);
							//printf("->%s\n", fileinfo);
							// sending the file information to client
							if (write(socket, fileinfo, strlen(fileinfo)) < 0)
								printf("write failed\n");
							printf("sent to client\n");
						}
						// only sending the first occurence
						break;
					}
				}
			}
			// in case the file does not exist
			if (!exists)
			{
				if (write(socket, "File not found", 14) < 0)
				{
					printf("write failed\n");
				}
				printf("file not found, sent to client\n");
			}
		}

		// id the client enters "w24fz" command
		if (strncmp(client_cmd, "w24fz", 5) == 0)
		{
			// flag to check if file/s exists or not
			int exists = 0;
			// int count = 0;
			// breaking client input based on whitespaces to get lower bound and upper bound on file size
			strtok(client_cmd, " ");
			char *s1 = strtok(NULL, " ");
			char *s2 = strtok(NULL, " ");
			// converting file sizes to int
			long int size1 = atoi(s1);
			long int size2 = atoi(s2);
			//printf("%ld\t%ld\n", size1, size2);
			// stores files matching size >= s1 & size <= s2
			char file_list[10000] = {'\0'};
			// stores path to ~/
			char *homedir = getenv("HOME");
			// opening ~/ in dirent variable d
			DIR *d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}
			struct dirent *dir;
			// as long as we're able to read files
			while ((dir = readdir(d)) != NULL)
			{
				// checking if the type is DT_REG(a file)
				if (dir->d_type == DT_REG)
				{
					// the directory name should not be "." or ".." because these are just references to current and parent directory
					if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
					{
						char path[1024];
						// running stat to get file size
						snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
						struct stat statbuf;
						if (stat(path, &statbuf) == 0)
						{
							// if file size is in required bounds, add it to the file_list separated by white space
							if (statbuf.st_size >= size1 && statbuf.st_size <= size2)
							{
								exists = 1;
								//printf("NAME: %s\tSIZE: %ld\n", dir->d_name, statbuf.st_size);
								strcat(file_list, dir->d_name);
								strcat(file_list, " ");
							}
						}
					}
				}
			}
			// if required file/s exists
			if (exists)
			{
				//printf("FILELIST-> %s\n", file_list);
				// using system to make tar of all the files that were within size limits and compressing to temp.tar.gz
				char create_tar[10000] = {'\0'}, create_tar_gz[256] = {'\0'};
				snprintf(create_tar, sizeof(create_tar), "tar -cvf %s %s 2>/dev/null 1>/dev/null", "temp.tar", file_list);
				int status = system(create_tar);
				snprintf(create_tar_gz, sizeof(create_tar_gz), "gzip -f %s 2>/dev/null 1>/dev/null", "temp.tar");
				status = system(create_tar_gz);
				// in case system commands fail
				if (status == -1)
				{
					perror("Failed to execute command");
					//return 1;
				}
				else
				{
					// write(socket,"FP",2);
					// sending temp.tar.gz to client
					send_file(socket, "temp.tar.gz");
					printf("sent to client\n");
				}
			}
			// required file/s does not exist
			else
			{
				// create a file with 'x' in it and send it to client as a sign that required file/s were not found
				system("echo x > empty.txt");
				system("tar -cvf temp.tar empty.txt 2>/dev/null 1>/dev/null");
				system("gzip -f temp.tar 2>/dev/null 1>/dev/null");
				send_file(socket, "temp.tar.gz");
				system("rm empty.txt");
				printf("sent to client\n");
			}
		}

		// if client enters "w24ft" command
		if (strncmp(client_cmd, "w24ft", 5) == 0)
		{
			// flag if file/s exist or not
			int exists = 0;
			// breaking the input command on white spaces to get the file extensions
			strtok(client_cmd, " ");
			char *type1 = NULL, *type2 = NULL, *type3 = NULL;
			type1 = strtok(NULL, " ");
			type2 = strtok(NULL, " ");
			type3 = strtok(NULL, " ");
			//printf("%s\t%s\t%s\n", type1, type2, type3);
			// stores list of files with matching extensions
			char file_list[10000] = {'\0'};
			// stores path to ~/
			char *homedir = getenv("HOME");
			// opening ~/ in dirent variable d
			DIR *d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open directory!\n");
				exit(3);
			}
			struct dirent *dir;
			// as long as we're able to read files
			while ((dir = readdir(d)) != NULL)
			{
				// checking if the type is DT_REG(a file)
				if (dir->d_type == DT_REG)
				{
					// the directory name should not be "." or ".." because these are just references to current and parent directory
					if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
					{
						// storing file name in temporary variable to isolate the file extension
						char temp_fname[256] = {'\0'};
						strcpy(temp_fname, dir->d_name);
						//printf("->%s\n", temp_fname);
						// stores file extension
						char ext[64] = {'\0'};
						int itr = 0, checknow = 0, k = 0;
						// iterate over file name copy
						while (temp_fname[itr] != '\0')
						{
							// if checknow is true, start storing the file extension characters
							if (checknow)
								ext[k++] = temp_fname[itr];
							// when we see '.', start storing because it marks the start of file extension
							if (temp_fname[itr] == '.')
								checknow = 1;
							itr++;
						}
						//printf("EXTENSION: %s\n", ext);
						// if any of the input extensions match current file extension, add the current file to file_list
						if (((type1 != NULL) && !strncmp(ext, type1, strlen(type1))) || ((type2 != NULL) && !strncmp(ext, type2, strlen(type2))) || ((type3 != NULL) && !strncmp(ext, type3, strlen(type3))))
						{
							exists = 1;
							strcat(file_list, dir->d_name);
							strcat(file_list, " ");
						}
					}
				}
			}
			// required file/s exists
			if (exists)
			{
				//printf("FILELIST-> %s\n", file_list);
				// use system to add the matching file_list to tar archive and then compress to .tar.gz
				char create_tar[10000] = {'\0'}, create_tar_gz[256] = {'\0'};
				snprintf(create_tar, sizeof(create_tar), "tar -cvf %s %s 2>/dev/null 1>/dev/null", "temp.tar", file_list);
				int status = system(create_tar);
				snprintf(create_tar_gz, sizeof(create_tar_gz), "gzip -f %s 2>/dev/null 1>/dev/null", "temp.tar");
				status = system(create_tar_gz);
				// in case system fails
				if (status == -1)
				{
					perror("Failed to execute command");
					//return 1;
				}
				else
				{
					// send the temp.tar.gz to client
					//printf("Tar command executed successfully\n");
					send_file(socket, "temp.tar.gz");
					printf("sent to client\n");
				}
			}
			// required file/s does not exist
			else
			{
				// create a file with 'x' in it and send it to client as a sign that required file/s were not found
				system("echo x > empty.txt");
				system("tar -cvf temp.tar empty.txt 2>/dev/null 1>/dev/null");
				system("gzip -f temp.tar 2>/dev/null 1>/dev/null");
				send_file(socket, "temp.tar.gz");
				system("rm empty.txt");
				printf("sent to client\n");
			}
		}

		// if client enters "w24fdb" command
		if (strncmp(client_cmd, "w24fdb ", strlen("w24fdb ")) == 0)
		{
			char fname[10000]= {'\0'};
			// handling for w24fdb
			//printf("received w24fdb\n");
			// initializing strings to NULL values to avoid garbage values
			memset(tarcommand, '\0', strlen(tarcommand));
			memset(user_date, '\0', 40);
			// flags to indicate if required files are present and in case there's an error in creating tar archive
			int file_present = 0, tar_file_error = 0;
			// fetching the date from client input
			memcpy(user_date + strlen(user_date), client_cmd + 7, strlen(client_cmd + 7));
			memcpy(user_date + strlen(user_date), " 00:00:00", strlen(" 00:00:00"));
			//printf("date supplied by user <%s>\n", user_date);
			sscanf(user_date, "%d-%d-%d %d:%d:%d", &tmVar.tm_year, &tmVar.tm_mon, &tmVar.tm_mday,
				   &tmVar.tm_hour, &tmVar.tm_min,
				   &tmVar.tm_sec);
			tmVar.tm_isdst = 1;
			tmVar.tm_year -= 1900;
			tmVar.tm_mon--;
			timeVar = mktime(&tmVar);
			//printf("time from epoch for user_date <%d>\n", timeVar);
			// opening ~/ in dirent variable d
			d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open home directory!\n");
				exit(3);
			}
			// as long as we're able to read files
			while ((dir = readdir(d)) != NULL)
			{
				// checking if the type is DT_REG(a file)
				if (dir->d_type == DT_REG)
				{
					// running stat to get the date, year and time for the all the files
					memset(path, '\0', strlen(path));
					snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
					struct stat statbuf;
					if (stat(path, &statbuf) == 0)
					{
						memset(when, '\0', strlen(when));
						struct tm *time = localtime(&statbuf.st_ctime);
						strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", time);
						sscanf(when, "%d-%d-%d %d:%d:%d", &tmVar.tm_year, &tmVar.tm_mon, &tmVar.tm_mday,
							   &tmVar.tm_hour, &tmVar.tm_min,
							   &tmVar.tm_sec);
						tmVar.tm_isdst = 1;
						tmVar.tm_year -= 1900;
						tmVar.tm_mon--;
						timefile = mktime(&tmVar);
						//printf("atimefile <%d>  and tmVar <%d>\n", timefile, timeVar);
						// comparing the epoch time for all files if it is less than the epoch time of the user supplied date
						// creating tar if the condition is satisfied
						if (timefile <= timeVar)
						{
							file_present = 1;
							memcpy(fname+strlen(fname),dir->d_name,strlen(dir->d_name));
							memcpy(fname+strlen(fname)," ",1);
							//printf("fname <%s>\n",fname);
							//sprintf(tarcommand, "tar -rf temp.tar %s --absolute-names 2>/dev/null 1>/dev/null", dir->d_name);
							//printf("file <%s> created on <%s> and is before <%s> and tarcommand <%s>\n", path, when, user_date, tarcommand);
							//if (system(tarcommand) != 0)
							//{
								//printf("unable to create tar file\n");
								//tar_file_error = 1;
							//}
						}
					}
				}
			}
			if (file_present == 0 )
			{
				// create a file with 'x' in it and send it to client as a sign that required file/s were not found
				system("echo x > empty.txt");
				system("tar -cvf temp.tar empty.txt 2>/dev/null 1>/dev/null");
				system("gzip -f temp.tar 2>/dev/null 1>/dev/null");
				send_file(socket, "temp.tar.gz");
				system("rm empty.txt");
				printf("sent to client\n");
			}
			// required file/s exist
			else
			{


				//printf("FILELIST-> %s\n", fname);
				// use system to add the matching file_list to tar archive and then compress to .tar.gz
				char create_tar[10000] = {'\0'}, create_tar_gz[256] = {'\0'};
				snprintf(create_tar, sizeof(create_tar), "tar -cvf %s %s 2>/dev/null 1>/dev/null", "temp.tar", fname);
				int status = system(create_tar);
				snprintf(create_tar_gz, sizeof(create_tar_gz), "gzip -f %s 2>/dev/null 1>/dev/null", "temp.tar");
				status = system(create_tar_gz);
				// in case system fails
				if (status == -1)
				{
					perror("Failed to execute command");
					//return 1;
				}
				else
				{
					// send the temp.tar.gz to client
					//printf("Tar command executed successfully\n");
					send_file(socket, "temp.tar");
					printf("sent to client\n");
				}

/*
				printf("files are present\n");
				// compressing the .tar to .tar.gz and sending to the client
				system("gzip -f temp.tar");
				printf("before sleep\n");
				sleep(20);
				printf("sending tar file to client\n");
				send_file(socket, "temp.tar.gz");
				printf("tar file sent to client\n");*/
			}

			// if (write(socket, "enter next date", strlen("enter_next_date")) < 0) printf("write failed\n");
			// ##########
		}

		// if the client enters "w24fda" command
		if (strncmp(client_cmd, "w24fda ", strlen("w24fda ")) == 0)
		{
			// handling for w24fdb
			// initializing strings to NULL values to avoid garbage values
			memset(tarcommand, '\0', strlen(tarcommand));
			memset(user_date, '\0', 40);
			// flags to indicate if required files are present and in case there's an error in creating tar archive
			int file_present = 0, tar_file_error = 0;
			// fetching the date from client input
			memcpy(user_date + strlen(user_date), client_cmd + 7, strlen(client_cmd + 7));
			memcpy(user_date + strlen(user_date), " 00:00:00", strlen(" 00:00:00"));
			//printf("date supplied by user <%s>\n", user_date);
			sscanf(user_date, "%d-%d-%d %d:%d:%d", &tmVar.tm_year, &tmVar.tm_mon, &tmVar.tm_mday,
				   &tmVar.tm_hour, &tmVar.tm_min,
				   &tmVar.tm_sec);
			tmVar.tm_isdst = 1;
			tmVar.tm_year -= 1900;
			tmVar.tm_mon--;
			timeVar = mktime(&tmVar);
			//printf("time from epoch for user_date <%d>\n", timeVar);
			// ##########
			// opening ~/ in dirent variable d
			d = opendir(homedir);
			// in case it fails to open ~/
			if (d == NULL)
			{
				printf("Failed to open home directory!\n");
				exit(3);
			}
			// as long as we're able to read files
			while ((dir = readdir(d)) != NULL)
			{
				// checking if the type is DT_REG(a file)
				if (dir->d_type == DT_REG)
				{
					// running stat to get the date, year and time for the all the files
					memset(path, '\0', strlen(path));
					memset(when, '\0', strlen(when));
					snprintf(path, sizeof(path), "%s/%s", homedir, dir->d_name);
					struct stat statbuf;
					if (stat(path, &statbuf) == 0)
					{
						struct tm *time = localtime(&statbuf.st_ctime);
						strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", time);
						sscanf(when, "%d-%d-%d %d:%d:%d", &tmVar.tm_year, &tmVar.tm_mon, &tmVar.tm_mday,
							   &tmVar.tm_hour, &tmVar.tm_min,
							   &tmVar.tm_sec);
						tmVar.tm_isdst = 1;
						tmVar.tm_year -= 1900;
						tmVar.tm_mon--;
						timefile = mktime(&tmVar);
						// comparing the epoch time for all files if it is more than the epoch time of the user supplied date
						// creating tar if the condition is satisfied
						if (timefile >= timeVar)
						{
							file_present = 1;
							sprintf(tarcommand, "tar -rf temp.tar %s --absolute-names 2>/dev/null 1>/dev/null", dir->d_name);
							//printf("file <%s> created on <%s> and is after <%s> and tarcommand <%s>\n", dir->d_name, when, user_date, tarcommand);
							if (system(tarcommand) != 0)
							{
								tar_file_error = 1;
								printf("unable to create tar file\n");
							}
						}
					}
				}
			}
			// required file/s does not exist
			if (file_present == 0 || tar_file_error == 1)
			{
				// create a file with 'x' in it and send it to client as a sign that required file/s were not found
				system("echo x > empty.txt");
				system("tar -cvf temp.tar empty.txt 2>/dev/null 1>/dev/null");
				system("gzip -f temp.tar 2>/dev/null 1>/dev/null");
				send_file(socket, "temp.tar.gz");
				system("rm empty.txt");
				printf("sent to client\n");
			}
			// required file/s exist
			else
			{
				// compressing the .tar to .tar.gz and sending to the client
				system("gzip -f temp.tar");
				//printf("sending tar file to client\n");
				send_file(socket, "temp.tar.gz");
				printf("sent to client\n");
			}
		}
	}
}

int main(int argc, char const *argv[])
{
	// declaring variables to switch client requests between server, mirror 1 and mirror 2
	int reqcount = 0;
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
	listen(serversocket, 100);

	// to keep listening for client requests
	while (1)
	{
		// accepts the client request
		connectedsocket = accept(serversocket, (struct sockaddr *)NULL, NULL);
		if (connectedsocket < 0)
		{
			printf("Error in connecting to client!\n");
			exit(2);
		}
		// incrementing the request count
		reqcount += 1;
		printf("Got a client <%d>, message from mirror1\n", reqcount);
		// creating fork() for each client
		if (fork() == 0)
			crequest(connectedsocket);
		close(connectedsocket);
	}
	return 0;
}
