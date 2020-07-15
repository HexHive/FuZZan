#include<stdio.h>
#include<stdlib.h>

int data1;

int main() {
  int data2;

  int *data3=(int *)malloc(sizeof(int));

  printf("global %p stack %p heap %p \n", &data1, &data2, data3);
  free(data3);

  return 1;
}
