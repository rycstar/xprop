#include <stdio.h>
#include <error.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "prop_api.h"

int main(int argc, char * argv[]){
	void * ctl = NULL;
	int i = 0;
	char value[32] = {0};
	ctl = x_prop_ctrl_open("/tmp/prop_srv");
	assert(ctl);

	do{
		snprintf(value,sizeof(value),"test_%d",100-i);
		x_prop_ctrl_request_async(ctl,"sys.abc.yyy",value);
		usleep(100*1000);
	}while(i++ < 100);
	x_prop_ctrl_close(ctl);
	return 0;
}