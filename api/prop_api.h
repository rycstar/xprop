#ifndef __PROP_API_H
#define __PROP_API_H

#define DEFAULT_PROP_PATH "/tmp/__prop__"
#define PROP_SRV_SOCKET_PATH "/tmp/prop_srv"


/*********************For unix socket communication***********************/

/*
* return a point to 'ctl'
*/
void* x_prop_ctrl_open();
int x_prop_ctrl_request_async(void * ctl, const char * name, const char * val);
int x_prop_ctrl_request_sync(void * ctl, const char * name, const char * val, char * reply, int reply_len, \
	void (*msg_cb)(char *msg, size_t len));
int x_prop_ctrl_close(void * ctl);


/*system prop API*/
//int system_prop_init();
void * system_prop_get(const char * name, char * val, unsigned int val_len);
/*int system_prop_set(const char * name, char * val);*/
int system_prop_wait(void * pi_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/);
int system_prop_get_pi_serial(void * pi_);
int system_prop_get_pa_serial();
#endif