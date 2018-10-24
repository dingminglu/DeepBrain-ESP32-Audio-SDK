#ifndef __MEMORY_MANAGE_H__
#define __MEMORY_MANAGE_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

void *mm_malloc(size_t mem_size);
void mm_free(void *mem);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __MEMORY_MANAGE_H__ */

