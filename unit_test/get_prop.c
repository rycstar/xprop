#include <stdio.h>
#include <string.h>
#include "prop_api.h"

int main(int argc, char * argv[]){

	char val[32] = {0};
	system_prop_get(argv[1],val,sizeof(val));
	printf("get prop(%s):%s\n",argv[1],val);

	return 0;
}