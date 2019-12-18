//! var dim_64: -DDIM=64
//! var dim_128: -DDIM=128
//! var dim_256: -DDIM=256
//! var dim_512: -DDIM=512
//! var dim_768: -DDIM=768

#include <stdio.h>
#include <stdlib.h>

typedef int T;


T kernel(T x, T y) {
  return (x*y);
}


void mx_process(T a[DIM][DIM], T b[DIM][DIM], T out[DIM][DIM])
{
  for (int ia = 0; ia < DIM; ++ia)
    for (int ib = 0; ib < DIM; ++ib)
      {
	T sum = 0;
	for (int id = 0; id < DIM; ++id)
	  
	  sum += kernel(a[ia][id],b[id][ib]);
	
	out[ia][ib] = sum;
      }
}

T mx_reduce(T A[DIM][DIM], T B[DIM][DIM], T C[DIM][DIM]) {
  int i,j,ret_val;

  for (i = 0; i<DIM; i++)
    for (j = 0; j<DIM; j++)
      ret_val += A[i][j]+B[i][j];

  return ret_val;
}

void mx_init(T A[DIM][DIM], T B[DIM][DIM]) {
  int i,j;
  
  for(i = 0; i<DIM; i++)
    for(j = 0; j<DIM; j++)
      A[i][j] = (T)(i+j);
  
  for(i = 0; i<DIM; i++)
    for(j = 0; j<DIM; j++)
      B[i][j] = (T)(i*j);

}

int main(void)
{
  
  T ret_val = 0;
  
  int i,j, err;
  
  T mA[DIM][DIM];
  T mB[DIM][DIM];
  T mC[DIM][DIM];

  mx_init(mA,mB);
  
 
  mx_process(mA,mB,mC);
  
  ret_val = mx_reduce(mA,mB,mC);

        
  return 0;
  
}

