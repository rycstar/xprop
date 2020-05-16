#include <stdio.h>
#include <string.h>
#include "prop_api.h"

int main(int argc, char * argv[]){
	char val[128] = {0};
	if(system_prop_get(argv[1],val,sizeof(val))){
		printf("%s\n",val);
	}else if(argc == 3){
		printf("%s\n",argv[2]);
	}
	return 0;
}