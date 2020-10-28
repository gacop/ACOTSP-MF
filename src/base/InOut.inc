/**
 * @file InOut.inc
 * @brief Mainly input / output / statistic routines
 * @author Victoriano Monstesinos Cánovas
 * @date 20 Dec 2017
 */

#define VERSION		"\nAnt System for the TSP\n"

/* Default parameter settings */
#define DEFAULT_MAX_TRIALS      13
#define DEFAULT_MAX_ITERATIONS  100
#define DEFAULT_ALPHA           1.0f
#define DEFAULT_BETA            5.0f	/* 2 to 5*/
#define DEFAULT_RHO             0.5f
#define DEFAULT_K_PAD           0

#define CHECK_DEFAULT(x,y)	(x == y ? "  *" : "")

#define H_SHORT_LINE    "---------------------------------------\n"

#include "parse.inc"


/**
 * @brief Set default parameter settings
 */
void set_default_parameters()
{
    m = -1;
    alpha = DEFAULT_ALPHA;
    beta = DEFAULT_BETA;
    rho = DEFAULT_RHO;
    max_trials = DEFAULT_MAX_TRIALS;
    max_iterations = DEFAULT_MAX_ITERATIONS;
    K_PAD = DEFAULT_K_PAD;
}

/**
 * @brief Writes chosen parameter settings in standard output and in report files
 */
void write_params ()
{
    const char * format_s = " %19s %15s\n";
    const char * format_d = " %19s %15d%s\n";
    const char * format_f1 = " %19s %15.1f%s\n";
    const char * format_f2 = " %19s %15.2f%s\n";

    printf("\n" H_SHORT_LINE);
    printf("          PARAMETER SETTINGS\n");
    printf(H_SHORT_LINE);
    printf(format_s, "Instance:          ", name);
    printf(format_d, "Cities:            ", n, "");
    printf(format_d, "Tries:             ", max_trials, CHECK_DEFAULT(max_trials, DEFAULT_MAX_TRIALS));
    printf(format_d, "Iterations:        ", max_iterations, CHECK_DEFAULT(max_iterations, DEFAULT_MAX_ITERATIONS));
    printf(format_d, "Ants:              ", m, CHECK_DEFAULT(m, n));
    printf(format_f1, "Alpha:             ", alpha, CHECK_DEFAULT(alpha, DEFAULT_ALPHA));
    printf(format_f1, "Beta:              ", beta, CHECK_DEFAULT(beta, DEFAULT_BETA));
    printf(format_f2, "Rho:               ", rho, CHECK_DEFAULT(rho, DEFAULT_RHO));
    printf(format_s, "Selection Function:", "I-Roulette v3");
    printf(format_d, "Threads:           ", threads, "");
    const char * affinity = getenv("KMP_AFFINITY");
    if (affinity != NULL)
        printf(" %9s %25s\n", "Affinity:", affinity);
    const char * hw_subset = getenv("KMP_HW_SUBSET");
    if (hw_subset != NULL)
        printf(format_s, "HW subset:         ", hw_subset);
    printf(format_d, "Prefetch padding:  ", K_PAD, "");
    printf(H_SHORT_LINE);
    printf(" * - default value\n\n");
}

/**
 * @brief Initialize the program
 * @param argc Number of arguments
 * @param argv Program arguments, needed for parsing commandline
 */
void init_program (int argc, char * argv[])
{
    const double t_start_initialization = omp_get_wtime();

    set_default_parameters();

    parse_command_line(argc, argv);

    read_etsp();

    if (m < 0 || m > n)
        m = n;

    rho_d = 1.0f - rho;

    threads = omp_get_max_threads();

    compute_distance_matrix();

    _mm_free(node);

    compute_heuristic_information();

    vector_iterations = n / VECTOR_LENGTH;
    vector_remainder = n % VECTOR_LENGTH;
    remainder_start = vector_iterations * VECTOR_LENGTH;

    allocate_memory();

    srand(time(NULL));

    for (integer i = 0; i < threads; i++)
        for (integer j = 0; j < n; j++)
            seeds[i][j] = rand();

    vslNewStream(&rnStream, VSL_BRNG_MT19937, time(NULL));

    t_init_trial = 0.0;
    t_choice_info = 0.0;
    t_construct_solutions = 0.0;
    t_stats = 0.0;
    t_pheromone_evaporation = 0.0;
    t_pheromone_deposit = 0.0;

    printf(VERSION);

    write_params();

    printf("Initialization took %.3f seconds\n", omp_get_wtime() - t_start_initialization);

    printf("\n%5s %13s %18s\n", "Try", "Best", "Time (s)");

    printf(H_SHORT_LINE);
}

