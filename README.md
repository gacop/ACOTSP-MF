# ACOTSP-MF
This github site corresponds to the paper "ACOTSP-MF: A memory-friendly and highly scalable ACOTSP approach" published in Engineering Applications of Artificial Intelligence, by Pablo A. Martínez and José M. García. Computer Engineering Department, University of Murcia, 30100 Murcia, Spain. https://doi.org/10.1016/j.engappai.2020.104131

## Abstract:
Ant Colony Optimization (ACO) is a population-based meta-heuristic inspired by the social behavior of ants.
It is successfully applied in solving many NP-hard problems, such as the Traveling Salesman Problem (TSP).
Large-sized instances pose two memory problems to the ACOTSP algorithm: the memory size and the memory
bandwidth.
This work has focused on developing ACOTSP-MF, a new ACOTSP algorithm proposed to adequately
manage the memory issues that arise while solving large TSP instances. ACOTSP-MF uses the nearest neighbor
list, introducing a novel class of cities, the backup cities, while grouping cities into three classes: the nearest
neighbor cities, the backup cities, and the rest of the cities (the majority). ACOTSP-MF also modifies how the
base ACOTSP carries out the tour construction and pheromone update phases depending on the group to which a
city belongs. This way, ACOTSP-MF reduces both the memory requirements of its data structures (from 𝑂(𝑛 ∗ 𝑛)
to 𝑂(𝑛)), and the memory bandwidth needs (thanks to better exploitation of the memory data locality).
In this paper, we have carried out an in-depth analysis of ACOTSP-MF performance for medium and large
TSP instances, covering vectorization and scalability issues and showing its main bottlenecks. For medium-size
instances, the paper reports speedup factors of 20-500X for the rl11849 instance compared to the base
ACOTSP version. ACOTSP-MF is intended and especially adequate for large-size instances. In this context, the
paper reports excellent execution time for the Tour Construction phase, with less than 500 ms per iteration for
the earring-200k instance. Finally, a study about the solution quality of ACOTSP-MF has been included,
showing that ACOTSP-MF paired with local search offers high solution quality (within 2% of the best-known
solution).

### Maintainers:

* [Pablo Martínez][martinez_pablo]: [@Dr-Noob](https://github.com/Dr-Noob)
* [José M. García][garcia_josem]: [@jmgarcia](https://github.com/jmgarcia83)

[martinez_pablo]: https://www.um.es/gacop
[garcia_josem]: https://webs.um.es/jmgarcia

The code of this site is licenced under the GNU General Public License v3.0 GNU GPLv3
