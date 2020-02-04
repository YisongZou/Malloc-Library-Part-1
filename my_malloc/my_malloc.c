#include "my_malloc.h"
#include <stdio.h>
#include <unistd.h>

//Global variable for the function get_data_segment_size( )
size_t data_segment_size = 0;

typedef struct List_Node ListNode;
//Using Linked list to manage the memory
struct List_Node{
  ListNode* next;//8 bytes
  ListNode* prev;//8 bytes 
  size_t size;//8 bytes 
};


//The head of the free List
ListNode* head = NULL;

//If there is still enough space established, split it into two parts
ListNode* split(ListNode* current, size_t size){
  current->size -= sizeof(ListNode);
  current->size	-= size;
  ListNode * temp = (ListNode*)((char*)current+ sizeof(ListNode) + current->size);
  temp->size = size;
  return temp;
}

//Establish New nodes if there is not enough space
void* establish(size_t size){
  ListNode* current = sbrk(sizeof(ListNode) + size);
   //Update global variable
  data_segment_size += size;
  data_segment_size += sizeof(ListNode);
  current->size = size;
  return current;
 }

//Find the first fit block currently in the linked list, allocate an
//address from the first free region with enough space to fit the requested allocation size. 
ListNode *  ff_find(size_t size){
  ListNode * current = head;
    while (current){
      if( current->size >= size){
	return current;
      }
        current = current->next;
      }
    return current;
}

void* ff_malloc(size_t size){
   ListNode * current;
  ListNode * malloced;
       if ( head ){
           if ( (current = ff_find(size)) ){
	//Is able to split
		if (current->size - size > sizeof(ListNode)){
	       malloced =  split(current, size); 
           }
	//Found the node and use it without split
		else{
		  if(current->prev == NULL && current->next == NULL){
		    head = NULL;
		    malloced = current;
		  }
		 else if(current->prev == NULL){
		   head = current->next;
		   current->next->prev = NULL;
		   malloced = current;
                  }
		  else if(current->next == NULL){
		    current->prev->next = NULL;
		    malloced = current;
                  }
		  else{
		    current->prev->next = current->next;
		    current->next->prev = current->prev;
		    malloced = current;
		  }
		}
	   }
       
      else {
	//Establish a note at the end
	malloced = establish(size);
      }
       }
       else{
	 //First time establish
	   malloced = establish(size);
     	 }
       return (char*)malloced + sizeof(ListNode);
}

//This function will help to merge the neighboring free blocks which
//can solve the problem of poor region selection during malloc.
ListNode* merge(ListNode * current){
     current->size += sizeof(ListNode);
    current->size += current->next->size;
    current->next = current->next->next;
      if (current->next){
            current->next->prev= current;
        }
   return current;
}

void add_free(void*ptr){
  ListNode * temp = (ListNode*)(ptr - sizeof(ListNode));
  if(head == NULL){
    head = temp;
    head->next = NULL;
    head->prev = NULL;
  }
  else{
    if(temp < head){
	temp->next = head;
        temp->prev = NULL;
        head = temp;
        return;
      }
    ListNode * last = head;
    ListNode * current = head;
    while(current != NULL){
      if(current < temp ){
	if(current->next && temp < current->next){
	  temp->next = current->next;
	  current->next = temp;
	  temp->next->prev = temp;
	  temp->prev = current;
	  return;
	}
      }
	last = current;
	current= current->next;
    }
      last->next = temp;
      temp->prev = last;
      temp->next = NULL;
  }
}

void ff_free(void* ptr){
    ListNode*  current = (ListNode*)((char*)(ptr) - sizeof(ListNode));
    add_free(ptr);
    if (current->next && ((char*)current + sizeof(ListNode) + current->size  == (char*)current->next)){
       merge(current);
    }
    if (current->prev && ((char*)current - current->prev->size - sizeof(ListNode) ==(char*) current->prev)){
      merge(current->prev);
    }
}

//The function bf_find( ) will find the best fit free node in the
//linked list when a new chunk of memory need to be malloced, allocate
//an address from the free region which has the smallest number of
// bytes greater than or equal to the requested allocation size.
ListNode* bf_find(size_t size){
  ListNode* current = head;
  ListNode* best = NULL;
  int rest = 0;
  int min_rest = 0;
  //Get first data for min_rest
  while (current){
      rest = current->size - size;
      if(rest == 0){
	return current;
      }
      if(rest > 0 ){
	min_rest = rest;
	best = current;
	break;
      }
    current = current->next;
    }
  //If there cannot find such node
  if(current == NULL){
    return current;
  }
  current = head;
  //Find the best fit node
  while (current){
    rest = current->size - size;
	if(rest == 0){
	  return current;
	}
	if(rest > 0 && rest < min_rest){
	  best = current;
	  min_rest = rest;
	}	
      current = current->next;
  }
  return best;
}

//Best Fit malloc/free
void *bf_malloc(size_t size){
 ListNode * current;
  ListNode * malloced;
       if ( head ){
           if ( (current = bf_find(size)) ){
        //Is able to split
                if (current->size - size > sizeof(ListNode)){
               malloced =  split(current, size);
           }
        //Found the node and use it without split
		else{
                  if(current->prev == NULL && current->next == NULL){
                    head = NULL;
                    malloced = current;
                  }
                 else if(current->prev == NULL){
                   head = current->next;
                   current->next->prev = NULL;
                   malloced = current;
                  }
                  else if(current->next == NULL){
                    current->prev->next = NULL;
                    malloced = current;
                  }
                  else{
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                    malloced = current;
                  }
                }
           }
             else {
        //Establish a note at the end
	       malloced = establish(size);
      }
       }
       else{
         //First time establish
	 malloced = establish(size);
         }
       return (char*)malloced + sizeof(ListNode);
}

void bf_free(void *ptr){
ListNode*  current = (ListNode*)((char*)(ptr) - sizeof(ListNode));
    add_free(ptr);
    if (current->next && ((char*)current + sizeof(ListNode) + current->size  == (char*)current->next)){
       merge(current);
    }
    if (current->prev && ((char*)current - current->prev->size - sizeof(ListNode) ==(char*) current->prev)){
      merge(current->prev);
    }
}

unsigned long get_data_segment_size(){
  return data_segment_size;
}

unsigned long get_data_segment_free_space_size(){
   size_t size = 0;
  ListNode* current = head;
  while (current){
      size += sizeof(ListNode);
      size += current->size;
      current = current->next;
  }
  return size;
}
