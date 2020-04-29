#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <time.h>
#include "prop_tree.h"
#include "prop_api.h"

#define __BUF_ALIGN(__value, __alignment) (((__value) + (__alignment)-1) & ~((__alignment)-1))
#define PA_ROOT_NODE(pa) ((tPropBt *)(pa->data))
#define PA_SIZE  (128 * 1024)
#define PROP_AREA_MAGIC (0x50726f70)
#define PROP_AREA_VERSION (0x312e3030)

#define PI_SERIAL_DIRTY(serial) ((serial)&1)

static inline int sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
{
	int saved_errno = errno;
	int res = syscall(__NR_futex, addr1, op, val1, timeout, addr2, val3);
	if(res == -1){
		res = -errno;
		errno = saved_errno;
	}
	return res;
}

tPropArea * map_prop_area_rw(const char * path){
	tPropArea * pa = NULL;
	void * memory_area = 0;
	const int fd = open(path, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_EXCL, 0444);

	if(fd > 0){
		if(ftruncate(fd, PA_SIZE) == 0){
			memory_area = (tPropArea *)mmap(NULL, PA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if(memory_area != MAP_FAILED){
				pa = (tPropArea *)memory_area;
				memset(pa,0,PA_SIZE);
				pa->pa_size = PA_SIZE;
				pa->pa_data_size = pa->pa_size - sizeof(tPropArea);
				pa->bytes_used = sizeof(tPropBt);/*the root node area*/

				pa->magic = PROP_AREA_MAGIC;
				pa->version = PROP_AREA_VERSION;

				/*init serial for prop monitor and memory order*/
				atomic_init(&pa->serial, 0u);
			}
		}
		close(fd);
	}
	return pa;
}

tPropArea * map_prop_area_ro(const char * path){
	tPropArea * pa = NULL;
	struct stat fd_stat;
	void * memory_area = 0;
	int fd = open(path, O_CLOEXEC | O_NOFOLLOW | O_RDONLY);

	if(fd > 0){
		if(fstat(fd, &fd_stat) == 0 && fd_stat.st_size == PA_SIZE){
			if( (memory_area = mmap(NULL, PA_SIZE, PROT_READ, MAP_SHARED, fd, 0)) != MAP_FAILED){
				pa = (tPropArea *)memory_area;
				if ((pa->magic != PROP_AREA_MAGIC) || (pa->version != PROP_AREA_VERSION)) {
				    munmap(pa, PA_SIZE);
				    pa = NULL;
				}
			}
		}
		close(fd);
	}
	return pa;
}

#if 0
tPropArea * map_prop_area_rw_test(const char * path){

	tPropArea * area = (tPropArea *)malloc(PA_SIZE);
	if(area){
		memset(area,0,PA_SIZE);
		area->pa_size = PA_SIZE;
		area->pa_data_size = area->pa_size - sizeof(tPropArea);
		area->bytes_used = sizeof(tPropBt);/*the root node area*/

		area->magic = PROP_AREA_MAGIC;
		area->version = PROP_AREA_VERSION;
		atomic_init(&area->serial, 0u);
	}
	return area;
}
#endif
static int cmp_str(const char* one, uint32_t one_len, const char* two, uint32_t two_len) {
  if (one_len < two_len)
    return -1;
  else if (one_len > two_len)
    return 1;
  else
    return strncmp(one, two, one_len);
}

static void * p_alloc_buf(tPropArea * pa, uint32_t size, uint32_t * offset){
	const uint32_t aligned = __BUF_ALIGN(size, sizeof(uint32_t));

	if(pa->bytes_used + aligned > pa->pa_data_size){
		return NULL;
	}
	*offset = pa->bytes_used;
	pa->bytes_used += aligned;
	return (pa->data + *offset);
}

static tPropBt* new_prop_bt(tPropArea * pa, const char* name, uint32_t namelen, uint32_t* const off) {
  tPropBt* bt = NULL;
  bt = (tPropBt*) p_alloc_buf(pa,sizeof(tPropBt) + namelen + 1, off);
  if (bt) {
  	bt->name_len = namelen;
  	memcpy(bt->name,name,namelen);
  	bt->name[namelen] = '\0';
  } 
  return bt;
}

static tPropInfo* new_prop_info(tPropArea * pa, const char* name, uint32_t namelen, const char* value,
                                    uint32_t valuelen, uint32_t* const off) {
	tPropInfo *info = NULL;

	if(valuelen >= PROP_VALUE_MAX) return NULL;
	info = (tPropInfo*)p_alloc_buf(pa,sizeof(tPropInfo) + namelen + 1, off);

	if(info){
		memcpy(info->name, name, namelen);
		info->name[namelen] = '\0';

		/*init serial for prop monitor and memory order*/
		atomic_init(&info->serial, valuelen << 24);

		memcpy(info->value, value, valuelen);
		info->value[valuelen] = '\0';
	}
	return info;
}

static void* to_prop_obj(tPropArea * pa, uint32_t off) {
  if (off > pa->pa_data_size) return NULL;
  return (pa->data + off);
}


static inline tPropBt* to_prop_bt(tPropArea * pa,atomic_uint* off_p) {
  uint32_t off = atomic_load_explicit(off_p, memory_order_consume);
  return (tPropBt*)(to_prop_obj(pa,off));
}

static inline tPropInfo* to_prop_info(tPropArea * pa,atomic_uint* off_p) {
  uint32_t off = atomic_load_explicit(off_p, memory_order_consume);
  return (tPropInfo*)(to_prop_obj(pa, off));
}

static tPropBt * _find_prop_bt(tPropArea * pa, tPropBt * const trie, const char * name, uint32_t len, uint8_t allow_alloc){
	tPropBt * current = trie, * new_bt = NULL;
	int cmp_res = 0;
	uint32_t child_offset = 0, new_offset = 0;/*trie PTR*/
	while(1){
		if(!current) return NULL;
		cmp_res = cmp_str(name,len,current->name,current->name_len);
		if(0 == cmp_res)	return current;

		if(cmp_res < 0){
			child_offset = atomic_load_explicit(&current->left, memory_order_relaxed);
			if(child_offset){
				current = to_prop_bt(pa, &current->left);
			}else{
				if(!allow_alloc) return NULL;

				new_bt = new_prop_bt(pa,name,len, &new_offset);
				if (new_bt) {
		          atomic_store_explicit(&current->left, new_offset, memory_order_release);
		        }
		        return new_bt;
			}
		}else{
			child_offset = atomic_load_explicit(&current->right, memory_order_relaxed);
			if(child_offset){
				current = to_prop_bt(pa, &current->right);
			}else{
				if(!allow_alloc) return NULL;

				new_bt = new_prop_bt(pa,name,len, &new_offset);
				if (new_bt) {
		          atomic_store_explicit(&current->right, new_offset, memory_order_release);
		        }
		        return new_bt;
			}
		}
	}
}

static tPropInfo * _find_prop(tPropArea * pa, tPropBt * const trie, const char * name, uint32_t namelen, \
	const char * val, uint32_t val_len,uint8_t allow_alloc){
	char * remain_name = (char*)name, * sep = (char*)name;
	uint32_t substr_size = 0, children_offset = 0 ,new_offset = 0, prop_offset = 0;
	tPropBt * root = NULL, * current = trie;
	tPropInfo * new_info = NULL;

	if (!trie || !pa) return NULL;

	while(1){
		sep = strchr(remain_name,'.');
		
		substr_size = sep ? (sep - remain_name) : strlen(remain_name);

		if(substr_size <= 0) return NULL;

		children_offset = atomic_load_explicit(&current->children, memory_order_relaxed);
	    if (children_offset != 0) {
	      root = to_prop_bt(pa,&current->children);
	    } else if (allow_alloc) {
	      root = new_prop_bt(pa,remain_name, substr_size, &new_offset);
	      if (root) {
	        atomic_store_explicit(&current->children, new_offset, memory_order_release);
	      }
	    }

	    if(!root) return NULL;

		current = _find_prop_bt(pa, root, remain_name, substr_size, allow_alloc);
		if(! current) return NULL;
		if(! sep) break;
		remain_name = sep + 1;/*skip char '.'*/
	}

	prop_offset = atomic_load_explicit(&current->prop, memory_order_relaxed);
	if (prop_offset != 0) {
		return to_prop_info(pa,&current->prop);
	} else if (allow_alloc) {
		new_info = new_prop_info(pa,name, namelen, val, val_len, &new_offset);
		if (new_info) {
	  		atomic_store_explicit(&current->prop, new_offset, memory_order_release);
		}
		return new_info;
	} else {
		return NULL;
	}
	
}

tPropInfo * find_prop(tPropArea * const pa, const char * name, uint32_t name_len, \
	const char * val, uint32_t val_len,uint8_t allow_alloc){

	return _find_prop(pa,PA_ROOT_NODE(pa),name,name_len,val,val_len,allow_alloc);
}

/*****************************API functions********************************/

static int is_read_only(const char *name){
	return strncmp(name, "ro.", 3) == 0;
}

/*
* return a point to prop area.
*/
void * x_prop_init(const char * path){
	return (void *)map_prop_area_ro(path); 
}

int x_prop_uninit(void ** pa_){
	if(*pa_){
		munmap(*pa_,PA_SIZE);
		*pa_ = NULL;
	}
	return 0;
}

/*
* pa is the value return from x_prop_init();
*/
// Wait for non-locked serial, and retrieve it with acquire semantics.
static uint32_t pi_serial(tPropInfo* pi) {
  uint32_t serial = atomic_load_explicit(&pi->serial, memory_order_acquire);
  while (PI_SERIAL_DIRTY(serial)) {
    sys_futex(&pi->serial, FUTEX_WAIT,serial,NULL,NULL,0);
    serial = atomic_load_explicit(&pi->serial, memory_order_acquire);
  }
  return serial;
}

void* x_prop_get(void * pa_, const char * name, char * val, unsigned int val_len){
	tPropArea * pa = (tPropArea *)pa_;

	if(!pa || !name) return NULL;

	tPropInfo * pi = find_prop(pa,name,strlen(name),NULL,0,0);
	if(pi){
		pi_serial(pi);/*for data rw sync!!!*/
		if(val)	snprintf(val,val_len,"%s",pi->value);
		return pi;
	}
	return NULL;
}
int x_prop_set(void * pa_, const char * name, char * val){
	tPropArea * pa = (tPropArea *)pa_;
	tPropInfo * pi;
	uint32_t serial, val_len;

	if(!pa || !name || !val) return -1;
	val_len = strlen(val);
	if (val_len >= PROP_VALUE_MAX || is_read_only(name)) {
	    return -1;
	}
  	pi = find_prop(pa,name,strlen(name),NULL,0,0);
  	if(!pi){
  		return -1;
  	}
  	serial = atomic_load_explicit(&pi->serial, memory_order_relaxed);
  	serial |= 1;
  	atomic_store_explicit(&pi->serial, serial, memory_order_relaxed);
  	atomic_thread_fence(memory_order_release);
  	strcpy(pi->value, val);

  	/*to wake the monitor threads*/
  	atomic_store_explicit(&pi->serial, (val_len << 24) | ((serial + 1) & 0xffffff), memory_order_release);
    sys_futex(&pi->serial, FUTEX_WAKE,INT_MAX,NULL,NULL,0);

    atomic_store_explicit(&pa->serial,
                        atomic_load_explicit(&pa->serial, memory_order_relaxed) + 1,
                        memory_order_release);
  	sys_futex(&pa->serial, FUTEX_WAKE,INT_MAX,NULL,NULL,0);
    return 0;
}


int x_prop_add(void * pa_, const char * name, char * val){
	tPropArea * pa = (tPropArea *)pa_;
	tPropInfo * pi;

	if(!pa || !name || !val) return -1;

	if (strlen(val) >= PROP_VALUE_MAX) {
	    return -1;
	}
	pi = find_prop(pa,name,strlen(name),val,strlen(val),1);
	if(!pi) {
		return -1;
	}
	atomic_store_explicit(&pa->serial,
                        atomic_load_explicit(&pa->serial, memory_order_relaxed) + 1,
                        memory_order_release);
  	sys_futex(&pa->serial, FUTEX_WAKE,INT_MAX,NULL,NULL,0);
  	return 0;
}

/*******************************monitor functions*****************************/
static struct timespec * timeoutCalc(int timeout, struct timespec * timespec_){
	if(timeout > 0 && timespec_){
		/*clock_gettime(CLOCK_MONOTONIC, timespec_); 
		timespec_->tv_sec += timeout / 1000 ; 
		timespec_->tv_nsec += (1000 * 1000) * (timeout % 1000);*/
		timespec_->tv_sec = timeout / 1000; 
		timespec_->tv_nsec = (1000 * 1000) * (timeout % 1000);
		return timespec_;
	}
	return NULL;
}

static int _wait(atomic_uint * serial_, uint32_t old_serial, uint32_t *new_serial_p,int timeout){
	int rc = 0;
	uint32_t tmp_serial = 0;
	struct timespec timeout_;
	struct timespec * p_timeout = NULL;

	if(!serial_){
		return -1;
	}

	p_timeout = timeoutCalc(timeout,&timeout_);
	
	do{
		if((rc = sys_futex(serial_, FUTEX_WAIT,old_serial,p_timeout,NULL,0)) != 0 && rc != -ETIMEDOUT){
			return -1;
		}
		tmp_serial = atomic_load_explicit(serial_,memory_order_acquire);
	}while(tmp_serial == old_serial);
	*new_serial_p = tmp_serial;
	return 0;
}

int x_prop_wait_any(void * pa_,unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/){
	tPropArea * pa = (tPropArea *)pa_;
	return _wait(&pa->serial,old_serial,new_serial_p,timeout);
}

int x_prop_wait(void * pi_, unsigned int old_serial, unsigned int *new_serial_p, int timeout/*ms*/){
	tPropInfo * pi = (tPropInfo *)pi_;
	return _wait(&pi->serial,old_serial,new_serial_p,timeout);
}


unsigned int x_prop_get_pi_serial(void * pi_){
	tPropInfo * pi = (tPropInfo *)pi_;
	return atomic_load_explicit(&pi->serial,memory_order_acquire);
}
unsigned int x_prop_get_pa_serial(void * pa_){
	tPropArea * pa = (tPropArea *)pa_;
	return atomic_load_explicit(&pa->serial,memory_order_acquire);
}