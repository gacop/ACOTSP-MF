#!/bin/bash

# Download instances from TSPLIB

URL="http://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp"
INSTANCES=(
  eil51
  lin318
  rat783
  pr1002
  rl1889
  pr2392
  fl3795
  rl5934
)

for i in "${INSTANCES[@]}"
do
  wget "$URL"/"$i".tsp.gz
  gunzip "$i".tsp.gz
done
