/**
 * @file ants.inc
 * @brief Main ACOTSP-MF source code file
 * @author Martinez Sanchez, Pablo A.
 * @date 28 Oct 2020
 */
 
#include <assert.h>
#include <sys/time.h>

#define CHUNK 32
#define CHUNK_L (CHUNK+K_BACKUP)

int ** choice_info_meta;             // Choice information matrix

float ** choice_info_tmp;            // tmp data used at compute_choice_info()
int ** choice_info_meta_tmp;         // tmp data used at compute_choice_info()

/* Ant Colony */
int ** tour;		             // Ant tours
float ** visited;		     // Stores the information of which cities are visited
int * tour_length;	             // Length of the tours
/* Ant Colony */

int * best_tour;		     // Best tour found
int best_tour_length;                // Length of the best tour found

float ** choice_info;		     // Choice information matrix
float ** pheromone;
float pheromone_initial_trail;
float tmp_choose_next_t;

float rho_d;			     // 1 - rho

#define SKIP_TRIALS	0	     // Skip first trials (warm-up)

void (*fallback)(const int a, const int phase); // Fallback pointer

/* Time measurement */
double t_start_trial;
double t_init_trial;
double t_choice_info;
double t_choice_info_init;
double t_update_choice_info;
double t_construct_solutions;
double t_three_opt;
double t_stats;
double t_pheromone_evaporation;
double t_pheromone_deposit;
double * trial_time;
double vec_time;
double * tour_timer1;
double * tour_timer2;
double * tour_timer3;
/* Time measurement */

/* Solution quality measurement */
int * trial_sol;            // Best solution of each trial
/* Solution quality measurement */

int vector_iterations;
int vector_remainder;
int remainder_start;

// from LS
void three_opt( int *tour, int tid);

/**
 * @brief Allocate the memory for the ant colony and the best tour found
 * Each "tour" needs to be allocated for length n+1, since the first city of
 * a tour (at position 0) is repeated at position n. This is done to make the
 * computation of the tour length easier.
 */
void allocate_memory ()
{
    tour = generate_integer_matrix(m, n+1);

    visited = generate_real_matrix(m, n);

    tour_length = (int *) _mm_malloc(sizeof(int) * m, CACHE_LINE);

    best_tour = (int *) _mm_malloc(sizeof(int) * (n+1), CACHE_LINE);

    seeds = generate_real_matrix(threads, n);

    pheromone = generate_real_matrix(n, CHUNK_L);

    choice_info = generate_real_matrix(n, CHUNK_L);

    choice_info_meta = generate_integer_matrix(n, CHUNK_L);

    trial_time = (double *) _mm_malloc(sizeof(double) * (max_trials-SKIP_TRIALS), CACHE_LINE);

    trial_sol = (int *) _mm_malloc(sizeof(int) * (max_trials-SKIP_TRIALS), CACHE_LINE);

    // Just used at compute_choice_info()
    choice_info_tmp = generate_real_matrix(threads, n);

    choice_info_meta_tmp = generate_integer_matrix(threads, n);
}

/**
 * @brief Free the memory for the ant colony and the best tour found
 */
void free_memory ()
{
    _mm_free(tour);
    _mm_free(visited);
    _mm_free(tour_length);
    _mm_free(best_tour);
    _mm_free(seeds);
    _mm_free(pheromone);
    _mm_free(choice_info);
    _mm_free(trial_time);
    _mm_free(trial_sol);
}

/**
 * @brief Empty the ants's memory regarding visited cities
 * Vector of visited cities is reinitialized to FALSE (= Not visited).
 * @param a Ant identifier
 */
void ant_empty_memory (const int a)
{
    const int n_copy = n;

    float * visited_v = visited[a];

    __assume_aligned(visited_v, 64);

    for (int i = 0; i < n_copy; i++)
        visited_v[i] = FALSE;
}

void init_pheromone_trails ()
{
    for (int i = 0; i < n; i++) {
        float * pheromone_v = pheromone[i];

        __assume_aligned(pheromone_v, 64);

        for (int j = 0; j < CHUNK_L; j++)
            pheromone_v[j] = pheromone_initial_trail;
    }
}

