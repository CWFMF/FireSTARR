#/bin/bash
find -type f -name log.txt | xargs -I {} tail -n100 {} | sed -n '/Ran [0-9]* simulations/{s/.*Ran \([0-9]*\) simulations.*/\1/g;p;}'