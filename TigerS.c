#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>

static void validation_message(const char *on_what){
	perror(on_what);
	exit(1);
}
static void sigchld_handler(int signo){
	pid_t PID;
	int status;

	do{
		PID = waitpid(-1, &status, WNOHANG);
	} while(PID != -1);
	// re-instate handler
	signal(SIGCHLD, sigchld_handler);
}

void trim(char *str){
	int i;
    int begin = 0;
    int end = strlen(str) - 1;
    while (isspace((unsigned char) str[begin]))
        begin++;
    while ((end >= begin) && isspace((unsigned char) str[end]))
        end--;
    for (i = begin; i <= end; i++)
        str[i - begin] = str[i];
    str[i - begin] = '\0';
}


int authenticate(char *str) {
	FILE *fp;
	int finds = 0;
	char temp[512];
	if((fp = fopen("authentication.txt", "r")) == NULL) {
		return(-1);
	}
	while(fgets(temp, 512, fp) != NULL) {
			if((strstr(temp, str)) != NULL) {
						finds++;
			}
	}
	if(fp) {
		fclose(fp);
	}
   	return finds;
}

int get_username( char *command){
	char *token; 
    char *rest = command;
    int result = 0;
    for(int i = 0; i<2; i++){
    	token = strtok_r(rest, " ", &rest);
    } 
    result = authenticate(token);
    return result;
}
int get_password(char *command){
    int result = 0;
    char *p = strrchr(command, ' ');
	if (p && *(p + 1)){
		result = authenticate(p+1);
		return result;			
	}else{
		return 0;
	}
    
}

const char *get_filename(char *buffer){
	char *name;
	char *token; 
    char *rest = buffer;
    while ((token = strtok_r(rest, " ", &rest))){
        if((strcmp(token,"tget")||strcmp(token,"tput")||strcmp(token,"tconnect"))!=0){
        	name = token;
        }
    }
    trim(name);
    return name;

}
int get_command( char *command){
	char cpy[1024];
	char u_verify[1024];
	char p_verify[1024];
	strcpy(cpy, command);
	strcpy(u_verify, command);
	strcpy(p_verify, command);
	char *str = strtok(cpy, " ");
	int value;
	int username, password;
	if(strcmp(str, "tconnect")== 0 ){
		username = get_username(u_verify);
		password = get_password(p_verify);
		if((username >= 1)&&(password >= 1)){
			value = 1;
		} 
	}
	else if(strcmp(str,"tget")== 0 ){
		value = 2; 
	}
	else if(strcmp(str,"tput")== 0 ){
		value = 3;
	}
	else if(strcmp(str,"exit")== 0){
		value = 4;
	}
	else{
		value = 0;
	}
	return value;
}


int main(int argc, char **argv){
	if(argc<2){
		fprintf(stderr, "Please provide the port number\n");
		exit(0);
	}
	int sock,new_sock, port_no, z, user_authenticated = 0;
	char buffer[4096];
	struct sockaddr_in server_address, client_address;
	socklen_t client_length;
	pid_t PID;

	signal(SIGCHLD, sigchld_handler);

	sock = socket(AF_INET, SOCK_STREAM,0);
	if(sock == -1){
		validation_message("socket()");
	}
	bzero((char *) &server_address, sizeof(server_address));
	port_no = atoi(argv[1]);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port_no);
	if(bind(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1){
		validation_message("bind()");
	}
	
	listen(sock, 100);
	for(;;){
		client_length = sizeof(client_address);
		new_sock = accept(sock, (struct sockaddr *)&client_address, &client_length);
		if(new_sock == -1){
 			validation_message("accept(2)");
 		}
 		//forking a new server process to service this client
 		if((PID = fork()) < 0){
 			// failed to fork
 			close(new_sock);
 			continue;
 		} 
 		else if( PID > 0){
 			// Parent process
 			close(new_sock);
 			continue;
 		}
 		// Create a stream for the child process
 		// Receive routine for server
 		while(1){
 			FILE *fp;
			int words = 0, i =0, j = 0, count = 0, command = 0;
			int buf_len = 0;
			char c, ch, token[5];
			const char *filename;
 			bzero(buffer, 4096);
 			z = read(new_sock, buffer, 4096);
 			if(z == -1){
 				validation_message("read()");
 			} 
 			command = get_command(buffer);
 			bzero(buffer, 4096);
 			if(command==1 || user_authenticated==1){
 				user_authenticated = 1;
 				z = write(new_sock, &user_authenticated, sizeof(user_authenticated));
 				if(z == -1){
 					validation_message("write()");
 				}
 				z = read(new_sock, buffer, 4096);
 				if(z == -1){
 					validation_message("read()");
 				}
 				command = get_command(buffer);
 				filename = get_filename(buffer);

 				if(command == 2){
 					fp = fopen(filename, "r");
					while((c = fgetc(fp))!= EOF){
						words++;
					}
					z = write(new_sock, &words, sizeof(int));
					if(z == -1){
 						validation_message("write()");
 					}
					rewind(fp);
					while((ch = fgetc(fp))!=EOF){
						z = write(new_sock, &ch, 1);
						if(z == -1){
 							validation_message("write()");
 						}
					}
					printf("File has been sent successfully\n");
					fclose(fp);
 				}
 				else if(command == 3){
 					z = read(new_sock, &words, sizeof(int));
					if(z == -1){
 						validation_message("read()");
 					}
					fp = fopen(filename, "a");
					while(count != words){
						z = read(new_sock, &ch, 1);
						if(z == -1){
 							validation_message("read()");
 						}
						fputc(ch, fp);
						count++;
					}
					printf("The file has been received successfully\n");
					fclose(fp);
 				}
 			}
 			else{
 				bzero(buffer, 4096);
 				z = write(new_sock, &user_authenticated, sizeof(user_authenticated));
 				if(z == -1){
 						validation_message("write()");
 				}
 			}
			
		}
	}
	close(sock);
	close(new_sock);
	return 0;
}	
