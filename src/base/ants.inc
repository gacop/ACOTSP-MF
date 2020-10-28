/**
 * @file ants.inc
 * @brief Implementation of procedures for ants' behaviour
 * @author Victoriano Monstesinos Cánovas
 * @date 16 Jan 2018
 */

/* Ant Colony */
integer ** tour;		// Ant tours
//integer ** visited;		// Stores the information of which cities are visited
real ** visited;		// Stores the information of which cities are visited
integer * tour_length;	// Length of the tours
/* Ant Colony */

integer * best_tour;		// Best tour found
integer best_tour_length;	// Length of the best tour found

real ** pheromone;		// Pheromone matrix
real ** choice_info;		// Choice information matrix

//real ** prob_of_selection;	// Stores the selection probabilities

real rho_d;			// 1 - rho

#define SKIP_TRIALS	3	// Skip first trials (warm-up)

/* Time measurement */
double t_start_trial;
double t_init_trial;
double t_choice_info;
double t_construct_solutions;
double t_stats;
double t_pheromone_evaporation;
double t_pheromone_deposit;
double * trial_time;
/* Time measurement */

/* Solution quality measurement */
integer * trial_sol;            // Best solution of each trial
/* Solution quality measurement */

integer vector_iterations;
integer vector_remainder;
integer remainder_start;


/**
 * @brief Allocate the memory for the ant colony and the best tour found
 * Each "tour" needs to be allocated for length n+1, since the first city of
 * a tour (at position 0) is repeated at position n. This is done to make the
 * computation of the tour length easier.
 */
