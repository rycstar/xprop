#include <stdio.h>
#include <string.h>
#include "prop_api.h"
#include <assert.h> 

int main(int argc, char * argv[]){
	void * pa = NULL;
	void *pi = NULL;
	char val[32] = {0};
	unsigned int old_s = 0, new_s = 0 , test_times = 15;
	pa = x_prop_init("/tmp/__prop__");
	assert(pa);
	pi = x_prop_get(pa,argv[1],val,sizeof(val));
	assert(pi);

	old_s = x_prop_get_pi_serial(pi);

	do{
		printf("-----times:%d---olds:%08x--\n",test_times,old_s);
		if(0 == x_prop_wait(pi,old_s,&new_s,0)){
			x_prop_get(pa,argv[1],val,sizeof(val));
			printf("prop changed,get prop(%s):%s\n",argv[1],val);
			old_s = new_s;
		}
	}while(test_times-- > 0);
	
	return 0;
}