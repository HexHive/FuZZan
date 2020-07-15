#include <stdio.h> 
#include <stdlib.h>

// Assigning functions to be executed before and 
// after main() 
void __attribute__((constructor)) calledFirst(); 
void __attribute__((destructor)) calledLast(); 

int k;

int main() 
{
  int p;
  double *twice = (double *) malloc(sizeof(double));

  printf("\nstart real main stack %p global %p heap %p \n", &p, &k, (void *) twice);
  //printf("\nstart real main \n");

  return 0; 
} 

// This function is assigned to execute before 
// main using __attribute__((constructor)) 
void calledFirst() 
{
  int p; 
  printf("\called first %p\n", &p);
} 
  
// This function is assigned to execute after 
// main using __attribute__((destructor)) 
void calledLast() 
{ 
  printf("\nI am called last (desctructor)\n"); 
} 
