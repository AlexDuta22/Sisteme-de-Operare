#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <libgen.h>

DIR *openDirectory(char *nume)
{
	DIR *dir;
	dir=opendir(nume);
	if(dir==NULL)
	{
		exit(-1);
	}
	return dir;
}
void write_in_Snapshot(int file, struct stat file_info, struct dirent *entry)
{
	time_t current_time=time(NULL);
	char *ora=ctime(&current_time);
	
	if (write(file, "Timestamp:", strlen("Timestamp:")) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
	if (write(file, ora, strlen(ora)) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}

	if (write(file, "Last modified:", strlen("Last modified:")) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}

	if (write(file, ctime(&file_info.st_mtime), strlen(ctime(&file_info.st_mtime))) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
	
	
	char buffer[500]="nume fisier:";
	strcat(buffer, entry->d_name);		
	if (write(file, buffer, strlen(buffer)) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
	
	sprintf(buffer,"\ndimensiune: %ld\n", file_info.st_size);
	if (write(file, buffer, strlen(buffer)) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
					
	sprintf(buffer,"inode: %ld\n", file_info.st_ino);
	if (write(file, buffer, strlen(buffer)) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
	
	if (write(file, "Permissions: ", strlen("Permissions: ")) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
	if (write(file, ((S_ISDIR(file_info.st_mode)) ? "d" : "-"), 1) == -1) {
		perror("eroare la scriere in snapshot\n");
		exit(-10);
	}
	write(file, ((file_info.st_mode & S_IRUSR) ? "r" : "-"),1);
	write(file, ((file_info.st_mode & S_IWUSR) ? "w" : "-"),1);	
	write(file, ((file_info.st_mode & S_IXUSR) ? "x" : "-"),1);
	write(file, ((file_info.st_mode & S_IRGRP) ? "r" : "-"),1);
	write(file, ((file_info.st_mode & S_IWGRP) ? "w" : "-"),1);
	write(file, ((file_info.st_mode & S_IXGRP) ? "x" : "-"),1);
	write(file, ((file_info.st_mode & S_IROTH) ? "r" : "-"),1);
	write(file, ((file_info.st_mode & S_IWOTH) ? "w" : "-"),1);
	write(file, ((file_info.st_mode & S_IXOTH) ? "x" : "-"),1);
	
	
}

int checkCorupt(char pathFile[1000], struct stat file_info) {
	int corupt = 1;

	if (file_info.st_mode & S_IRUSR)
		corupt = 0;
	if (file_info.st_mode & S_IWUSR)
		corupt = 0;
	if (file_info.st_mode & S_IXUSR)
		corupt = 0;
	if (file_info.st_mode & S_IRGRP)
		corupt = 0;
	if (file_info.st_mode & S_IWGRP)
		corupt = 0;
	if (file_info.st_mode & S_IXGRP)
		corupt = 0;
	if (file_info.st_mode & S_IROTH)
		corupt = 0;
	if (file_info.st_mode & S_IWOTH)
		corupt = 0;
	if (file_info.st_mode & S_IXOTH)
		corupt = 0;

	return corupt;
}

void mutare_fisier_corupt(char path[1000], char pathCorupt[1000]) {
	char newPath[1000];

	sprintf(newPath, "%s/%s", pathCorupt, basename(path));

	if (rename(path, newPath) != 0)
	{
		perror("nu sa putut muta fisierul corupt\n");
		exit(-11);
	}
	else
	{
		printf("sa mutat fisierul corupt\n");
	}
}


void checkDir(DIR* dir, char path[1000],char path_out[100], int k,int out,char izo[1000],int countPericulos,char script)
{
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		char pathaux[1000];
		strcpy(pathaux, path);
		strcat(pathaux, "/");
		strcat(pathaux, entry->d_name);
		/*
		for (int i = 0; i < k; i++)
		{
			printf("\t");
		}
		printf("--->%s\n", entry->d_name);
		*/
		struct stat file_info;
		if (lstat(pathaux, &file_info) == -1)
		{
			perror("eroare lstat\n");
			exit(-12);
		}
		if (S_ISDIR(file_info.st_mode))  //este director
		{
			DIR* d = opendir(pathaux);
			if (d == NULL)
			{
				exit(-2);
			}
			checkDir(d, pathaux, path_out, k + 1, out, izo, countPericulos,script);
		}
		else
		{
			if (S_ISREG(file_info.st_mode))  //este fisier
			{
				if (checkCorupt(pathaux, file_info))
				{
					pid_t pidNepot;

					pid_t pipe_file_desciptor[2];

					if (pipe(pipe_file_desciptor) < 0)
					{
						perror("eroare pipe file descriptor\n");
						exit(-8);
					}
					if ((pidNepot = fork()) < 0) 
					{
						perror("eroare pid Nepot\n");
						exit(-8);
					}
					else
					{
						if (pidNepot == 0)
						{
							if (close(pipe_file_desciptor[0]) != 0)
							{
								perror("eroare pipe1\n");
								exit(-9);
							}
							//inchid capatul de citire

							if (dup2(pipe_file_desciptor[1], 1) == -1)
							{
								perror("eroare pipe2\n");
								exit(-9);
							}
							execl("/bin/bash", "bin", script, pathaux, izo, NULL);
							perror("eroare exec\n");
							exit(-99);

						}
						else
						{
							if (close(pipe_file_desciptor[1]) != 0)
							{
								perror("eraore pipe3\n");
								exit(-9);
							}
							//inchid capatul de scriere

							char verificare_res[1000];
							verificare_res[strlen(verificare_res) - 1] = '\0';

							if (read(pipe_file_desciptor[0], verificare_res, 1000) == -1)
							{
								perror("eroare pipe4\n");
								exit(-9);
							}
							if (close(pipe_file_desciptor[0]) != 0)
							{
								perror("eroare pipe5\n");
								exit(-9);
							}
							//inchid capatul folosit

							int status;
							pid_t pidWaitNepot = waitpid(pidNepot,&status,0);
							if (WIFEXITED(status))
							{
								int exit_status = WEXITSTATUS(status);
								printf("nepotul cu pid %d sa terminat cu status %d\n",pidNepot,exit_status);

								if (strcmp(verificare_res, "SAFE") == 0)
								{
									printf("nu este corupt\n");
								}
								else
								{
									printf("este corupt\n");
									(*countPericulos)++;
									mutare_fisier_corupt(pathaux, izo);
								}
							}
							else
							{
								perror("eroare iesire proces\n");
							}
						}
					}

				}
			else
				{
				if (strstr(entry->d_name, "_snapshot.txt"))
				{
					if (out == 1)
					{
						remove(pathaux);
					}
					continue;
				}

				char numefis[100];
				strcpy(numefis, entry->d_name);

				char* aux = strtok(numefis, ".txt");
				strcpy(numefis, aux);

				strcat(numefis, "_snapshot.txt");

				char new_path[1000];
				if (out == 1)
				{
					strcpy(new_path, path_out);
				}
				else
				{
					strcpy(new_path, path);
				}
				strcat(new_path, "/");
				strcat(new_path, numefis);

				int file = open(new_path, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
				if (file == -1)
				{
					printf("Eroare deschidere");
					exit(-3);
				}


				struct stat info2;
				if (lstat(new_path, &info2) == -1)
				{
					perror("eroare la lstat\n");
					exit(-12);
				}
				if (info2.st_size == 0)
				{
				     	write_in_Snapshot(file,file_info,entry); //creaza snapshot
				     	close(file);
				}	
				else
				{ //verificam schimbarile daca exista

					int file2=open("tmp.txt", O_CREAT|O_TRUNC  |O_RDWR, S_IWUSR|S_IRUSR);
					if(file2==-1)
					{
						printf("tmp nu sa deschis\n");
					}
					write_in_Snapshot(file2,file_info,entry);


					char c1, c2;
					ssize_t nr_bytes, nr_bytes2;

					lseek(file2, 0, SEEK_SET);
	

					while((nr_bytes=read(file,&c1, sizeof(char)))>0 &&(nr_bytes2=read(file2,&c2, sizeof(char)))>0)
					{
						
						if (c1=='\n' && c2=='\n')
						{
							break;
						}
						if(c1=='\n')
						{
							while((nr_bytes=read(file2,&c2, sizeof(char)))>0)
							{
								if(c2=='\n')
								{
									break;
								}
							}
							break;
						}
						else
						{
							if(c2=='\n')
							{
								while((nr_bytes=read(file,&c1, sizeof(char)))>0)
								{
									if(c1=='\n')
									{
										break;
									}
								}
								break;
								}
						}
					}
					
					while((nr_bytes=read(file,&c1, sizeof(char)))>0 && (nr_bytes2=read(file2,&c2, sizeof(char)))>0)
					{
	
						if(c1!=c2)
						{
							lseek(file, 0, SEEK_SET);
							file=open(new_path, O_CREAT|O_TRUNC  |O_RDWR, S_IWUSR| S_IRUSR);
							write_in_Snapshot(file,file_info,entry);
							
							break;
						}
					}
					close(file2);
					remove("tmp.txt");
				}
		
			}		
		}
	}

}
}



int main(int argc, char** argv)
{
	if (argc<4|| argc>15)
	{
		exit(-4);
	}

	pid_t p[10];
	int countProcese=0;
	int c = 0;

	int fis_out=0;
	char path_out[100]="";

	char numeDir[10][1000];

	if(strcmp(argv[1],"-o")==0)
	{
		struct stat info;
		if (lstat(argv[2],&info) == -1)
		{
			perror("eroare la lstat\n");
			exit(-12);
		}
		if(S_ISDIR(info.st_mode))
		{
			fis_out=1;
			strcpy(path_out,argv[2]);
		}
		else
		{
			printf("Nu sunt in directoru corect de iesire\n");
			return 0;
		}
	}
	else
	{	
		fis_out=0;
		printf("Nu am director de iesire\n");
	}	

	if (strcmp(argv[3],"-s") != 0)
	{
		printf("nu am director de izolat\n");
		exit(-15);
	}
	else
	{
		struct stat info;
		if (lstat(argv[4], &info) == -1)
		{
			perror("eroare la lstat\n");
			exit(-12);
		}
		if (!S_ISDIR(info.st_mode))
		{
			printf("director de izolat gresit\n");
			exit(-15);
		}

	}

	for(int i=5;i<argc;i++)
	{
		struct stat file_info;
		if (lstat(argv[i], &file_info) == -1)
		{
			perror("eroare lstat\n");
			exit(-12);
		}
		if(S_ISDIR(file_info.st_mode))
		{
			strcpy(numeDir[countProcese], argv[i]);

			p[countProcese]=fork();
			if(p[countProcese]<0)
			{
				printf("Pid incorect\n");
				exit(-5);
			}
			else
			{
				if(p[countProcese]==0)
				{
					DIR *dir=openDirectory(argv[i]);
	                           
					if(fis_out==0)
					{
						strcpy(path_out,argv[i]);
					}
					char path[1000]="";
					strcpy(path, argv[i]);

					int countPericol = 0;
					checkDir(dir, path, path_out, 1, fis_out, argv[4], &countPericol, "verify_malicious.sh");

					printf("in directorul %s s-au gasit %d fisiere corupte\n", argv[i], countPericol);

					exit(c);
				}
				else
				{

				}	
			}
			countProcese++;
		}
	
	}
	for(int i=0;i<countProcese;i++)
	{
		int status;
		pid_t pid=waitpid(0,&status,0);

		if(WIFEXITED(status))
		{
			int exit_status=WEXITSTATUS(status);
			printf("Statusul iesire proces %d cu pid %d(nume director verificat:%s) este:%d\n", i, pid, numeDir[i], exit_status);
		}
		else
		{
			printf("procesul %d cu pid %d sa incheiat gresit :(\n", i, pid);
		}
	}

	return 0;
}