/**
 * @file acotsp.cpp
 * @brief Main file containing ACOTSP main algorithm
 * @author Martinez Sanchez, Pablo A.
 * @date 28 Oct 2020
 */

#include "common.hpp"

int iteration;

int n;                  // Number of cities
int m;                  // Number of ants

float rho;              // Pheromone evaporation rate
float alpha;            // Importance of the pheromone trail
float beta;             // Importance of the heuristic information

int max_trials;         // Number of independent trials
int max_iterations;     // Number of iterations

int threads;            // Number of threads

float LS_PERCENTAGE;
int K_BACKUP;
int K_PAD;              // Number of 64-byte blocks for prefetch padding
                        // Padding to avoid sharing a cache line between threads

bool write_tour_file;

#include "utilities.inc"
#include "TSP.inc"
#include "ants.inc"
#include "InOut.inc"
#include "ls.inc"

/**
 * @brief Main control for running the Ant System algorithm
 * This function controls the run of "max_trials" independent trials.
 */
int main(int argc, char * argv[])
{
    init_program(argc, argv);

    for (int trial = 1; trial <= max_trials; trial++) {
        init_trial();

        for (iteration = 0; iteration < max_iterations; iteration++) {
            tour_construction();
  
            local_search();

            pheromone_update();
        }

        exit_trial(trial);

    }

    exit_program();
}