void allocate_memory ()
{
    tour = generate_integer_matrix(m, n+1, false);

    //visited = generate_integer_matrix(m, n, false);
    //visited = generate_integer_matrix(threads, n, true);
    visited = generate_real_matrix(m, n, false);

    tour_length = (integer *) _mm_malloc(sizeof(integer) * m, CACHE_LINE);

    best_tour = (integer *) _mm_malloc(sizeof(integer) * (n+1), CACHE_LINE);

    //prob_of_selection = generate_real_matrix(threads, n, true);

    //seeds = generate_integer_matrix(threads, n, true);
    seeds = generate_real_matrix(threads, n, true);

    pheromone = generate_real_matrix(n, n, false);

    choice_info = generate_real_matrix(n, n, false);

    trial_time = (double *) _mm_malloc(sizeof(double) * (max_trials-SKIP_TRIALS), CACHE_LINE);

    trial_sol = (integer *) _mm_malloc(sizeof(integer) * (max_trials-SKIP_TRIALS), CACHE_LINE);
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
    //_mm_free(prob_of_selection);
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
void ant_empty_memory (const integer a)
{
    const integer n_copy = n;

    //integer * visited_v = visited[a];
    real * visited_v = visited[a];

    __assume_aligned(visited_v, 64);

    for (integer i = 0; i < n_copy; i++)
        visited_v[i] = FALSE;
}

/**
 * @brief Chooses for an ant the closest city as the next one 
 * @param a Ant identifier
 * @param phase The construction step
 */
void choose_closest_next (const integer a, const integer phase)
{
    const integer current_city = tour[a][phase-1];

    //integer * visited_v = visited[a];
    real * visited_v = visited[a];
    integer * distance_v = distance_matrix[current_city];

    __assume_aligned(visited_v, 64);
    __assume_aligned(distance_v, 64);

/*    for (integer city = 0; city < n; city++)
        if (visited_v[city] == FALSE)
            if (distance_v[city] < min_distance) {
                next_city = city;
                min_distance = distance_v[city];
            }*/

/*    for (integer city = 0; city < n; city++)
        if (visited_v[city] > 0.0f)
            if (distance_v[city] < min_distance) {
                next_city = city;
                min_distance = distance_v[city];
            }*/

    // TODO Comprobar si esto mejora el tiempo
    // Se puede hacer automáticamente sin strip-mining?
    // Si se puede hacer en choose_next, aquí también

    integer index[VECTOR_LENGTH] __attribute__((aligned(64)));
    integer min[VECTOR_LENGTH] __attribute__((aligned(64)));

    for (integer i = 0; i < VECTOR_LENGTH; i++) {
        index[i] = -1;
        min[i] = INFTY;
    }

    integer j = 0;
    for (integer i = 0; i < remainder_start; i += VECTOR_LENGTH) {
        #pragma ivdep
        for (integer k = 0; k < VECTOR_LENGTH; k++, j++)
            if (visited_v[j] > 0.0f)
                if (distance_v[j] < min[k]) {
                    index[k] = j;
                    min[k] = distance_v[j];
                }
    }

    integer next_city = -1;
    integer min_distance = INFTY;

    for (integer i = 0; i < VECTOR_LENGTH; i++)
        if (min[i] < min_distance) {
            next_city = index[i];
            min_distance = min[i];
        }

    integer k = 0;

    for (integer i = remainder_start; i < n; i++, k++)
        if (visited_v[i] > 0.0f)
            if (distance_v[i] < min_distance) {
                next_city = i;
                min_distance = distance_v[i];
            }

    tour[a][phase] = next_city;
    visited_v[next_city] = TRUE;
}

/**
 * @brief Chooses for an ant the closest city as the next one 
 * @param a Ant identifier
 * @param visited_v Visited vector
 * @param phase The construction step
 */
/*void choose_closest_next (const integer a, const integer thread_id, const integer phase)
{
    real * visited_v = visited[thread_id];

    const integer current_city = tour[a][phase-1];
    integer next_city = n;
    integer min_distance = INFTY;

    for (integer city = 0; city < n; city++)
        if (visited_v[city] == FALSE)
            if (distance_matrix[current_city][city] < min_distance) {
                next_city = city;
                min_distance = distance_matrix[current_city][city];
            }

    tour[a][phase] = next_city;
    visited_v[next_city] = TRUE;
}*/

/**
 * @brief Generate a tour and compute tour length
 */
integer nn_tour ()
{
    ant_empty_memory(0);

    // Place the ant on a randomly chosen initial city
    integer rnd = integer_random(0, n);	// Random number between 0 .. n-1
    tour[0][0] = rnd;
    visited[0][rnd] = TRUE;
 
    for (integer step = 1; step < n; step++)
        choose_closest_next(0, step);
        //choose_closest_next(0, 0, step);

    tour[0][n] = tour[0][0];

    return compute_tour_length(tour[0]);
}

/**
 * @brief Initialize pheromone trails
 * Pheromone matrix is reinitialized.
 */
void init_pheromone_trails ()
{
    const real initial_trail = (real) m / (real) nn_tour();

    for (integer i = 0; i < n; i++) {
        real * pheromone_v = pheromone[i];

        __assume_aligned(pheromone_v, 64);

        for (integer j = 0; j < n; j++)
            pheromone_v[j] = initial_trail;
    }
}

/**
 * @brief Initilialize variables when starting a trial
 */
void init_trial ()
{
    t_start_trial = omp_get_wtime(); // Start timing the trial

    best_tour_length = INFTY;

    init_pheromone_trails();

    t_init_trial += omp_get_wtime() - t_start_trial;
}

/**
 * @brief Calculates choice information for each edge
 */
void compute_choice_info ()
{
    const double t_start = omp_get_wtime();

    #pragma omp parallel for
    for (integer i = 0; i < n; i++) {
        real * choice_info_v = choice_info[i];
        real * pheromone_v = pheromone[i];
        real * heuristic_v = heuristic[i];

        __assume_aligned(choice_info_v, 64);
        __assume_aligned(pheromone_v, 64);
        __assume_aligned(heuristic_v, 64);

        //#pragma novector
        #pragma ivdep
        for (integer j = 0; j < n; j++)
            choice_info_v[j] = powf(pheromone_v[j], alpha) * heuristic_v[j];
    }

    t_choice_info += omp_get_wtime() - t_start;
}

/**
 * @brief Choose for an ant a next city
 * Choose for an ant probabilistically a next city among all unvisited cities
 * in the current city's candidate list. If this is not possible, choose the
 * best next.
 * @param a Ant identifier
 * @param phase The construction step
 */
void choose_next (const integer a, const integer thread_id, const integer phase)
{
    const integer current_city = tour[a][phase-1];

    //real * prob = prob_of_selection[thread_id];

    //integer * visited_v = visited[a];
    real * visited_v = visited[a];

    real * choice_info_v = choice_info[current_city];

    //integer * seeds_v = seeds[thread_id];
    real * seeds_v = seeds[thread_id];

   //__assume_aligned(prob, 64);
    __assume_aligned(visited_v, 64);
    __assume_aligned(choice_info_v, 64);
    __assume_aligned(seeds_v, 64);

    integer city = -1;
    real max_prob = -1.0f;

    real s[n];

    //#pragma novector
    #pragma ivdep
    for (integer i = 0; i < n; i++)
    {
        const real prob = choice_info_v[i] * visited_v[i] * random01(&seeds_v[i]);
        if (prob > max_prob)
        {
            city = i;
            max_prob = prob;
        }
    }

    if (max_prob <= 0.0f)
        choose_closest_next(a, phase);
    else
    {
        tour[a][phase] = city;
        visited_v[city] = TRUE;
    }
}

/**
 * @brief Construct solutions for all ants
 */
void construct_solutions ()
{
    const double t_start = omp_get_wtime();

    #pragma omp parallel for
    for (integer a = 0; a < m; a++)
    {
        const integer thread_id = omp_get_thread_num();

        // Mark all cities as unvisited
        ant_empty_memory(a);
        //ant_empty_memory(thread_id);

        // Place the ant on initial city
        tour[a][0] = a;
        visited[a][a] = TRUE;
        //visited[thread_id][a] = TRUE;

        // Construct tours
        for (integer step = 1; step < n; step++)
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
integer find_best ()
{
    integer min = tour_length[0];
    integer best_ant = 0;

    for (integer a = 1; a < m; a++)
        if (tour_length[a] < min) {
            min = tour_length[a];
            best_ant = a;
        }

    return best_ant;
}

/**
 * @brief Save new best solution
 * @param a Ant identifier
 */
void save_new_best (const integer a)
{
    best_tour_length = tour_length[a];

    for (integer i = 0; i <= n; i++)
        best_tour[i] = tour[a][i];
}

/**
 * @brief Check whether a new best solution is found
 */
void update_statistics ()
{
    const double t_start = omp_get_wtime();

    const integer iteration_best_ant = find_best();

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
    compute_choice_info();

    construct_solutions();

    update_statistics();
}

/**
 * @brief Implements the pheromone trail evaporation
 * Pheromones are reduced by factor rho
 */
void pheromone_evaporation ()
{
    const double t_start = omp_get_wtime();

    const real rho_d_copy = rho_d;

    #pragma omp parallel for
    for (integer i = 0; i < n; i++) {
        real * pheromone_v = pheromone[i];
        for (integer j = 0; j < n; j++)
            pheromone_v[j] = rho_d_copy * pheromone_v[j];
    }

    t_pheromone_evaporation += omp_get_wtime() - t_start;
}

/**
 * @brief Reinforces edges used in ants' solution
 * Pheromones of arcs in ants' tour are increased.
 */
void pheromone_deposit ()
{
    const double t_start = omp_get_wtime();

    for (integer a = 0; a < m; a++) {
        const real d_tau = 1.0f / (real) tour_length[a];

        for (integer i = 0; i < n; i++) {
            integer j = tour[a][i];
            integer h = tour[a][i+1];
            pheromone[j][h] += d_tau;
            pheromone[h][j] = pheromone[j][h];
        }
    }

/*    #pragma omp parallel
    {
    #pragma omp for
    for (integer a = 0; a < m; a++) {
        const real d_tau = 1.0f / (real) tour_length[a];

        for (integer i = 0; i < n; i++) {
            integer j = tour[a][i];
            integer h = tour[a][i+1];
            if (j < h) {
                integer aux = h;
                h = j;
                j = aux;
            }
            #pragma omp atomic
            pheromone[j][h] += d_tau;
        }
    }

    #pragma omp for
    for (integer i = 0; i < n; i++)
        for (integer j = 0; j < i; j++)
            pheromone[j][i] = pheromone[i][j];
    }*/

    t_pheromone_deposit += omp_get_wtime() - t_start;
}

/**
 * @brief Manage global pheromone trail update
 * Pheromone trails are evaporated and pheromones are deposited according to the
 * rules defined by the Ant System algorithm.
 */
void pheromone_update ()
{
    pheromone_evaporation();

    pheromone_deposit();
}


