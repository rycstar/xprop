#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h> 
#include "prop_tree.h"
#include "prop_api.h"

int main(int argc, char * argv[]){
	char name[32] = {0},val[32] = {0};
	int fd = 0, test_times = 100;

#if 1	
	tPropArea * pa = map_prop_area_rw("/tmp/__prop__");
	assert(pa);
	strcpy(name,"sys.abc.xxx");
	strcpy(val,"truexxx");
	assert( 0 == x_prop_add(pa, name, val));

	strcpy(name,"sys.abc.yyy");
	strcpy(val,"trueyyy1");
	assert ( 0 == x_prop_add(pa,name,val));

	strcpy(name,"sys.abc.zzz");
	strcpy(val,"truezzz");
	assert ( 0 == x_prop_add(pa,name,val));



#endif	
#if 0
	fd = open(argv[1],O_RDWR|O_CREAT);
	if(fd > 0){
		printf("write size:%04x\n",pa->pa_size);
		write(fd,pa,pa->pa_size);
		close(fd);
	}
#endif	
	while(test_times -- > 0){
		memset(val,0,sizeof(val));
		if(test_times % 3 == 0){
			strcpy(name,"sys.abc.xxx");
		}else if (test_times % 3 == 1){
			strcpy(name,"sys.abc.yyy");
		}else{
			strcpy(name,"sys.abc.zzz");
		}
		x_prop_get(pa,name,val,sizeof(val));
		
		val[0] = 30 + rand()%10;
		x_prop_set(pa,name,val);
		printf("set prop name(%s),val:(%s)\n",name,val);
		usleep(1000*1000);
	}
	return 0;
}