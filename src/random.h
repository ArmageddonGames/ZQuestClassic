#ifndef RANDOM_H
#define RANDOM_H

#include "zdefs.h"
#include <random>

typedef std::mt19937 zc_randgen;

int32_t zc_oldrand(zc_randgen* rng=NULL);
int32_t zc_rand(zc_randgen* rng=NULL);
int32_t zc_rand(int32_t upper, int32_t lower=0, zc_randgen* rng=NULL);
void zc_srand(int32_t seedval, zc_randgen* rng=NULL);

#endif

