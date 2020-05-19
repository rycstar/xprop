#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "prop_tree.h"
#include "prop_api.h"

static void event_run(tPropArea * pa, int *sock){
	int r = 0, res = 0;
	unsigned int fromlen = 0;
	char buf[256];
	char * name = NULL, * val = NULL;
	char * split_point = NULL;
	struct pollfd fds;
	struct sockaddr_un from;
	memset(&fds, 0 , sizeof(fds));

	fds.fd = *sock;
	fds.events = POLLIN;

	while(1){
		r = poll(&fds, 1, -1);
		if(r > 0){
			if (fds.revents & fds.events){
				if((res = recvfrom(*sock, buf, sizeof(buf) - 1, 0,(struct sockaddr *) &from, &fromlen)) > 0){
					buf[res] = '\0';
					split_point = strstr(buf,"\r\n");
					if(split_point){
						*split_point = '\0';
						name = buf;
						val = split_point + 2;
						
						if(! x_prop_get(pa,name,NULL,0)){
							x_prop_add(pa, name, val);
						}else{
							x_prop_set(pa, name, val);
						}
					}
				}
			}
		}	
	}
}

static char * remove_head_space(char * src){
	char *tmp = src;
	while(*tmp && *tmp == ' ') tmp++;
	return tmp;
}

static char * remove_tail_char(char * src, char c){
	char *tmp = src;
	int len = strlen(tmp);
	while(len > 0 && *(tmp + len - 1) == c) len--;
    *(tmp + len) = '\0';
	return tmp;
}

static char * remove_unused_space(char * src){
	char * tmp = remove_tail_char(src, ' ');
	return remove_head_space(tmp);
}

static void load_prop_file(tPropArea *pa, const char * filename){
	FILE *stream = NULL;
	char *line = NULL, *format_line = NULL;
	size_t len = 0;
	ssize_t nread = 0;
	
	char *p_name = NULL, * p_val =NULL; 
	
	stream = fopen(filename,"r");

	if(stream){
	    while((nread = getline(&line, &len, stream))!= -1 ){
	    	line[nread - 1] = '\0';/*the line include '\n' and not include '\0'*/
	        
			format_line = remove_tail_char(line,'\n');
			format_line = remove_tail_char(format_line, '\r');/*for windows file*/
			format_line = remove_head_space(format_line);

	        if(format_line[0] == '#') continue;
	        
	        p_val = strstr(format_line,"=");
            if(p_val){
                *p_val = 0;
                p_name = remove_unused_space(format_line);
               	p_val = remove_unused_space(++p_val);
               	if(strlen(p_name) > 0 && strlen(p_val) > 0) x_prop_add(pa,p_name, p_val);
	        }
	    }
	    if(line) free(line);
	    fclose(stream);
	}
}

int main(int argc, char * argv[]){
	//int c = 0, index = 0;
	int i = 0;
	char * def_sys_config = NULL;

	int sock = 0;
	struct sockaddr_un addr;
	tPropArea * pa = NULL;

	/*parse args*/
#if 0	/*add more options here if needed*/
	while ((c = getopt (argc, argv, "abc:")) != -1){
		switch (c){
			case a:
				break;
			case b:
				break;
			case c:
				break;
			default:
				abort();
		}
	}
	if(optind < argc) def_sys_config = argv[index];
#endif	

	pa = map_prop_area_rw(DEFAULT_PROP_PATH);

	if(!pa){
		printf("Failed to mmap prop area\n");
        return -1;
	}

	/*load system default configs*/
	for(i = 1; i < argc; i++){
		def_sys_config = argv[i];
		if(def_sys_config)	load_prop_file(pa,def_sys_config);
	}
	x_prop_add(pa, "ro.author", "terry.rong");

	/*start local socket service*/
	sock = socket(PF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if(sock < 0){
		printf("Failed tp create socket\n");
		return -1;
	}
	memset(&addr, 0 , sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             PROP_SRV_SOCKET_PATH);
	/*as this is the service and only start once, we first to unlink the fail.
	*/
	if ((unlink(addr.sun_path) != 0) && (errno != ENOENT)) {
        printf("Failed to unlink old socket\n");
        return -1;
    }

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0){
    	printf("Failed to bind socket\n");
    	return -1;
    }

    event_run(pa,&sock);

    return 0;
}
