#include <stdio.h>
#include <string.h>
#include "prop_api.h"
#include <assert.h> 

int main(int argc, char * argv[]){
	void * pa = NULL;
	char val[32] = {0};

	pa = x_prop_init("/tmp/__prop__");
	assert(pa);
	x_prop_get(pa,argv[1],val,sizeof(val));
	printf("get prop(%s):%s\n",argv[1],val);

	return 0;
}