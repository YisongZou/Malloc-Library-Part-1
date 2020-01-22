#include "my_malloc.h"
#include <stdio.h>
#include <unistd.h>

#define METADATA_SIZE 32

size_t data_segment_size = 0;
//size_t free_space_size = 0;

typedef struct List_Node ListNode;
//Using Linked list to manage the memory
struct List_Node{
  ListNode* next;
  ListNode* prev;
  size_t size;
  int placeholder;
  int free;
  char first_byte[1];
};


//The head of the linked List                                                                                                                                
ListNode* head = NULL;


//If there is still enough space established, split it into two parts
void split(ListNode* current, size_t size){
  ListNode* splitted_second;
  splitted_second = (ListNode*)(current->first_byte + size);
  splitted_second->next = current->next;
  splitted_second->prev = current;
  splitted_second->size = current->size - size -METADATA_SIZE;
  splitted_second->free = 1;
    //  printf("split!!\n");
  if(current->next){
  current->next->prev = splitted_second;
  }
  current->next = splitted_second;
  current->size = size;
}

//Establish New nodes if there is not enough space
ListNode* establish(ListNode* last, size_t size){
  ListNode* current = sbrk(0);
  if (sbrk(METADATA_SIZE + size) == (void*)-1 ){
    return NULL;
  }
  data_segment_size += size;
  data_segment_size += METADATA_SIZE;
  current->next = NULL;
  current->size = size;
  current->prev = last;
  current->free = 0;
  if ( last ) {
    last->next  = current;
  }
  return current;
}

//FIND First FIT NODE 
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
	  if (current->size - size > METADATA_SIZE){
	      split(current, size); 
            }
	  current->free = 0;
	  //free_space_size -= size;
	  // free_space_size -= METADATA_SIZE;
        }
      else {
	current = establish(last, size);
	if (!current){
	    return NULL;
	  }
      }
    }
       else{
	   current = establish(NULL, size);
	   if (!current){
	       return NULL;
	     }
	   head = current;
	 }
       return current->first_byte;
}

ListNode* merge(ListNode * current){
  if (current->next && current->next->free){
    current->size += METADATA_SIZE;
    current->size += current->next->size;
    current->next = current->next->next;
    //    printf("merge!!!!\n");
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
    // free_space_size += current->size;
    // free_space_size += METADATA_SIZE;
    //  printf("free!\n");
    if (current->next){
       merge(current);
       //  printf("mergeright!\n");
    }
    if (current->prev && current->prev->free){
      current = merge(current->prev);
      //  printf("mergeleft!\n");
    }
   }
}

//FIND Best FIT NODE
ListNode* bf_find(ListNode ** last, size_t size){
  ListNode* current = head;
  ListNode* best = NULL;
  int rest = 0;
  int min_rest = 0;
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
  if(current == NULL){
    return current;
  }
  current = head; 
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
      if (current->size - size > METADATA_SIZE){
	split(current, size);
      }
          current->free = 0;
	  // free_space_size -= size;
	  // free_space_size -= METADATA_SIZE;
    }
    else {
      current = establish(last, size);
      if (!current){
            return NULL;
          }
    }
  }
  else{
    current = establish(NULL, size);
    if (!current){
      return NULL;
    }
    head = current;
  }
  return current->first_byte;
}

void bf_free(void *ptr){
  if(head && ptr > (void*)head && ptr < sbrk(0)){
    ListNode*  current = (ListNode*)((char*)(ptr) - METADATA_SIZE);
    current->free = 1;
    //free_space_size += current->size;
    // free_space_size += METADATA_SIZE;
    if (current->next){
      merge(current);
    }
    if (current->prev && current->prev->free){
      current = merge(current->prev);
    }
  }
}

unsigned long get_data_segment_size(){
  /* size_t size = 0;
  ListNode* current = head;
  while (current){
    size += METADATA_SIZE;
    size += current->size;
    current = current->next;
    }*/
  // printf("%ld\n", size);
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
  //  printf("%ld\n", size);*/
  return size;
}
