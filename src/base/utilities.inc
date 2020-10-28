/**
 * @file utilities.inc
 * @brief Some additional useful procedures
 * @author Victoriano Montesinos Cánovas
 * @date 20 Dec 2017
 */

/* Constants for a random number generator, for details see numerical recipes in C */
/*#define IA	16807
#define IM	2147483647
#define AM	(1.0f/IM)
#define IQ	127773
#define INV_IQ	(1.0f/IQ)
#define IR	2836

integer ** seeds;*/

#define IA	16807.0f
#define IM	2147483647.0f
#define AM	(1.0f/IM)
#define IQ	127773.0f
#define INV_IQ	(1.0f/IQ)
#define IR	2836.0f

real ** seeds;

VSLStreamStatePtr rnStream;	// Random stream


/*real random01 (integer * seed_ptr)
{
    integer seed = *seed_ptr;
    const integer k = seed * INV_IQ;
    seed = IA * (seed - k * IQ) - IR * k;
    if (seed < 0)
        seed += IM;

    *seed_ptr = seed;

    return (real)(AM * seed);
}*/

real random01 (real * seed_ptr)
{
    real seed = *seed_ptr;
    //const integer k = seed * INV_IQ;
    const real k = integer(seed * INV_IQ);
    //const real k = roundf(seed * INV_IQ);
    seed = IA * (seed - k * IQ) - IR * k;
    if (seed < 0)
        seed += IM;

    *seed_ptr = seed;

    return AM * seed;
}

/**
 * @brief Generate a random number that is uniformly distributed in [0,1]
 * Random number seed is modified (important, this has to be done).
 * @param thread_id Thread identifier
 * @return Random number uniformly distributed in [0,1]
 */
/*real random01 (const integer thread_id)
{
    integer * seed = seeds[thread_id];
    //const integer k = (*seed) / IQ;
    const integer k = (*seed) * INV_IQ;
    (*seed) = IA * ((*seed) - k * IQ) - IR * k;
    if ((*seed) < 0)
        (*seed) += IM;
    return (real) (AM * (*seed));
}*/

/*real random01 (integer * seed)
{
    const integer k = (*seed) * INV_IQ;
    (*seed) = IA * ((*seed) - k * IQ) - IR * k;
    if ((*seed) < 0)
        (*seed) += IM;
    return (real) (AM * (*seed));
}*/

/**
 * @brief Generate a random number that is uniformly distributed in [0,1]
 * Seed is modified (important, this has to be done).
 * @param seed_ptr Pointer to a seed
 * @return Random number uniformly distributed in [0,1]
 */
/*real random01 (integer * seed_ptr)
{
    integer seed = *seed_ptr;
    const real k = seed * INV_IQ;
    seed = IA * (seed - k * IQ) - IR * k;
    if (seed < 0)
        seed += IM;

    *seed_ptr = seed;

    return (real)(AM * seed);
}*/
/*real random01 (real * seed_ptr)
{
    real seed = *seed_ptr;
    const real k = seed * INV_IQ;
    seed = IA * (seed - k * IQ) - IR * k;
    if (seed < 0)
        seed += IM;

    *seed_ptr = seed;

    return AM * seed;
}*/

/**
 * @brief Generate a random number that is uniformly distributed in [a,b)
 * @param a Left interval bound
 * @param b Right interval bound
 * @return Random number uniformly distributed in [a,b)
 */
integer integer_random (const integer a, const integer b)
{
    int random;
    viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, rnStream, 1, &random, a, b);
    return (integer) random;
}

/**
 * @brief Compute the amount of elements to allocate as padding
 * @param n Number of elements of the structure to be padded
 * @param element_type_size Number of bytes of the structure's elements' type
 * @param k Number of cache lines to use as prefetch padding
 * @return Amount of padding elements
 */
integer padding_elements(const integer n, const integer element_type_size, const integer k)
{
    const integer pad_elements = CACHE_LINE / element_type_size;

    const integer align_pad = pad_elements - n % pad_elements;

    const integer prefetch_pad = k * pad_elements;

    return align_pad + prefetch_pad;
}

/**
 * @brief Malloc a matrix (n x m)
 * @param n Number of rows
 * @param m Number of columns
 * @param prefetch_pad If true, prefetch padding is used; otherwise not
 * @return Pointer to matrix
 */
real ** generate_real_matrix (const integer n, const integer m, const bool prefetch_pad)
{
    const integer k = (prefetch_pad ? K_PAD : 0);
    const integer nPadded = n + padding_elements(n, sizeof(real *), k);
    const integer mPadded = m + padding_elements(m, sizeof(real), k);

    real ** matrix = (real **) _mm_malloc(sizeof(real) * n * mPadded + sizeof(real *) * nPadded, CACHE_LINE);

/*    if (prefetch_pad)
    {
        #pragma omp parallel for
        for (integer i = 0; i < n; i++)
            matrix[i] = (real *)(matrix + nPadded) + i*mPadded;
    }
    else
    {
        for (integer i = 0; i < n; i++)
            matrix[i] = (real *)(matrix + nPadded) + i*mPadded;
    }*/

    for (integer i = 0; i < n; i++)
        matrix[i] = (real *)(matrix + nPadded) + i*mPadded;

    return matrix;
}

/**
 * @brief Malloc a matrix (n x m)
 * @param n Number of rows
 * @param m Number of columns
 * @param prefetch_pad If true, prefetch padding is used; otherwise not
 * @return Pointer to matrix
 */
integer ** generate_integer_matrix (const integer n, const integer m, const bool prefetch_pad)
{
    const integer k = (prefetch_pad ? K_PAD : 0);
    const integer nPadded = n + padding_elements(n, sizeof(integer *), k);
    const integer mPadded = m + padding_elements(m, sizeof(integer), k);

    integer ** matrix = (integer **) _mm_malloc(sizeof(integer) * n * mPadded + sizeof(integer *) * nPadded, CACHE_LINE);

    for (integer i = 0; i < n; i++)
        matrix[i] = (integer *)(matrix + nPadded) + i*mPadded;

    return matrix;
}

/**
 * @brief Computes the mean of a vector
 * @param vector Array of integers
 * @param n Number of elements of the vector
 * @return Mean of the vector
 */
real mean_v (integer * vector, const integer n)
{
    integer sum = 0;

    for (integer i = 0; i < n; i++)
        sum += vector[i];

    return (real) sum / (real) n;
}

/**
 * @brief Computes the mean of a vector
 * @param vector Array of integers
 * @param n Number of elements of the vector
 * @return Mean of the vector
 */
double dmean_v (double * vector, const integer n)
{
    double sum = 0;

    for (integer i = 0; i < n; i++)
        sum += vector[i];

    return sum / (double) n;
}

/**
 * @brief Computes the standard deviation of a vector
 * @param vector Array of doubles
 * @param n Number of elements of the vector
 * @return Standard deviation of the vector
 */
real std_v (integer * vector, const integer n)
{
    const real mean = mean_v(vector, n);

    real sum = 0.0f;

    for (integer i = 0; i < n; i++) {
        real dif = (real) vector[i] - mean;
        sum += dif*dif;
    }

    return sqrtf(sum / (real) n);
}

/**
 * @brief Computes the standard deviation of a vector
 * @param vector Array of doubles
 * @param n Number of elements of the vector
 * @return Standard deviation of the vector
 */
double dstd_v (double * vector, const integer n)
{
    const double mean = dmean_v(vector, n);

    double sum = 0.0;

    for (integer i = 0; i < n; i++) {
        double dif = vector[i] - mean;
        sum += dif*dif;
    }

    return sqrt(sum / (double) n);
}

