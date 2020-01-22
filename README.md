# Malloc-Library-Part-1
For project implement my own version of several memory allocation functions from the C standard library.

I implement versions of malloc() and free(): 

void *malloc(size_t size); void free(void *ptr);

To implement my allocation strategies, I created 4 functions:

//First Fit malloc/free
void *ff_malloc(size_t size); 
void ff_free(void *ptr);
//Best Fit malloc/free
void *bf_malloc(size_t size); 
void bf_free(void *ptr);

If there is no free space that fits an allocation request, then sbrk() is used to create that space. On free(),my implementation merge the newly freed region with any currently free adjacent regions. In other words, my bookkeeping data structure does not contain multiple adjacent free regions, as this would lead to poor region selection during malloc.
