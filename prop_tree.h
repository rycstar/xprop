#ifndef __PROP_TREE_H
#define __PROP_TREE_H
// Properties are stored in a hybrid trie/binary tree structure.
// Each property's name is delimited at '.' characters, and the tokens are put
// into a trie structure.  Siblings at each level of the trie are stored in a
// binary tree.  For instance, "ro.secure"="1" could be stored as follows:
//
// +-----+   children    +----+   children    +--------+
// |     |-------------->| ro |-------------->| secure |
// +-----+               +----+               +--------+
//                       /    \                /   |
//                 left /      \ right   left /    |  prop   +===========+
//                     v        v            v     +-------->| ro.secure |
//                  +-----+   +-----+     +-----+            +-----------+
//                  | net |   | sys |     | com |            |     1     |
//                  +-----+   +-----+     +-----+            +===========+

// Represents a node in the trie.

#ifdef __STD_C_NO_ATOMICS__
#error ##C11 standard needed##
#endif
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include "prop_api.h"

#define PROP_VALUE_MAX 92
/*****************prop define******************/
typedef struct prop_bt
{
	uint32_t name_len;
	atomic_uint prop;
	atomic_uint left;
	atomic_uint right;
	atomic_uint children;

	char name[0];
}tPropBt;

typedef struct prop_info
{
	atomic_uint serial;
	char value[PROP_VALUE_MAX];
	char name[0];
}tPropInfo;

/****************prop area (share memory) define**************/
typedef struct prop_area
{
	uint32_t pa_size;
	uint32_t pa_data_size;

	uint32_t bytes_used;
	atomic_uint serial;
	uint32_t magic;
	uint32_t version;
	uint32_t reserved[28];
	char data[0];
}tPropArea;


tPropInfo * find_prop(tPropArea * const pa, const char * name, uint32_t name_len, \
	const char * val, uint32_t val_len,uint8_t allow_alloc);

/*read/write permission*/
tPropArea * map_prop_area_rw(const char * path);

/*read only permission*/
tPropArea * map_prop_area_ro(const char * path);
#endif