void choose_closest_next_naive (const int a, const int phase){
    const int current_city = tour[a][phase-1];

    float * visited_v = visited[a];
    int * distance_v  = get_distance_matrix(current_city);

    int min = INFTY;
    int next_city = -1;

    __assume_aligned(visited_v, 64);
    __assume_aligned(distance_v, 64);

    #pragma ivdep
    for(int i=0; i < n; i++) {
      distance_v[i] = distance_v[i] * (int)visited_v[i];
      if(distance_v[i] > 0 && distance_v[i] < min) {
        min = distance_v[i];
        next_city = i;
      }
    }

    tour[a][phase] = next_city;
    visited_v[next_city] = TRUE;

    _mm_free(distance_v);
}

void choose_closest_next_naive_parallel (const int a, const int phase){
    const int current_city = tour[a][phase-1];

    float * visited_v = visited[a];
    int * distance_v  = get_distance_matrix_parallel(current_city);

    int n_threads = omp_get_max_threads();
    int min[n_threads];
    int next_city[n_threads];
    int next_city_ = -1;

    __assume_aligned(visited_v, 64);
    __assume_aligned(distance_v, 64);

    #pragma omp parallel
    {
      int tid = omp_get_thread_num();
      min[tid] = INFTY;
      next_city[tid] = -1;

      #pragma omp for
      for(int i=0; i < n; i++) {
        distance_v[i] = distance_v[i] * visited_v[i];
        if(distance_v[i] > 0 && distance_v[i] < min[tid]) {
          min[tid] = distance_v[i];
          next_city[tid] = i;
        }
      }

      #pragma omp master
      {
        int min_ = INFTY;
        for(int i=0;i < n_threads;i++) {
          if(min_ > min[i]) {
            next_city_ = next_city[i];
            min_ = min[i];
          }
        }
      }
    }

    tour[a][phase] = next_city_;
    visited_v[next_city_] = TRUE;

    _mm_free(distance_v);
}

/**
 * @brief Chooses for an ant the closest city as the next one
 * @param a Ant identifier
 * @param phase The construction step
 */
void choose_closest_next (const int a, const int phase)
{
    const int current_city = tour[a][phase-1];

    float * visited_v = visited[a];
    int * distance_v  = get_distance_matrix(current_city);

    __assume_aligned(visited_v, 64);
    __assume_aligned(distance_v, 64);

    int index[VECTOR_LENGTH] __attribute__((aligned(64)));
    int min[VECTOR_LENGTH] __attribute__((aligned(64)));

    for (int i = 0; i < VECTOR_LENGTH; i++) {
        index[i] = -1;
        min[i] = INFTY;
    }

    //vector_iterations = n / VECTOR_LENGTH
    //remainder_start = vector_iterations * VECTOR_LENGTH
    int j = 0;
    for (int i = 0; i < remainder_start; i += VECTOR_LENGTH) {
        #pragma ivdep
        for (int k = 0; k < VECTOR_LENGTH; k++, j++) {
            if (visited_v[j] == FALSE) {
                if (distance_v[j] < min[k]) {
                    index[k] = j;
                    min[k] = distance_v[j];
                }
           }
        }
    }

    int next_city = -1;
    int min_distance = INFTY;

    for (int i = 0; i < VECTOR_LENGTH; i++)
        if (min[i] < min_distance) {
            next_city = index[i];
            min_distance = min[i];
        }

    int k = 0;

    for (int i = remainder_start; i < n; i++, k++)
        if (visited_v[i] == FALSE)
            if (distance_v[i] < min_distance) {
                next_city = i;
                min_distance = distance_v[i];
            }

    tour[a][phase] = next_city;
    visited_v[next_city] = TRUE;

    _mm_free(distance_v);
}

/**
 * @brief Generate a tour and compute tour length
 */
int nn_tour ()
{
    ant_empty_memory(0);

    // Place the ant on a randomly chosen initial city
    int rnd = integer_random(0, n);	// Random number between 0 .. n-1
    tour[0][0] = rnd;
    visited[0][rnd] = TRUE;

    for (int step = 1; step < n; step++)
       choose_closest_next_naive_parallel(0, step);

    tour[0][n] = tour[0][0];

    return compute_tour_length(tour[0]);
}

void compute_choice_info_init ();

/**
 * @brief Initilialize variables when starting a trial
 */
