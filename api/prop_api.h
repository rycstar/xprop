#ifndef __PROP_API_H
#define __PROP_API_H



/*
* return a point to prop area.
*/
void * x_prop_init(const char * path);

/*
* pa_ is the value return from x_prop_init();
*/

/*
* return the point of the prop, if you want to monitor the prop, use this val to call x_prop_wait.
*/
unsigned int x_prop_get(void * pa_, const char * name, char * val, unsigned int val_len);
int x_prop_set(void * pa_, const char * name, char * val);
int x_prop_add(void * pa_, const char * name, char * val);

int x_prop_wait_any(void * pa_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/);
int x_prop_wait(void * pi_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/);

unsigned int x_prop_get_pi_serial(void * pi_);
unsigned int x_prop_get_pa_serial(void * pa_);
#endif