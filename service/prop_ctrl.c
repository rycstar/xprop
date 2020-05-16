#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "prop_ctrl_api.h"

typedef struct prop_ctrl
{
	int s;
    int initialized_;
	struct sockaddr_un local;
	struct sockaddr_un dest;
}tPropCtrl;

static tPropCtrl g_prop_ctrl_;

#define PROP_CTRL_IFACE_CLIENT_DIR "/tmp"
#define PROP_CTRL_IFACE_CLIENT_PREFIX "prop_ctrl_"
void* x_prop_ctrl_open(const char *srv_path){
	int ret = 0,flags = 0;
	static int count = 0;/*open count in one process*/
    tPropCtrl * ctrl = &g_prop_ctrl_;
	memset(ctrl,0,sizeof(tPropCtrl));

	ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (ctrl->s < 0) {
            return NULL;
    }
    ctrl->local.sun_family = AF_UNIX;

    ret = snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
                          PROP_CTRL_IFACE_CLIENT_DIR "/"
                          PROP_CTRL_IFACE_CLIENT_PREFIX "%d-%d",
                          (int) getpid(), count++);
    if (ret < 0 || (size_t) ret >= sizeof(ctrl->local.sun_path)) {
            goto sock_fail;
    }


    if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
                    sizeof(ctrl->local)) < 0) {
	    goto sock_fail;
    }


    ctrl->dest.sun_family = AF_UNIX;
    ret = snprintf(ctrl->dest.sun_path,sizeof(ctrl->dest.sun_path),"%s",srv_path);

    if (ret >= sizeof(ctrl->dest.sun_path)) {
            goto conn_fail;
    }
    if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
                sizeof(ctrl->dest)) < 0) {
            goto conn_fail;
    }

    /*
     * Make socket non-blocking so that we don't hang forever if
     * target dies unexpectedly.
     */
    flags = fcntl(ctrl->s, F_GETFL);
    if (flags >= 0) {
            flags |= O_NONBLOCK;
            if (fcntl(ctrl->s, F_SETFL, flags) < 0) {
                    goto conn_fail;
            }
    }
    ctrl->initialized_ = 1;
	return (void *)ctrl;

conn_fail:
	unlink(ctrl->local.sun_path);
sock_fail:
    close(ctrl->s);
    ctrl->s = 0;
    return NULL;
}

int x_prop_ctrl_request_async(void * ctl, const char * name, const char * val){
	tPropCtrl *ctrl = (tPropCtrl *)ctl;
	char cmd[128] = {0};
	if(!ctrl) return -1;

	snprintf(cmd,sizeof(cmd),"%s\r\n%s",name,val);
	if(send(ctrl->s,cmd, sizeof(cmd), 0) < 0){
		printf("prop send request(%s) error\n",cmd);
		return -1;
	}
	return 0;
}

int x_prop_ctrl_request_sync(void * ctl, const char * name, const char * val, char * reply, int reply_len, \
	void (*msg_cb)(char *msg, size_t len)){
    if( ! g_prop_ctrl_.initialized_ ) return -1;
	return 0;
}

int x_prop_ctrl_close(void * ctl){
	int ret = -1;
	tPropCtrl *ctrl = (tPropCtrl *)ctl;
	if(ctrl){
		unlink(ctrl->local.sun_path);
		if(ctrl->s) close(ctrl->s);
    	ret = 0;
	}
	return ret;
}

int __attribute__((constructor)) system_prop_ctrl_open(){
    return x_prop_ctrl_open(PROP_SRV_SOCKET_PATH) ? 0 : -1;
}

int system_prop_request_async(const char * name, const char * val){
    if( ! g_prop_ctrl_.initialized_ ) return -1;
    return x_prop_ctrl_request_async((void *)&g_prop_ctrl_, name, val);
}

int system_prop_request_sync(const char * name, const char * val,char * reply, int reply_len, \
    void (*msg_cb)(char *msg, size_t len)){
    if( ! g_prop_ctrl_.initialized_ ) return -1;
    return x_prop_ctrl_request_sync((void *)&g_prop_ctrl_, name, val, reply, reply_len,msg_cb);
}

int __attribute__((destructor))  system_prop_ctrl_close(){
    if( ! g_prop_ctrl_.initialized_ ) return -1;
    return x_prop_ctrl_close((void *)&g_prop_ctrl_);
}