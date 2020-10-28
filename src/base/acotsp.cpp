/**
 * @file acotsp.cpp
 * @brief Main routines and control for the Ant System algorithm
 * @author Victoriano Monstesinos Cánovas
 * @date 23 Oct 2017
 */

#include "common.hpp"

integer n;		// Number of cities
integer m;		// Number of ants

real rho;		// Pheromone evaporation rate
real alpha;		// Importance of the pheromone trail
real beta;		// Importance of the heuristic information

integer max_trials;	// Number of independent trials
integer max_iterations;	// Number of iterations

integer threads;	// Number of threads

integer K_PAD;          // Number of 64-byte blocks for prefetch padding
                        // Padding to avoid sharing a cache line between threads

#include "utilities.inc"
#include "TSP.inc"
#include "ants.inc"
#include "InOut.inc"


/**
 * @brief Main control for running the Ant System algorithm
 * This function controls the run of "max_trials" independent trials.
 */
int main(int argc, char * argv[])
{
    init_program(argc, argv);

    for (integer trial = 1; trial <= max_trials; trial++) {

        init_trial();

        for (integer iteration = 0; iteration < max_iterations; iteration++) {

            tour_construction();

            //pheromone_update();

        }

        exit_trial(trial);

    }

    exit_program();

    return 0;
}

