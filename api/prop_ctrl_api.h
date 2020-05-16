#ifndef __PROP_CTRL_API_H
#define __PROP_CTRL_API_H


#include "prop_common.h"
/*********************For unix socket communication***********************/

/*
* return a point to 'ctl'
*/

int system_prop_request_async(const char * name, const char * val);
int system_prop_request_sync(const char * name, const char * val,char * reply, int reply_len, \
	void (*msg_cb)(char *msg, size_t len));

#endif