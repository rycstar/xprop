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

#define PROP_SRV_SOCKET_PATH "/tmp/prop_srv"


static void event_run(tPropArea * pa, int *sock){
	int r = 0, res = 0,fromlen = 0;
	char buf[256];
	char * name = NULL, * val = NULL;
	char * split_point = NULL;
	struct pollfd fds;
	struct sockaddr_un from;
	memset(&fds, 0 , sizeof(fds));

	fds.fd = *sock;
	fds.events = POLLIN;

/*for test*/
	usleep(5000*1000);

	while(1){
		r = poll(&fds, 1, -1);
		if(r > 0){
			if (fds.revents & fds.events){
				if((res = recvfrom(*sock, buf, sizeof(buf) - 1, 0,(struct sockaddr *) &from, &fromlen)) > 0){
					buf[res] = '\0';
					printf("get buf (%s)\n",buf);
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

int main(int argc, char * argv[]){
	int c = 0, index = 0;
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
#else
	if(argc >= 2){
		def_sys_config = argv[1];
	}
#endif	

	pa = map_prop_area_rw("/tmp/__prop__");

	if(!pa){
		printf("Failed to mmap prop area\n");
        return -1;
	}

	/*load system default configs*/
	if(def_sys_config){

	}

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