/**
 * @file utilities.inc
 * @brief Utilities functions
 * @author Martinez Sanchez, Pablo A.
 * @date 28 Oct 2020
 */
 
#define IA	16807.0f
#define IM	2147483647.0f
#define AM	(1.0f/IM)
#define IQ	127773.0f
#define INV_IQ	(1.0f/IQ)
#define IR	2836.0f

float ** seeds;

VSLStreamStatePtr rnStream;	// Random stream

float random01 (float * seed_ptr)
{
    float seed = *seed_ptr;
    const float k = int(seed * INV_IQ);
    seed = IA * (seed - k * IQ) - IR * k;
    if (seed < 0)
        seed += IM;

    *seed_ptr = seed;

    return AM * seed;
}

/**
 * @brief Generate a random number that is uniformly distributed in [a,b)
 * @param a Left interval bound
 * @param b Right interval bound
 * @return Random number uniformly distributed in [a,b)
 */
int integer_random (const int a, const int b)
{
    int random;
    viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, rnStream, 1, &random, a, b);
    return (int) random;
}

/**
 * @brief Compute the amount of elements to allocate as padding
 * @param n Number of elements of the structure to be padded
 * @param element_type_size Number of bytes of the structure's elements' type
 * @param k Number of cache lines to use as prefetch padding
 * @return Amount of padding elements
 */
int padding_elements(const int n, const int element_type_size, const int k)
{
    const int pad_elements = CACHE_LINE / element_type_size;

    const int align_pad = pad_elements - n % pad_elements;

    const int prefetch_pad = k * pad_elements;

    return align_pad + prefetch_pad;
}

float ** generate_real_matrix (const int n, const int m) {
    float ** matrix = (float **) _mm_malloc(sizeof(float *) * n, CACHE_LINE);

    for (int i = 0; i < n; i++)
      matrix[i] = (float *)_mm_malloc(sizeof(float) * m, CACHE_LINE);

    return matrix;
}

char ** generate_char_matrix (const int n, const int m, const bool prefetch_pad)
{
    const int k = (prefetch_pad ? K_PAD : 0);
    const int nPadded = n + padding_elements(n, sizeof(char *), k);
    const int mPadded = m + padding_elements(m, sizeof(char), k);

    char ** matrix = (char **) _mm_malloc(sizeof(char) * n * mPadded + sizeof(char *) * nPadded, CACHE_LINE);
    if(matrix == NULL) { fprintf(stderr, "You're dead\n"); }

    for (int i = 0; i < n; i++)
        matrix[i] = (char *)(matrix + nPadded) + i*mPadded;

    return matrix;
}

short ** generate_short_matrix (const int n, const int m, const bool prefetch_pad)
{
    const int k = (prefetch_pad ? K_PAD : 0);
    const int nPadded = n + padding_elements(n, sizeof(short *), k);
    const int mPadded = m + padding_elements(m, sizeof(short), k);

    short ** matrix = (short **) _mm_malloc(sizeof(short) * n * mPadded + sizeof(int *) * nPadded, CACHE_LINE);
    if(matrix == NULL) { fprintf(stderr, "You're dead\n"); }
 
    for (int i = 0; i < n; i++)
        matrix[i] = (short *)(matrix + nPadded) + i*mPadded;

    return matrix;
}

int ** generate_integer_matrix (const int n, const int m) {
    int ** matrix = (int **) _mm_malloc(sizeof(int *) * n, CACHE_LINE);

    for (int i = 0; i < n; i++)
      matrix[i] = (int *)_mm_malloc(sizeof(int) * m, CACHE_LINE);

    return matrix;
}

float **** generate_visited_aux(int m, int n, int neighbours) {
  float *** cube = (float ***) _mm_malloc(sizeof(float *) * n, CACHE_LINE);
  if(cube == NULL) { fprintf(stderr, "You're dead\n"); }

  for (int i = 0; i < n; i++) {
    cube[i] = (float **)_mm_malloc(sizeof(float*) * m, CACHE_LINE);
    if(cube[i] == NULL) { fprintf(stderr, "You're dead\n"); }
    for(int j=0;j<m;j++) {
      cube[i][j] = (float *) _mm_malloc(sizeof(float *) * neighbours, CACHE_LINE);
      if(cube[i][j] == NULL) { fprintf(stderr, "You're dead\n"); }
    }
  }

  return (float ****) cube;
}

/**
 * @brief Computes the mean of a vector
 * @param vector Array of integers
 * @param n Number of elements of the vector
 * @return Mean of the vector
 */
float mean_v (int * vector, const int n)
{
    int sum = 0;

    for (int i = 0; i < n; i++)
        sum += vector[i];

    return (float) sum / (float) n;
}

/**
 * @brief Computes the mean of a vector
 * @param vector Array of integers
 * @param n Number of elements of the vector
 * @return Mean of the vector
 */
double dmean_v (double * vector, const int n)
{
    double sum = 0;

    for (int i = 0; i < n; i++)
        sum += vector[i];

    return sum / (double) n;
}

/**
 * @brief Computes the standard deviation of a vector
 * @param vector Array of doubles
 * @param n Number of elements of the vector
 * @return Standard deviation of the vector
 */
float std_v (int * vector, const int n)
{
    const float mean = mean_v(vector, n);

    float sum = 0.0f;

    for (int i = 0; i < n; i++) {
        float dif = (float) vector[i] - mean;
        sum += dif*dif;
    }

    return sqrtf(sum / (float) n);
}

/**
 * @brief Computes the standard deviation of a vector
 * @param vector Array of doubles
 * @param n Number of elements of the vector
 * @return Standard deviation of the vector
 */
double dstd_v (double * vector, const int n)
{
    const double mean = dmean_v(vector, n);

    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        double dif = vector[i] - mean;
        sum += dif*dif;
    }

    return sqrt(sum / (double) n);
}

