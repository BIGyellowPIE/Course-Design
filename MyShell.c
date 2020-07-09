#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<pwd.h>
#include<wait.h>
#include<sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>


//define the printf color
#define L_GREEN "\e[1;32m"
#define L_BLUE  "\e[1;34m"
#define L_RED   "\e[1;31m"
#define WHITE   "\e[0m"

#define TRUE 1
#define FALSE 0


char lastdir[100];
char command[BUFSIZ];
char argv[100][100];
char **argvtmp1;
char **argvtmp2;
char argv_redirect[100];
char argv_redirect1[100];
int  argc;
int BUILTIN_COMMAND = 0;
int PIPE_COMMAND = 0;
int REDIRECT_COMMAND = 0;
int REDIRECT_COMMAND1 = 0;
//set the prompt
void set_prompt(char *prompt);
//analysis the command that user input
int analysis_command();
void builtin_command();
void do_command();
//print help information
void help();
void initial();
void init_lastdir();
void history_setup();
void history_finish();
void display_history_list();

int main(){
	char prompt[BUFSIZ];
	char *line;
	
	init_lastdir();
	history_setup();	
	while(1) {
		set_prompt(prompt);
		if(!(line = readline(prompt))) 
			break;
		if(*line)
			add_history(line);

		strcpy(command, line);
		//strcat(command, "\n");
		if(!(analysis_command())){
			//todo deal with the buff
			if(BUILTIN_COMMAND){
				builtin_command();		
			}//if
			else{
				do_command();
			}//else		
		}//if analysis_command
		initial();//initial 
	}//while
	history_finish();
	
	return 0;
}

//set the prompt
void set_prompt(char *prompt){
	char hostname[100];
	char cwd[100];
	char super = '#';
	//to cut the cwd by "/"	
	char delims[] = "/";	
	struct passwd* pwp;
	
	if(gethostname(hostname,sizeof(hostname)) == -1){
		//get hostname failed
		strcpy(hostname,"unknown");
	}//if
	//getuid() get user id ,then getpwuid get the user information by user id 
	pwp = getpwuid(getuid());	
	if(!(getcwd(cwd,sizeof(cwd)))){
		//get cwd failed
		strcpy(cwd,"unknown");	
	}//if
	char cwdcopy[100];
	strcpy(cwdcopy,cwd);
	char *first = strtok(cwdcopy,delims);
	char *second = strtok(NULL,delims);
	//if at home 
	if(!(strcmp(first,"home")) && !(strcmp(second,pwp->pw_name))){
		int offset = strlen(first) + strlen(second)+2;
		char newcwd[100];
		char *p = cwd;
		char *q = newcwd;

		p += offset;
		while(*(q++) = *(p++));
		char tmp[100];
		strcpy(tmp,"~");
		strcat(tmp,newcwd);
		strcpy(cwd,tmp);			
	}	
	
	if(getuid() == 0)//if super
		super = '#';
	else
		super = '$';
	sprintf(prompt, "\001\e[1;31m\002[MyShell]\001\e[1;32m\002%s@%s\001\e[0m\002:\001\e[1;34m\002%s\001\e[0m\002%c",pwp->pw_name,hostname,cwd,super);	
	
}

