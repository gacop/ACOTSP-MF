/**
 * @file parse.inc
 * @brief Parse program arguments
 * @author Martinez Sanchez, Pablo A.
 * @date 28 Oct 2020
 */
 
#include <unistd.h>
#include <ctype.h>
using namespace std;

char tsp_file_name[LINE_BUF_LEN];

/**
 * @brief Show how to use the program
 * @param progname Program name
 */
void help (const char * progname)
{
    printf("\nUsage: %s [OPTION] [ARGUMENT] ...\n\n" "Options:\n", progname);
    printf("  -i\t# inputfile (TSPLIB format necessary)\n");
    printf("  -o\t# output tour to file (%s)\n", DEFAULT_TOUR_FILE);
    printf("  -r\t# number of independent trials (default: %d)\n", DEFAULT_MAX_TRIALS);
    printf("  -n\t# number of iterations (default: %d)\n", DEFAULT_MAX_ITERATIONS);
    printf("  -m\t# number of ants\n");
    printf("  -a\t# alpha (influence of pheromone trails) (default: %.1f)\n", DEFAULT_ALPHA);
    printf("  -b\t# beta (influence of heuristic information) (default: %.1f)\n", DEFAULT_BETA);
    printf("  -e\t# rho: pheromone trail evaporation (default: %.1f)\n", DEFAULT_RHO);
    printf("  -k\t# backup cities to use in the two phase fallback (default: %d)\n", DEFAULT_K);
    printf("  -l\t# percentage of iterations that LS will be executed (100%%-0.1%%) (default: %f)\n", DEFAULT_L);
    printf("  -h\t# show this help\n\n");
    exit(0);
}

/**
 * @brief Check whether a parameter value is within allowed range
 * @param value Value
 * @param min Minimum value
 * @param max Maximum value
 * @param option_name Option name
 */
void check_out_of_range (const float value, const float min, const float max, const char * option_name)
{
    if (value < min || value > max) {
        printf("Error: Option `%s' out of range (value: %f)\n", option_name, value);
        exit(1);
    }
}

/**
 * @brief Parse command line options
 * @param argc Number of arguments
 * @param argv Vector of arguments
 */
