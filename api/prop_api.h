#ifndef __PROP_API_H
#define __PROP_API_H

#include "prop_common.h"

/*system prop API*/
//int system_prop_init();
void * system_prop_get(const char * name, char * val, unsigned int val_len);
/*int system_prop_set(const char * name, char * val);*/
int system_prop_wait(void * pi_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/);
int system_prop_get_pi_serial(void * pi_);
int system_prop_get_pa_serial();
#endif