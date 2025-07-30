#!/bin/bash
# find folders that don't have any perimeters in them
find -type f -name sim.sh | xargs -I {} grep -L perim {} | sed "s/\.\/\([^\/]*\)\/sim.sh/\1/g" | xargs -I {} sed -i "s/\(.*csv.*\)/\1 --perim {}.tif/g" {}/sim.sh