void parse_command_line (int argc, char * argv[])
{
    const char * progname = argv[0];

    int i = 0;
    int option;
    opterr = 0; // Avoids getopt's default error output

    while ((option = getopt(argc, argv, "a:b:e:hi:k:l:p:m:n:r:o")) != -1) {
        switch (option) {
        case 'a':
            alpha = atof(optarg);
            check_out_of_range(alpha, 0.0f, 20.0f, "alpha");
            break;
        case 'b':
            beta = atof(optarg);
            check_out_of_range(beta, 0.0f, 20.0f, "beta");
            break;
        case 'e':
            rho = atof(optarg);
            check_out_of_range(rho, 0.0f, 1.0f, "rho");
            break;
        case 'l':
            LS_PERCENTAGE = atof(optarg);
            check_out_of_range(LS_PERCENTAGE, 0.0f, 100.0f, "ls");
            break;
        case 'h':
            help(progname);
        case 'i':
            strncpy(tsp_file_name, optarg, strlen(optarg));
            break;
        case 'k':
            K_BACKUP = atol(optarg);
            check_out_of_range(K_BACKUP, 0, INFTY, "k backup");
            break;
        case 'm':
            m = atol(optarg);
            check_out_of_range(m, 1, INFTY, "m");
            break;
        case 'n':
            max_iterations = atol(optarg);
            check_out_of_range(max_iterations, 1, INFTY, "max_iterations");
            break;
        case 'r':
            max_trials = atol(optarg);
            check_out_of_range(max_trials, 1, INFTY, "max_trials");
            break;
        case 'o':
            write_tour_file = true;
            break;
        case '?':
            if ((optopt == 'a') || (optopt == 'b') || (optopt == 'e') || (optopt == 'i')
               || (optopt == 'm') || (optopt == 'n') || (optopt == 'r'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,"Unknown option character `\\x%x'.\n", optopt);
        default:
            help(progname);
        }
        i++;
    }

    if (i < 1) {
        printf("\nNo options are specified\n");
        printf("Try `%s -h' for more information.\n\n", progname);
        exit(1);
    }
}

/**
 * @brief Parse and read instance file
 * Instance files have to be in TSPLIB format, otherwise procedure fails.
 */
void read_etsp ()
{
    FILE * tsp_file;
    char buf[LINE_BUF_LEN];

    if ((tsp_file = fopen(tsp_file_name, "r")) == NULL) {
        printf("No instance file specified, abort\n");
        exit(1);
    }

    fscanf(tsp_file, "%s", buf);

    while (strcmp("NODE_COORD_SECTION", buf)) {
        if (!strcmp("NAME", buf)) {
            fscanf(tsp_file, "%s", buf);
            fscanf(tsp_file, "%s", buf);
            strcpy(name, buf);
        }
        else if (!strcmp("NAME:", buf)) {
            fscanf(tsp_file, "%s", buf);
            strcpy(name, buf);
        }
        else if (!strcmp("COMMENT", buf) || !strcmp("COMMENT:", buf))
            fgets(buf, LINE_BUF_LEN, tsp_file);
        else if (!strcmp("TYPE", buf)) {
            fscanf(tsp_file, "%s", buf);
            fscanf(tsp_file, "%s", buf);
            if (strcmp("TSP", buf)) {
                fprintf(stderr,"\n Not a TSP instance in TSPLIB format !!\n");
                exit(1);
            }
        }
        else if (!strcmp("TYPE:", buf)) {
            fscanf(tsp_file, "%s", buf);
            if (strcmp("TSP", buf)) {
                fprintf(stderr,"\n Not a TSP instance in TSPLIB format !!\n");
                exit(1);
            }
        }
        else if (!strcmp("DIMENSION", buf)) {
            fscanf(tsp_file, "%s", buf);
            fscanf(tsp_file, "%d", &n);
            check_out_of_range(n, 2, INFTY, "n");
        }
        else if (!strcmp("DIMENSION:", buf)) {
            fscanf(tsp_file, "%d", &n);
            check_out_of_range(n, 2, INFTY, "n");
        }
        else if (!strcmp("DISPLAY_DATA_TYPE", buf) || !strcmp("DISPLAY_DATA_TYPE:", buf))
            fgets(buf, LINE_BUF_LEN, tsp_file);
        else if (!strcmp("EDGE_WEIGHT_TYPE", buf)) {
            // Set pointer to appropriate distance function; has to be one of 
            // EUC_2D, CEIL_2D, GEO, ATT or EUC_TOROID. Everything else fails
            fscanf(tsp_file, "%s", buf);
            fscanf(tsp_file, "%s", buf);
            if (!strcmp("EUC_2D", buf))
                distance = round_distance;
            else if (!strcmp("CEIL_2D", buf))
                distance = ceil_distance;
            else if (!strcmp("GEO", buf))
                distance = geo_distance;
            else if (!strcmp("ATT", buf))
                distance = att_distance;
            else if (!strcmp("EUC_TOROID", buf))
                distance = toroid_distance;
            else {
                fprintf(stderr,"EDGE_WEIGHT_TYPE %s not implemented\n", buf);
                exit(1);
            }
        }
        else if (!strcmp("EDGE_WEIGHT_TYPE:", buf)) {
            fscanf(tsp_file, "%s", buf);
            if (!strcmp("EUC_2D", buf))
                distance = round_distance;
            else if (!strcmp("CEIL_2D", buf))
                distance = ceil_distance;
            else if (!strcmp("GEO", buf))
                distance = geo_distance;
            else if (!strcmp("ATT", buf))
                distance = att_distance;
            else if (!strcmp("EUC_TOROID", buf))
                distance = toroid_distance;
            else {
                fprintf(stderr,"EDGE_WEIGHT_TYPE %s not implemented\n", buf);
                exit(1);
            }
        }
        buf[0] = 0;
        fscanf(tsp_file,"%s", buf);
    }

    if (strcmp("NODE_COORD_SECTION", buf)) {
        printf("\n\nSome error ocurred finding start of coordinates from tsp file !!\n");
        exit(1);
    }

    city_coordinates = (point *) _mm_malloc(sizeof(struct point) * n, CACHE_LINE);
    int j;

    for (int i = 0; i < n; i++)
        fscanf(tsp_file, "%d %f %f", &j, &city_coordinates[i].x, &city_coordinates[i].y);
}

