/**
 * @file TSP.inc
 * @brief Procedures related to distance computation (TSP)
 * @author Victoriano Monstesinos Cánovas
 * Based on Thomas Stuetzle's code (2002)
 * @date 5 Apr 2017
 */

#define TRUNC(x)	real(int(x))
#define GEO(a,b)	(M_PI * (a + 5.0f * b / 3.0f) / 180.0f)

struct point {
    real x;
    real y;
};

char name[LINE_BUF_LEN];	// Instance name
struct point * node;		// Array of structs containing coordinates of nodes
integer ** distance_matrix;	// Distance[i][j] gives distance between city i and j
real ** heuristic;		// Heuristic information matrix. Stores the computation of (1 / distance) ^ beta (Scalar optimization)

integer (*distance)(const integer, const integer); // Function pointer

/* The following four functions implement different ways of computing distances
 * for TSPLIB instances. The distance is rounded to next integer. */

/**
 * @brief Compute Euclidean distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
integer round_distance (const integer i, const integer j)
{
    const real xd = node[i].x - node[j].x;
    const real yd = node[i].y - node[j].y;
    return (integer) (sqrtf(xd*xd + yd*yd) + 0.5f);
}

/**
 * @brief Compute ceiling distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
integer ceil_distance (const integer i, const integer j)
{
    const real xd = node[i].x - node[j].x;
    const real yd = node[i].y - node[j].y;
    return (integer) (sqrtf(xd*xd + yd*yd) + 0.000000001f);
}

/**
 * @brief Compute geometric distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
integer geo_distance (const integer i, const integer j)
{
    const real x1 = node[i].x;
    const real x2 = node[j].x;
    const real y1 = node[i].y;
    const real y2 = node[j].y;

    real deg = TRUNC(x1);
    const real lati = GEO(deg, x1 - deg);
    deg = TRUNC(x2);
    const real latj = GEO(deg, x2 - deg);

    deg = TRUNC(y1);
    const real longi = GEO(deg, y1 - deg);
    deg = TRUNC(y2);
    const real longj = GEO(deg, y2 - deg);

    const real q1 = cosf(longi - longj);
    const real q2 = cosf(lati - latj);
    const real q3 = cosf(lati + latj);

    return (integer) (6378.388f * acosf(0.5f * ((1.0f + q1) * q2 - (1.0f - q1) * q3)) + 1.0f);
}

/**
 * @brief Compute ATT distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
integer att_distance (const integer i, const integer j)
{
    const real xd = node[i].x - node[j].x;
    const real yd = node[i].y - node[j].y;
    const real rij = sqrtf((xd * xd + yd * yd) * 0.1f);
    const real tij = TRUNC(rij);
    integer dij;

    if (tij < rij)
        dij = (integer) tij + 1;
    else
        dij = (integer) tij;

    return dij;
}

/**
 * @brief Compute toroid distance between two nodes
 * @param i Node index
 * @param j Node index
 * @return Distance between the two nodes
 */
integer toroid_distance (const integer i, const integer j)
{
    const real temp1 = abs(node[i].x - node[j].x);
    const real temp2 = abs(node[i].y - node[j].y);

    const real xd = MIN(temp1, 1000.0f - temp1);
    const real yd = MIN(temp2, 1000.0f - temp2);

    return (integer) (sqrtf(xd*xd + yd*yd) + 0.5f);
}

/**
 * @brief Computes the matrix of all intercity distances
 */
void compute_distance_matrix ()
{
    distance_matrix = generate_integer_matrix(n, n, false);

    for (integer i = 0; i < n; i++)
        for (integer j = 0; j < n; j++)
            distance_matrix[i][j] = distance(i, j);
}

/**
 * @brief Computes heuristic information
 */
void compute_heuristic_information ()
{
    heuristic = generate_real_matrix(n, n, false);

    for (integer i = 0; i < n; i++)
        for (integer j = 0; j < n; j++)
            heuristic[i][j] = powf(1.0f / ((real) distance_matrix[i][j] + 0.1f), beta);
            // Add a small constant to avoid division by zero if a distance is zero
}

/**
 * @brief Compute the length of a tour
 * @param t Pointer to a tour
 * @return Tour length
 */
integer compute_tour_length (integer * t)
{
    integer tour_length = 0;

    for (integer i = 0; i < n; i++)
        tour_length += distance_matrix[t[i]][t[i+1]];

    return tour_length;
}

