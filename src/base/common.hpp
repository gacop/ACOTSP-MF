/**
 * @file common.hpp
 * @brief Libraries and macros
 * @author Victoriano Montesinos Cánovas
 * @date 20 Dec 2017
 */
#ifndef _COMMON_HPP
#define _COMMON_HPP

#include <cstdlib>
#include <climits>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include "omp.h"
#include "mkl_vsl.h"
using namespace std;

#define integer int
#define real	float

#define INFTY	INT_MAX

#define CACHE_LINE	64    // 64 bytes

#ifdef __MIC__
#define VECTOR_LENGTH	16
#else
#define VECTOR_LENGTH	8
#endif

/*#define TRUE	-1
#define FALSE	1*/

#define TRUE	-1.0f
#define FALSE	1.0f

#define LINE_BUF_LEN     100

#define MIN(a,b)	(a < b ? a : b)

#endif // _COMMON_HPP
