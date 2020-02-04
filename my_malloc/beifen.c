#include "my_malloc.h"
#include <stdio.h>
#include <unistd.h>

//The size of the fields in the node except the field first_byte
#define METADATA_SIZE 40

//Global variable for the function get_data_segment_size( )
size_t data_segment_size = 0;

typedef struct List_Node ListNode;
//Using Linked list to manage the memory
struct List_Node{
  ListNode* front;//8 bytes
   ListNode* next;//8 bytes
  ListNode* prev;//8 bytes 
  size_t size;//8 bytes 
  int placeholder;//4 bytes
  int free;//4 bytes
 };


//The head of the linked List
ListNode* head = NULL;

//If there is still enough space established, split it into two parts
void split(ListNode* current, size_t size){
  ListNode* splitted_second;
  //Find the firast byte for the new node
  splitted_second = (ListNode*)((char*)(current->front) + METADATA_SIZE + size);
  //connect the new node
  splitted_second->front = (ListNode*)((char*)(current->front) + METADATA_SIZE + current->size);
  splitted_second->next = current->next;
  splitted_second->prev = current;
  splitted_second->size = current->size - size - METADATA_SIZE;
  splitted_second->free = 1;
    if(current->next){
  current->next->prev = splitted_second;
    }
  current->next = splitted_second;
  current->size = size;
}

//Establish New nodes if there is not enough space
ListNode* establish(ListNode* last, size_t size){
  ListNode* current = sbrk(0);
  //If sbrk fails
  if (sbrk(METADATA_SIZE + size) == (void*)-1 ){
  return NULL;
  }
  //Update global variable
  data_segment_size += size;
  data_segment_size += METADATA_SIZE;
  //Connect
  current->next = NULL;
  current->size = size;
  current->prev = last;
  current->free = 0;
  if ( last ) {
    last->next  = current;
  }
  return current;
}

//Find the first fit block currently in the linked list, allocate an
//address from the first free region with enough space to fit the requested allocation size. 
ListNode *  ff_find(ListNode ** last, size_t size){
  ListNode * current = head;
    while (current){
      if(current->free && current->size >= size){
	return current;
      }
        *last = current;
        current = current->next;
      }
    return current;
}

void* ff_malloc(size_t size){
  ListNode* last;
  ListNode * current;
       if ( head ){
      last = head;
      if ( (current = ff_find(&last, size)) ){
	//Is able to split
	if (current->size - size > METADATA_SIZE + 16){
	      split(current, size); 
            }
	//Found the node and use it without split
	  current->free = 0;
      }
      else {
	//Establish a note at the end
	current = establish(last, size);
	if (!current){
	    return NULL;
	  }
      }
    }
       else{
	 //First time establish
	   current = establish(NULL, size);
	   if (!current){
	       return NULL;
	     }
	   head = current;
	 }
       current->front = current;
       void * ans = (char*)(current->front) + METADATA_SIZE;
       return ans;
}

//This function will help to merge the neighboring free blocks which
//can solve the problem of poor region selection during malloc.
ListNode* merge(ListNode * current){
  if (current->next && current->next->free){
    current->size += METADATA_SIZE;
    current->size += current->next->size;
    current->next = current->next->next;
      if ( current->next){
            current->next->prev= current;
        }
    }
  return current;
}

void ff_free(void* ptr){
  if(head && ptr > (void*)head && ptr < sbrk(0)){ 
    ListNode*  current = (ListNode*)((char*)(ptr) - METADATA_SIZE);
    current->free = 1;
    if (current->next){
       merge(current);
    }
    if (current->prev && current->prev->free){
      current = merge(current->prev);
    }
   }
}

//The function bf_find( ) will find the best fit free node in the
//linked list when a new chunk of memory need to be malloced, allocate
//an address from the free region which has the smallest number of
// bytes greater than or equal to the requested allocation size.
ListNode* bf_find(ListNode ** last, size_t size){
  ListNode* current = head;
  ListNode* best = NULL;
  int rest = 0;
  int min_rest = 0;
  //Get first data for min_rest
  while (current){
    if(current->free){
      rest = current->size - size;
      if(rest == 0){
	return current;
      }
      if(rest > 0 ){
	min_rest = rest;
	best = current;
	break;
      }
    }
    *last = current;
    current = current->next;
    }
  //If there cannot find such node
  if(current == NULL){
    return current;
  }
  current = head;
  //Find the best fit node
  while (current){
    if(current->free){
        rest = current->size - size;
	if(rest == 0){
	  return current;
	}
	if(rest > 0 && rest < min_rest){
	best = current;
	min_rest = rest;
      }
    }
      *last = current;
      current = current->next;
  }
  return best;
}

//Best Fit malloc/free
void *bf_malloc(size_t size){
  ListNode* last;
  ListNode * current;
  if ( head ){
    last = head;
    if ( (current = bf_find(&last, size)) ){
      //Is able to split
      if (current->size - size > METADATA_SIZE + 16){
	split(current, size);
      }
      //Is not able to split but find the node
          current->free = 0;
    }
    else {
      //Establish a node at the end
      current = establish(last, size);
      if (!current){
            return NULL;
          }
    }
  }
  else{
    //First time establish
    current = establish(NULL, size);
    if (!current){
      return NULL;
    }
    head = current;
  }
current->front = current;
       void * ans = (char*)(current->front) + METADATA_SIZE;
       return ans;
}

void bf_free(void *ptr){
  if(head && ptr > (void*)head && ptr < sbrk(0)){
    ListNode*  current = (ListNode*)((char*)(ptr) - METADATA_SIZE);
    current->free = 1;
    if (current->next){
      merge(current);
    }
    if (current->prev && current->prev->free){
      current = merge(current->prev);
    }
  }
}

unsigned long get_data_segment_size(){
  return data_segment_size;
}

unsigned long get_data_segment_free_space_size(){
   size_t size = 0;
    ListNode* current = head;
  while (current){
    if(current->free){
      size += METADATA_SIZE;
      size += current->size;
    }
      current = current->next;
  }
  return size;
}
