/**
 * @file TSP.inc
 * @brief Code for computing distances between coordinates
 * @author Martinez Sanchez, Pablo A.
 * @date 28 Oct 2020
 */
 
#define TRUNC(x)	float(int(x))
#define GEO(a,b)	(M_PI * (a + 5.0f * b / 3.0f) / 180.0f)

struct point {
  float x;
  float y;
};

char name[LINE_BUF_LEN];	 // Instance name
struct point * city_coordinates; // Array of structs containing coordinates of cities

// In the case of the EUC_2D, where round_distance is used, is a very slow distance
// calculation compared to squared_distance, so, in the get_distance_matrix, the
// squared_distance is used, while this function pointer stills points to the
// round_distance
int (*distance)(const int, const int); // Function pointer

/* The following four functions implement different ways of computing distances
 * for TSPLIB instances. The distance is rounded to next int. */

/**
 * @brief Compute Euclidean distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
int round_distance (const int i, const int j)
{
    const float xd = city_coordinates[i].x - city_coordinates[j].x;
    const float yd = city_coordinates[i].y - city_coordinates[j].y;
    return (int) (sqrtf(xd*xd + yd*yd) + 0.5f);
}

/**
 * The same as round_distance, but without sqrt!
 */
int squared_distance(const int i, const int j)
{
  float xd = city_coordinates[i].x - city_coordinates[j].x;
  float yd = city_coordinates[i].y - city_coordinates[j].y;
  return (int)(xd*xd) + (int)(yd*yd);
}

/**
 * @brief Compute ceiling distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
int ceil_distance (const int i, const int j)
{
    const float xd = city_coordinates[i].x - city_coordinates[j].x;
    const float yd = city_coordinates[i].y - city_coordinates[j].y;
    return (int) (sqrtf(xd*xd + yd*yd) + 0.000000001f);
}

/**
 * @brief Compute geometric distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
int geo_distance (const int i, const int j)
{
    const float x1 = city_coordinates[i].x;
    const float x2 = city_coordinates[j].x;
    const float y1 = city_coordinates[i].y;
    const float y2 = city_coordinates[j].y;

    float deg = TRUNC(x1);
    const float lati = GEO(deg, x1 - deg);
    deg = TRUNC(x2);
    const float latj = GEO(deg, x2 - deg);

    deg = TRUNC(y1);
    const float longi = GEO(deg, y1 - deg);
    deg = TRUNC(y2);
    const float longj = GEO(deg, y2 - deg);

    const float q1 = cosf(longi - longj);
    const float q2 = cosf(lati - latj);
    const float q3 = cosf(lati + latj);

    return (int) (6378.388f * acosf(0.5f * ((1.0f + q1) * q2 - (1.0f - q1) * q3)) + 1.0f);
}

/**
 * @brief Compute ATT distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
int att_distance (const int i, const int j)
{
    const float xd = city_coordinates[i].x - city_coordinates[j].x;
    const float yd = city_coordinates[i].y - city_coordinates[j].y;
    const float rij = sqrtf((xd * xd + yd * yd) * 0.1f);
    const float tij = TRUNC(rij);
    int dij;

    if (tij < rij)
        dij = (int) tij + 1;
    else
        dij = (int) tij;

    return dij;
}

/**
 * @brief Compute toroid distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
int toroid_distance (const int i, const int j)
{
    const float temp1 = abs(city_coordinates[i].x - city_coordinates[j].x);
    const float temp2 = abs(city_coordinates[i].y - city_coordinates[j].y);

    const float xd = MIN(temp1, 1000.0f - temp1);
    const float yd = MIN(temp2, 1000.0f - temp2);

    return (int) (sqrtf(xd*xd + yd*yd) + 0.5f);
}

/**
 * @brief Compute the length of a tour
 * @param t Pointer to a tour
 * @return Tour length
 */
int compute_tour_length (int * t)
{
    int tour_length = 0;

    for (int i = 0; i < n; i++)
        tour_length += distance(t[i], t[i+1]);

    return tour_length;
}


// Using squared_distance in this moment saves a lot of time, and
// it is still usable because we just want to know the distance
// between the cities, but does not need to be the pure euclidian distance
int * get_distance_matrix(int origin_city) {
  int * distance_submatrix = (int *) _mm_malloc(sizeof(int) * n, CACHE_LINE);

  if(distance == round_distance)
    for(int i=0;i<n;i++) distance_submatrix[i] = squared_distance(origin_city, i);
  else
    for(int i=0;i<n;i++) distance_submatrix[i] = distance(origin_city, i);

  return distance_submatrix;
}

int * get_distance_matrix_parallel(int origin_city) {
  int * distance_submatrix = (int *) _mm_malloc(sizeof(int) * n, CACHE_LINE);

  if(distance == round_distance) {
    #pragma omp parallel for
    for(int i=0;i<n;i++) distance_submatrix[i] = squared_distance(origin_city, i);
  }
  else {
    #pragma omp parallel for
    for(int i=0;i<n;i++) distance_submatrix[i] = distance(origin_city, i);
  }

  return distance_submatrix;
}

