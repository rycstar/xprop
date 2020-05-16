#include <stdio.h>
#include <error.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "prop_ctrl_api.h"

int main(int argc, char * argv[]){
	int i = 0;
	char value[32] = {0};

	do{
		snprintf(value,sizeof(value),"test_%d",100-i);
		system_prop_request_async("sys.abc.yyy",value);
		usleep(100*1000);
	}while(i++ < 100);
	return 0;
}