//analysis command that user input
int analysis_command(){    
	int i = 1;
	char *p;
	//to cut the cwd by " "	
	char delims[] = " ";
	argc = 1;
	
	strcpy(argv[0],strtok(command,delims));
	while(p = strtok(NULL,delims)){
		strcpy(argv[i++],p);
		argc++;
	}//while
	
	if(!(strcmp(argv[0],"exit"))||!(strcmp(argv[0],"help"))|| !(strcmp(argv[0],"cd"))||!(strcmp(argv[0],"history"))){
		BUILTIN_COMMAND = 1;	
	}
	int j;
	//is a pipe command ?
	int pipe_location;
	for(j = 0;j < argc;j++){
		if(strcmp(argv[j],"|") == 0){
			PIPE_COMMAND = 1;
			pipe_location = j;				
			break;
		}	
	}//for
	
	//is a redirect command ?
	int redirect_location;
	for(j = 0;j < argc;j++){
		if(strcmp(argv[j],">") == 0){
			REDIRECT_COMMAND = 1;
			redirect_location = j;				
			break;
		}
	}//for
	
	int redirect_location1;
	for(j = 0;j < argc;j++){
		if(strcmp(argv[j],"<") == 0){
			REDIRECT_COMMAND1 = 1;
			redirect_location1 = j;				
			break;
		}
	}//for

	if(PIPE_COMMAND){
		//command 1
		argvtmp1 = malloc(sizeof(char *)*pipe_location + 1);
		int i;	
		for(i = 0;i < pipe_location + 1;i++){
			argvtmp1[i] = malloc(sizeof(char)*100);
			if(i <= pipe_location)
				strcpy(argvtmp1[i],argv[i]);	
		}//for
		argvtmp1[pipe_location] = NULL;
		
		//command 2
		argvtmp2 = malloc(sizeof(char *)*(argc - pipe_location));
		int j;	
		for(j = 0;j < argc - pipe_location;j++){
			argvtmp2[j] = malloc(sizeof(char)*100);
			if(j <= pipe_location)
				strcpy(argvtmp2[j],argv[pipe_location + 1 + j]);	
		}//for
		argvtmp2[argc - pipe_location - 1] = NULL;
		
	}//if pipe_command

	else if(REDIRECT_COMMAND){
		strcpy(argv_redirect,argv[redirect_location + 1]);
		argvtmp1 = malloc(sizeof(char *)*redirect_location + 1);
		int i;	
		for(i = 0;i < redirect_location + 1;i++){
			argvtmp1[i] = malloc(sizeof(char)*100);
			if(i < redirect_location)
				strcpy(argvtmp1[i],argv[i]);	
		}//for
		argvtmp1[redirect_location] = NULL;
	}//redirect command
	
	else if(REDIRECT_COMMAND1){
		strcpy(argv_redirect1,argv[redirect_location1 + 1]);
		argvtmp1 = malloc(sizeof(char *)*redirect_location1 + 1);
		int i;	
		for(i = 0;i < redirect_location1 + 1;i++){
			argvtmp1[i] = malloc(sizeof(char)*100);
			if(i < redirect_location1)
				strcpy(argvtmp1[i],argv[i]);	
		}//for
		argvtmp1[redirect_location1] = NULL;
	}//redirect command

	else{
		argvtmp1 = malloc(sizeof(char *)*argc+1);
		int i;	
		for(i = 0;i < argc + 1;i++){
			argvtmp1[i] = malloc(sizeof(char)*100);
			if(i < argc)
				strcpy(argvtmp1[i],argv[i]);	
		}//for
		argvtmp1[argc] = NULL;
	}

	return 0;
}

void builtin_command(){
	struct passwd* pwp;
	//exit when command is exit	
	if(strcmp(argv[0],"exit") == 0){
		exit(EXIT_SUCCESS);
	}
	else if(strcmp(argv[0],"help") == 0){
		help();
	}//else if
	else if(strcmp(argv[0],"history") == 0){
		display_history_list();
	}//else if	
	else if(strcmp(argv[0],"cd") == 0){
		char cd_path[100];
		if((strlen(argv[1])) == 0 ){
			pwp = getpwuid(getuid());
			sprintf(cd_path,"/home/%s",pwp->pw_name);
			strcpy(argv[1],cd_path);
			argc++;			
		}
		else if((strcmp(argv[1],"~") == 0) ){
			pwp = getpwuid(getuid());
			sprintf(cd_path,"/home/%s",pwp->pw_name);
			strcpy(argv[1],cd_path);			
		}
		
		if((chdir(argv[1]))< 0){
			printf("cd: %s: No such file or directory\n",argv[1]);
		}
	}//else if cd
}

