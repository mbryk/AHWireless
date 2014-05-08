#!/bin/bash

for (( i = 0; i < 4; i++ ))
do
	gnome-terminal -x bash -c "./waf --run \"scratch/AdhocSetup --pwr=10 --DIST_LIMIT_SQRT=100\" ; bash"
done