void init_trial ()
{
    t_start_trial = omp_get_wtime(); // Start timing the trial

    best_tour_length = INFTY;

    pheromone_initial_trail = (float) m / (float) nn_tour();

    init_pheromone_trails();

    t_init_trial += omp_get_wtime() - t_start_trial;

    compute_choice_info_init();
}

void print_choice_info (float ** ci)
{
    for (int i = 0; i < n; i++) {
      for(int j = 0; j < n; j++) {
        printf("%g ", ci[i][j]);
      }
      printf("\n");
    }
}

void print_choice_info_meta (int ** ci)
{
    for (int i = 0; i < n; i++) {
      for(int j = 0; j < n; j++) {
        printf("%2d ", ci[i][j]);
      }
      printf("\n");
    }
}

void swap(float* a, float* b, int* aa, int* bb) {
  float t = *a;
  int tt = *aa;

  *a = *b;
  *b = t;

  *aa = *bb;
  *bb = tt;
}

int partition (float* ci, int* cim, int low, int high) {
  float pivot = ci[high];
  int i = (low - 1);  // Index of smaller element

  for (int j = low; j <= high- 1; j++) {
    // If current element is smaller than or
    // equal to pivot
    if (ci[j] >= pivot) {
      i++;    // increment index of smaller element
      swap(&ci[i], &ci[j], &cim[i], &cim[j]);
    }
  }
  swap(&ci[i + 1], &ci[high], &cim[i + 1], &cim[high]);
  return (i + 1);
}

void quickSort(float* ci, int* cim, int low, int high, int partial) {
  if (low < high) {
    /* pi is partitioning index, arr[p] is now
       at right place */
    int pi = partition(ci, cim, low, high);

    // Separately sort elements before
    // partition and after partition
    quickSort(ci, cim, low, pi - 1, partial);
    if(pi < partial) quickSort(ci, cim, pi + 1, high, partial);
  }
}

/*
 * Move zeros to the end and sort
 * non-zero elements with quicksort
 */
void sort_choice_info(float *choice_info_v, int *choice_info_meta_v, int partial_qs) {
  int last_zero = n-1;
  int count = 0;

  for (int i = 0; i < n; i++) {
    if(choice_info_v[i] == -1.0f) { /* Its the current city, send it to the last one */
      choice_info_meta_v[i] = choice_info_meta_v[n-1];
      choice_info_v[i] = 0.0f;
      choice_info_meta_v[n-1] = i;
      last_zero--;
    }
    else if(choice_info_v[i] == 0.0f) { /* Its zero, send it to the end */
      choice_info_meta_v[last_zero] = i;
      last_zero--;
    }
    else {
      choice_info_meta_v[count] = i;
      choice_info_v[count] = choice_info_v[i];
      count++;
    }
  }

  for(; count < n; count ++) {
    choice_info_v[count] = 0;
  } 

  quickSort(choice_info_v, choice_info_meta_v, 0, last_zero, partial_qs);
}

/*
 * First iteration choice info computation
 * Have no pheromone, computation does not
 * use pheromone or heuristic matrix since
 * we do not know which are the best
 * neighbours
 */
void compute_choice_info_init ()
{
    const double t_start = omp_get_wtime();

    float p_init = powf(pheromone_initial_trail, alpha);

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        int thread_id = omp_get_thread_num();
        float * choice_info_v = choice_info[i];
        int * choice_info_meta_v = choice_info_meta[i];

        float * pheromone_v = pheromone[i];

        float * choice_info_tmp_v = choice_info_tmp[thread_id];
        int * choice_info_meta_tmp_v = choice_info_meta_tmp[thread_id];

        __assume_aligned(choice_info_v, 64);
        __assume_aligned(pheromone_v, 64);

        #pragma ivdep
        for (int j = 0; j < n; j++)
          choice_info_tmp_v[j] = p_init * powf(1.0f / ((float) distance(i,j) + 0.1f), beta);

	choice_info_tmp_v[i] = 0.0f;

        // Sort choice info
        for (int i = 0; i < n; i++) choice_info_meta_tmp_v[i] = i;
        sort_choice_info(choice_info_tmp_v, choice_info_meta_tmp_v, CHUNK_L);

        // Finally, copy from N-sized structure co CHUNK-sized structure
        #pragma ivdep
        for (int j = 0; j < CHUNK_L; j++) {
          choice_info_v[j] = choice_info_tmp_v[j];
          choice_info_meta_v[j] = choice_info_meta_tmp_v[j];
        }

    }

    t_choice_info_init += omp_get_wtime() - t_start;
}

