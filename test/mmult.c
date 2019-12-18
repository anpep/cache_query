//! var mmult_200: -DN=200
//! var mmult_500: -DN=500
//! var mmult_1000: -DN=1000
//! var mmult_1500: -DN=1500

#include <stdlib.h>
#include <string.h>
#include <time.h>

void mmult(int A[N][N], int B[N][N], int C[N][N]) {
  unsigned int i,j,k;
  
  for (i = 0; i < N; ++i)
    for (j = 0; j < N; ++j)
      for (k = 0; k < N; ++k)
        C[i][j] += A[i][k] * B[k][j];

  return;
}

	   
int main(void) {
  static int m1[N][N], m2[N][N], res[N][N];
  unsigned int i,j;

  memset(res,0,N*N*sizeof(int));

  srand(time(NULL));
  
  for (i = 0; i < N; ++i)
    for (j = 0; j < N; ++j) {
      m1[i][j] = m2[i][j] = (int)rand();
    }

  mmult(m1,m2,res);

  return 0;
}
