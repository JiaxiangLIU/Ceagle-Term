#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    int n;
    int  i = 1;
    int product = 1;

    n = atoi(argv[1]);


    while(i <= n) {
      product = product * i;
      i = i  +1;
    }

    return product;
}
