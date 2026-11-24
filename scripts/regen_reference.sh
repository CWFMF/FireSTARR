#!/bin/bash
scripts/mk_clean.sh
./firestarr test/output/reference 2024-06-03 58.81228184403946 -122.9117103995713 01:00 --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --wx ./test/input/10N_50651/firestarr_10N_50651_wx.csv --output_date_offsets [1,2,3] --tz -5 --raster-root ./test/input/10N_50651/ --perim ./test/input/10N_50651/10N_50651.tif -i --ascii
rm test/output/reference/*.ini
rm test/output/reference/*.log
git diff test/output/reference && echo "Reference check worked"
