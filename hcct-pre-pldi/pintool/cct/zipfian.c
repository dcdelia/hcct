#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double zipf(double k, double s, double h) {
    return 1.0/(pow(k,s)*h);
}

int main(int argc, char** argv){
    
    if (argc < 3) {
        printf("usage: %s num-elements exp\n", argv[0]);
        exit(1);
    }

    double h = 0;
    unsigned k;
    unsigned N = atoi(argv[1]);
    double s = atof(argv[2]);

    // compute harmonic number H(N,s)
    for (k = 1; k<=N; ++k) h += 1.0/pow(k,s);

    printf("number of elements (N) = %u\n", N);
    printf("exponent (s) = %f\n", s);
    printf("harmonic number H(N,s) = %f\n", h);
    printf("zipfian ditribution:\n");

    printf("%-10s zipf(rank, %.2f, %u)\n", "rank", s, N);
    for (k = 1; k<=N; k *= 2)
        printf("%-10u %-.25f\n", k, zipf(k, s, h));

    return 0;
}
