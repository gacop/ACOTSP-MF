#!/bin/bash

# Download instances from TSPLIB, MATH.UWATERLOO and ARTSP

URL_TSPLIB="http://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp"
URL_ARTSP="http://www.math.uwaterloo.ca/tsp/data/"

INSTANCES_TSPLIB=(
  eil51
  lin318
  rat783
  pr1002
  rl1889
  pr2392
  fl3795
  rl5934
)

INSTANCES_ARTSP=(
  ml/mona-lisa100K  
  art/vangogh120K
  art/venus140K
  art/pareja160K
  art/courbet180K
  art/earring200K
)

for i in "${INSTANCES_TSPLIB[@]}"
do
  wget "$URL_TSPLIB"/"$i".tsp.gz
  gunzip "$i".tsp.gz
done

wget "http://www.math.uwaterloo.ca/tsp/vlsi/pla7397.tsp"
wget "http://www.math.uwaterloo.ca/tsp/world/vm22775.tsp"

for i in "${INSTANCES_ARTSP[@]}"
do
  wget "$URL_ARTSP"/"$i".tsp -P artsp/
done


