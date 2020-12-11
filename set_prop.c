#include <stdio.h>
#include <error.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include "prop_ctrl_api.h"
#include "api/prop_api.h"

static int check_name(const char * name){
	int ret = 0;
	if(name && strchr(name,'.')) ret = 1;
	return ret;
}

static int check_val(const char * value){
	int ret = 0;
	if(value && strlen(value) > 0 && strlen(value) < 92) ret = 1;
	return ret;
}

int main(int argc, char * argv[]){
    int ret = 0;
	if(argc != 3){
		printf("Usage: prop_set [name] [value]\n");
		return -1;
	}

	if(!check_name(argv[1])) return -1;
	if(!check_val(argv[2])) return -1;
    ret = system_prop_set(argv[1], argv[2]);
    if(ret){
        printf("%s=%s set error\n",argv[1], argv[2]);
    }
	//system_prop_request_async(argv[1],argv[2]);

	return 0;
}
