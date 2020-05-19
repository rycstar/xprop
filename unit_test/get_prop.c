#include <stdio.h>
#include <string.h>
#include "prop_api.h"

static void prop_show_cb(const void * pi, void * unused){
	char name[128] = {0};
	char val[128] = {0};
	system_prop_get_pi_info(pi, name, sizeof(name), val,sizeof(val));
	printf("%s = %s\n", name, val);
}

int main(int argc, char * argv[]){
	char name[128] = {0};
	char val[128] = {0};
	if(argc == 1){
		/*list all the prop*/
		printf("-----------list all props------------\n");
		system_prop_foreach(prop_show_cb, NULL);
	}else if(system_prop_get(argv[1],val,sizeof(val))){
		printf("%s\n",val);
	}else if(argc == 3){
		printf("%s\n",argv[2]);
	}
	return 0;
}