int get_city_index(int origin_city, int destination_city) {
  int * city_indexes_v = choice_info_meta[origin_city];
  int i = 0;
  while(i < CHUNK_L && city_indexes_v[i] != destination_city) i++;

  if(i == CHUNK_L) return -1;
  return i;
}

void testing_print_new(int * testing_ci_meta, int * choice_info_meta_v, int city) {
  int j;
  for(int i=0;i<32;i++) {
    j = 0;
    for(j=0;j<32;j++) {
      if(testing_ci_meta[i] == choice_info_meta_v[j]) break;
    }
    if(j == 32) printf("It %d: New city at pos %d from %d !!!\n", iteration, i, city);
  }
}

/**
 * @brief Calculates choice information for each edge
 * Used at any iteration except the first, initial iteration
 */

void update_choice_info() {
  const double t_start = omp_get_wtime();

  #pragma omp parallel for
  for (int i = 0; i < n; i++) {
    float * choice_info_v = choice_info[i];
    int * choice_info_meta_v = choice_info_meta[i];
    float * pheromone_v = pheromone[i];

    #pragma ivdep
    for (int j = 0; j < CHUNK; j++) {
      choice_info_v[j] = powf(pheromone_v[j], alpha) * powf(1.0f / ((float) distance(i, choice_info_meta_v[j]) + 0.1f), beta);
    }

  }

  t_update_choice_info += omp_get_wtime() - t_start;

}

void two_phase_fallback(int a, int phase) {
  int current_city = tour[a][phase-1];
  int city = -1;
  float min = INFTY;
  float * visited_v = visited[a];
  int * choice_info_meta_v = choice_info_meta[current_city];

  __assume_aligned(visited_v, 64);
  __assume_aligned(choice_info_meta_v, 64);

  #pragma ivdep
  for (int i = CHUNK; i < CHUNK_L; i++) {
    if(visited_v[choice_info_meta_v[i]] == FALSE) {
      float dist = distance(current_city, choice_info_meta_v[i]);

      if(min > dist) {
        city = i;
        min = dist;
      }
    }
  }

  if (min == INFTY) { /* first fallback failed, go to second fallback */
    choose_closest_next_naive(a, phase);
  }
  else { /* first fallback success */
    tour[a][phase] = choice_info_meta_v[city];
    visited_v[choice_info_meta_v[city]] = TRUE;
  }

}

void choose_next (const int a, const int thread_id, const int phase)
{
    const int current_city = tour[a][phase-1];
    float * visited_v = visited[a];
    float * choice_info_v = choice_info[current_city];
    int * choice_info_meta_v = choice_info_meta[current_city];
    float * seeds_v = seeds[thread_id];

    __assume_aligned(visited_v, 64);
    __assume_aligned(choice_info_v, 64);
    __assume_aligned(choice_info_meta_v, 64);
    __assume_aligned(seeds_v, 64);

    int city = -1;
    float max_prob = -1.0f;

    #pragma ivdep
    for (int i = 0; i < CHUNK; i++)
    {
        const float prob = choice_info_v[i] * visited_v[choice_info_meta_v[i]] * random01(&seeds_v[i]);
        if (prob > max_prob)
        {
            city = i;
            max_prob = prob;
        }
    }

    if (max_prob <= 0.0f) {
        two_phase_fallback(a, phase);
    }
    else {
        tour[a][phase] = choice_info_meta_v[city];
        visited_v[choice_info_meta_v[city]] = TRUE;
    }
}

/**
 * @brief Construct solutions for all ants
 */
void construct_solutions ()
{
    const double t_start = omp_get_wtime();

    #pragma omp parallel for
    for (int a = 0; a < m; a++)
    {
        const int thread_id = omp_get_thread_num();

        // Mark all cities as unvisited
        ant_empty_memory(a);

        // Place the ant on initial city
        int rnd = integer_random(0, n);
        tour[a][0] = rnd;
        visited[a][rnd] = TRUE;

        // Construct tours
        for (int step = 1; step < n; step++)
            choose_next(a, thread_id, step);

        tour[a][n] = tour[a][0];
        tour_length[a] = compute_tour_length(tour[a]);
    }

    t_construct_solutions += omp_get_wtime() - t_start;
}

