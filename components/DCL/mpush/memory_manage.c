
#include <stdlib.h>
#include "memory_manage.h"

void *mm_malloc(size_t mem_size)
{
	return malloc(mem_size);
}

void mm_free(void *mem)
{
	if (mem != NULL)
	{
		free(mem);
	}
}


