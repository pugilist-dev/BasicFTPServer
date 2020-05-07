#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

static void validation_message(const char *on_what){
	perror(on_what);
	exit(1);
}

void usage(){
	printf("usage: tget <filename>\n");
	printf("usage: tput <filename>");
}
/*
 * filename server_ipaddress portno
	argv[0] filename
	argv[1] server_ip address
	argv[2] portno
	*/
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
	strcpy(cpy, command);
	strcpy(u_verify, command);
	char *str = strtok(cpy, " ");
	int value;
	if(strcmp(str,"tget")== 0 ){
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
	int sock, port_no, z;
	struct sockaddr_in server_address;
	struct hostent *server;
	char buffer[4096];


	// Setting up the connection
	if(argc <3){
		fprintf(stderr,"Usage %s hostname port\n", argv[0]);
		exit(1);
	}
	port_no = atoi(argv[2]);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	// Validation for socket declaration
	if(sock == -1){
		validation_message("socket()");
	}
	server = gethostbyname(argv[1]);
	if(server == NULL){
		fprintf(stderr, "There is no host by that name\n");
		exit(1);
	}
	bzero((char *) &server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(port_no);
	if(connect(sock, (struct sockaddr *)&server_address, sizeof(server_address))<0){
		validation_message("connect()");
	}
	bzero(buffer, 4096);

	FILE *f;
	int words = 0, command = 0;
	int count = 0;
	char c, ch;
	const char *filename;
	int user_authenticated = 0;
	// Routine to read the number of chaaracters in the file
	
	
	do{
		printf("Enter the username and password:\n");
		printf("Usage: tconnect <username> <password>\n");
		fgets(buffer, 4096, stdin);
		z = write(sock, buffer, 4096);
		if(z == -1){
 				validation_message("write()");
 		}
		bzero(buffer, 4096);
		z = read(sock, &user_authenticated, sizeof(user_authenticated));
		if(z == -1){
 			validation_message("read()");
 		}
 		printf("%s\n",buffer);
 		bzero(buffer, 4096);
	}while(user_authenticated != 1);

	while(user_authenticated == 1){
		printf(">>");
		fgets(buffer, 4096, stdin);
		z = write(sock, buffer, 4096);
		if(z == -1){
 			validation_message("read()");
 		}
		command = get_command(buffer);
		filename = get_filename(buffer);
		if(command == 2){
			z = read(sock, &words, sizeof(int));
			if(z == -1){
 				validation_message("read()");
 			}
			f = fopen(filename, "a");
			while(count != words){
				z = read(sock, &ch, 1);
				if(z == -1){
 						validation_message("read()");
 				}
				fputc(ch, f);
				count++;
			}
			printf(" The file has been received successfully\n");
			fclose(f);
		}else if(command == 3){
			f = fopen(filename, "r");
			if(f == NULL){ 
        		printf("Cannot open file %s \n", filename);
        	} 
			while((c = fgetc(f))!= EOF){
				words++;
			}
			z = write(sock, &words, sizeof(int));
			if(z == -1){
 						validation_message("write()");
 			}
			rewind(f);
			while((ch = fgetc(f))!=EOF){
				z = write(sock, &ch, 1);
				if(z == -1){
 					validation_message("write()");
 				}
			}
			printf("File has been sent successfully\n");
			fclose(f);
		}else{
			usage();
			bzero(buffer, 4096);
		}

	}
	close(sock);
	return 0;
}
