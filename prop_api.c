#include <stdio.h>
#include "prop_tree.h"

typedef struct _Prop_system
{
	unsigned char initialized_;
	tPropArea *pa;
}tPropSystem;

static tPropSystem g_prop_sys;

int __attribute__((constructor)) system_prop_init(){
	memset(&g_prop_sys, 0 ,sizeof(g_prop_sys));
	g_prop_sys.pa = (tPropArea *)x_prop_init(DEFAULT_PROP_PATH);
	if(! g_prop_sys.pa) return -1;
	g_prop_sys.initialized_ = 1;
	return 0;
}

void * system_prop_get(const char * name, char * val, unsigned int val_len){
	if(!g_prop_sys.initialized_) return NULL;
	return x_prop_get(g_prop_sys.pa, name, val, val_len);
}
/*
int system_prop_set(const char * name, char * val){
	if(!g_prop_sys.initialized_) return -1;
	return x_prop_set(g_prop_sys.pa, name, val);
}*/

int system_prop_wait(void * pi_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/){
	if(!g_prop_sys.initialized_) return -1;
	return x_prop_wait(pi_,old_serial,new_serial_p,timeout);
}

int system_prop_get_pi_serial(void * pi_){
	if(!g_prop_sys.initialized_) return 0;
	return x_prop_get_pi_serial(pi_);
}

int system_prop_get_pa_serial(){
	if(!g_prop_sys.initialized_) return 0;
	return x_prop_get_pa_serial(g_prop_sys.pa);
}

void __attribute__((destructor)) system_prop_destory(){
	if(!g_prop_sys.initialized_) return;
	x_prop_uninit((void *)&(g_prop_sys.pa));
}