/**
 * @brief Find the best ant of the current iteration
 * @return Index of the iteration best ant
 */
int find_best ()
{
    int min = tour_length[0];
    int best_ant = 0;

    for (int a = 1; a < m; a++)
        if (tour_length[a] < min) {
            min = tour_length[a];
            best_ant = a;
        }

    return best_ant;
}

/**
 * Same as find_best but with variable (n)
 * best ants
 */
int* find_bests(int n) {
  int* best_ants = (int *) malloc(sizeof(int)* n);

  int min = INT_MAX;
  int best_ant = 0;

  for(int i=0;i<n;i++) {
    for (int a = 0; a < m; a++) {
      if (tour_length[a] < min) {
        min = tour_length[a];
        best_ant = a;
      }
    }
    best_ants[i] = best_ant;
    tour_length[best_ant] = INT_MAX;
    min = INT_MAX;
  }

  return best_ants;
}

/**
 * @brief Save new best solution
 * @param a Ant identifier
 */
void save_new_best (const int a)
{
    best_tour_length = tour_length[a];

    for (int i = 0; i <= n; i++)
        best_tour[i] = tour[a][i];
}

/**
 * @brief Check whether a new best solution is found
 */
void update_statistics ()
{
    const double t_start = omp_get_wtime();

    const int iteration_best_ant = find_best();

    if (tour_length[iteration_best_ant] < best_tour_length)
        save_new_best(iteration_best_ant);

    t_stats += omp_get_wtime() - t_start;
}

/**
 * @brief Run tour construction stage
 * Compute choice information, construct solutions and update
 * statistics.
 */
void tour_construction ()
{
    if(iteration == 0) {
      compute_choice_info_init();
    }
    else {
      update_choice_info();
    }

    construct_solutions();

    update_statistics();
}

void local_search () {
  float f = iteration*(LS_PERCENTAGE/100);

  if(LS_PERCENTAGE > 0.0 && ceilf(f) == f) { /* run LS */
    const double t_start = omp_get_wtime();

    #pragma omp parallel for
    for(int i=0;i<m;i++)
      three_opt(tour[i], omp_get_thread_num());

    #pragma omp parallel for
    for(int i=0;i<m;i++)
      tour_length[i] = compute_tour_length(tour[i]);

    t_three_opt += omp_get_wtime() - t_start;
  }
}


/**
 * @brief Implements the pheromone trail evaporation
 * Pheromones are reduced by factor rho
 */
void pheromone_evaporation ()
{
    const double t_start = omp_get_wtime();

    const float rho_d_copy = rho_d;

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        float * pheromone_v = pheromone[i];
        for (int j = 0; j < CHUNK; j++) {
            pheromone_v[j] = rho_d_copy * pheromone_v[j];
            pheromone_v[j] = fmax(0.00000000001f, pheromone_v[j]); // IEEE Problems in choose_next
        }
    }

    t_pheromone_evaporation += omp_get_wtime() - t_start;
}

/**
 * @brief Reinforces edges used in ants' solution
 * Pheromones of arcs in ants' tour are increased.
 * Solo actualiza las feromonas la hormiga cuyo tour
 * es el mas corto
 */
void pheromone_deposit () {
  const double t_start = omp_get_wtime();

  int best_ant = find_best();
  int city_index;
  int * tour_v = tour[best_ant];
  const float d_tau = 1.0f / (float) tour_length[best_ant];

  #pragma omp parallel for
  for (int i = 0; i < n; i++) {
    int j = tour_v[i];
    int k = tour_v[i+1];

    if((city_index = get_city_index(j, k)) != -1) pheromone[j][city_index] += d_tau;
    if((city_index = get_city_index(k, j)) != -1) pheromone[k][city_index] += d_tau;
  }

  t_pheromone_deposit += omp_get_wtime() - t_start;
}



/**
 * @brief Manage global pheromone trail update
 * Pheromone trails are evaporated and pheromones are deposited according to the
 * rules defined by the Ant System algorithm.
 */

void pheromone_update ()
{
    update_statistics();

    pheromone_evaporation();

    pheromone_deposit();
}