/**
 * @brief Print a tour
 * @param t Pointer to a tour
 */
void print_tour (integer * t)
{
    for (integer i = 0; i <= n; i++) {
        if (!(i % 10))
            printf("\n");
        printf("%5d ", t[i]);
    }
    printf("\n");
    printf("\nTour length = %d\n\n", compute_tour_length(t));
}

/**
 * @brief Check whether a tour can be feasible
 * @param t Pointer to a tour
 */
void check_tour (integer * t)
{
    integer sum = 0;

    for (integer i = 0; i < n; i++)
        sum += t[i];

    if (sum != (n-1) * n/2) {
        printf("Tour must be flawed !!\n");
        print_tour(t);
        exit(1);
    }
}

/**
 * @brief Save some statistical information on a trial once it finishes
 * @param trial Trial number
 */
void exit_trial (const integer trial)
{
    const double t_trial = omp_get_wtime() - t_start_trial; // End timing the trial

    if (trial == SKIP_TRIALS) {
        t_init_trial = 0.0;
        t_choice_info = 0.0;
        t_construct_solutions = 0.0;
        t_stats = 0.0;
        t_pheromone_evaporation = 0.0;
        t_pheromone_deposit = 0.0;
    }
    else if (trial > SKIP_TRIALS) {
        trial_time[trial-1-SKIP_TRIALS] = t_trial;
        trial_sol[trial-1-SKIP_TRIALS] = best_tour_length;
    }

    printf("%5d %13d %14.2f %s\n", trial, best_tour_length, t_trial, (trial <= SKIP_TRIALS ? "*" : ""));

    check_tour(best_tour);
}

/**
 * @brief Save some final statistical information and free memory
 */
void exit_program ()
{
    const integer measured_trials = (max_trials - SKIP_TRIALS);
    const double inv_measured_trials = 1.0 / (max_trials - SKIP_TRIALS);

    const double time_mean = dmean_v(trial_time, measured_trials);
    const double time_std = dstd_v(trial_time, measured_trials);
    const integer sol_mean = (integer) mean_v(trial_sol, measured_trials);
    const real sol_std = std_v(trial_sol, measured_trials);

    printf(H_SHORT_LINE);
    printf("%s %12d %14.2f s\n", " Avg.:", sol_mean, time_mean);
    printf(H_SHORT_LINE);
    printf("%s %14.1f %12.2f s\n", "  Std:", sol_std, time_std);
    printf(H_SHORT_LINE);
    printf(" * - warm-up, not included in average\n\n");

    const double t_tour_construction = t_choice_info + t_construct_solutions + t_stats;
    const double t_pheromone_update = t_pheromone_evaporation + t_pheromone_deposit;

    const double time_sum = t_init_trial + t_tour_construction + t_pheromone_update;

    const double inv_total = 100.0 / time_sum;

    // Time breakdown information
    const char * format = " %17s %15.2f %13.2f\n";
    const char * line_lvl_1 = "-----------------------------------------------------\n";
    const char * line_lvl_2 = " -----------------------------------------------\n";

    printf(" %17s %19s %13s\n", "Procedure        ", "Time (s)", "Fraction (%)");
    printf("%s", line_lvl_1);
    printf(format, "Init Trial       ", t_init_trial * inv_measured_trials, t_init_trial * inv_total);
    printf(format, "Tour Construction", t_tour_construction * inv_measured_trials, t_tour_construction * inv_total);
    printf("%s", line_lvl_2);
    printf(format, "  Choice Info    ", t_choice_info * inv_measured_trials, t_choice_info * inv_total);
    printf(format, "  Construct Sol. ", t_construct_solutions * inv_measured_trials, t_construct_solutions * inv_total);
    printf(format, "  Update Stats   ", t_stats * inv_measured_trials, t_stats * inv_total);
    printf("%s", line_lvl_2);
    printf(format, "Pheromone Update ", t_pheromone_update * inv_measured_trials, t_pheromone_update * inv_total);
    printf("%s", line_lvl_2);
    printf(format, "  Evaporation    ", t_pheromone_evaporation * inv_measured_trials, t_pheromone_evaporation * inv_total);
    printf(format, "  Deposit        ", t_pheromone_deposit * inv_measured_trials, t_pheromone_deposit * inv_total);
    printf("%s", line_lvl_1);
    printf(" %17s %15.2f s %11.2f %\n", "Total            ", time_sum * inv_measured_trials, time_sum * inv_total);
    printf("%s", line_lvl_1);

    vslDeleteStream(&rnStream);

    _mm_free(distance_matrix);
    _mm_free(heuristic);
    free_memory();
}