void do_command(){
	//do_command
	
	if(PIPE_COMMAND){
		int fd[2],res;
		int status;
		
		res = pipe(fd);
	
		if(res == -1)
			printf("pipe failed !\n");
		pid_t pid1 = fork();
		if(pid1 == -1){
			printf("fork failed !\n");		
		}//if
		else if(pid1 == 0){
				dup2(fd[1],1);//dup the stdout
				close(fd[0]);//close the read edge
				if(execvp(argvtmp1[0],argvtmp1) < 0){
					printf("%s:command not found\n",argvtmp1[0]);		
				}//if			
		}//else if child pid1
		else{
			waitpid(pid1,&status,0);
			pid_t pid2 = fork();
			if(pid2 == -1){
				printf("fork failed !\n");		
			}//if
			else if(pid2 == 0){
				close(fd[1]);//close write edge
				dup2(fd[0],0);//dup the stdin
				if(execvp(argvtmp2[0],argvtmp2) < 0){
					printf("%s:command not found\n",argvtmp2[0]);		
				}//if
			}//else if pid2 == 0
			else{
				close(fd[0]);
				close(fd[1]);
				waitpid(pid2,&status,0);
			}//else 
		}//else parent process
	}//if pipe command

	else if(REDIRECT_COMMAND){
		pid_t pid = fork();	
		if(pid == -1){
			printf("fork failed !\n");		
		}//if
		else if(pid == 0){
			int redirect_flag = 0;
			FILE* fstream;
			fstream = fopen(argv_redirect,"w+");
			freopen(argv_redirect,"w",stdout);
			if(execvp(argvtmp1[0],argvtmp1) < 0){
				redirect_flag = 1;//execvp this redirect command failed		
			}//if
			fclose(stdout);
			fclose(fstream);
			if(redirect_flag){
				printf("%s:command not found\n",argvtmp1[0]);
			}//redirect flag	
				
		}//else if 
		else{
			int pidReturn = wait(NULL);	
		}//else  
	}//else if redirect command 

	else if(REDIRECT_COMMAND1){
		pid_t pid = fork();	
		if(pid == -1){
			printf("fork failed !\n");		
		}//if
		else if(pid == 0){
			int redirect_flag = 0;
			FILE* fstream;
			fstream = fopen(argv_redirect1,"r+");
			freopen(argv_redirect1,"r",stdin);
			if(execvp(argvtmp1[0],argvtmp1) < 0){
				redirect_flag = 1;//execvp this redirect command failed		
			}//if
			fclose(stdin);
			fclose(fstream);
			if(redirect_flag){
				printf("%s:command not found\n",argvtmp1[0]);
			}//redirect flag	
				
		}//else if 
		else{
			int pidReturn = wait(NULL);	
		}//else  
	}//else if redirect command 

	else{
		pid_t pid = fork();	
		if(pid == -1){
			printf("fork failed !\n");		
		}//if
		else if(pid == 0){
			if(execvp(argvtmp1[0],argvtmp1) < 0){
				printf("%s:command not found\n",argvtmp1[0]);			
			}//if	
		}//else if 
		else{
			int pidReturn = wait(NULL);	
		}//else  
	}//else normal command

        free(argvtmp1);
	free(argvtmp2);
}

void help(){
	struct passwd* pwp;
	pwp = getpwuid(getuid());
	printf("Hi, %s !\n",pwp->pw_name);
	printf("Here are the built-in commands:\n\n");
	printf("1. cd\n");
	printf("2. history\n");
	printf("3. help\n");
	printf("4. exit\n");
}

void initial(){
	int i = 0;	
	for(i = 0;i < argc;i++){
		strcpy(argv[i],"\0");	
	}
	argc = 0;
	BUILTIN_COMMAND = 0;
	PIPE_COMMAND = 0;
	REDIRECT_COMMAND = 0;
	REDIRECT_COMMAND1 = 0;
}

void init_lastdir(){
	getcwd(lastdir, sizeof(lastdir));
}

void history_setup(){
	using_history();
	stifle_history(50);
	read_history("/tmp/msh_history");	
}

void history_finish(){
	append_history(history_length, "/tmp/msh_history");
	history_truncate_file("/tmp/msh_history", history_max_entries);
}

void display_history_list(){
	HIST_ENTRY** h = history_list();
	if(h) {
		int i = 0;
		while(h[i]) {
			printf("%d: %s\n", i, h[i]->line);
			i++;
		}
	}
}

