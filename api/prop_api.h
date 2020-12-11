#ifndef __PROP_API_H
#define __PROP_API_H
//#include "../sys/system_properties.h"
/*system prop API*/
//int system_prop_init();
#ifdef __cplusplus 
extern "C" { 
#endif


typedef struct prop_info prop_info;
int system_prop_get(const char * name, char * val, unsigned int val_len);
//int system_prop_set(const char * name, char * val);
int system_prop_set (const char* key, const char* value);
int system_prop_foreach(void (*propCb)(const void *pi, void * priv), void * priv);
int system_prop_wait(void * pi_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/);
int system_prop_get_pi_info(const void * pi_, char * name, int name_len, char * val, int val_len);


const prop_info* system_prop_find(const char* name);
extern void system_prop_read_callback(const void *pi,
    void (*callback)(void* cookie, const char *name, const char *value, unsigned int serial),
    void* cookie) ;

/* Deprecated. Use __system_property_read_callback instead. */
int system_prop_read(const void* pi, char* name,size_t nlen, char* value,size_t vlen);
/* Deprecated. Use __system_property_read_callback instead. */
const prop_info* system_prop_find_nth(unsigned n) ;
unsigned int  system_prop_get_pi_serial(const void * pi_);

#ifdef __cplusplus  
}  
#endif 

#endif
