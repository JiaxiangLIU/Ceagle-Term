#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    int n;

    n = atoi(argv[1]);

    int  i = 1;
    int sum = 0;
    int product = 1;
    while(i <= n) {
      sum = sum + i;
      product = product * i;
      i = i  +1;
    }

    assert(product ==0);
    return product;
}
