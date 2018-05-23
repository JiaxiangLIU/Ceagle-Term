#!/bin/bash

desination=/home/dexi/Documents/sv/

count()
{
	name=$1
	cnt=0
	while read line ; do
		cnt=$(( cnt + line ))
	done < <( for file in $(cat ${name}) ; do ls "$file" | wc -l ; done )
	printf "${name}: %d\n" "${cnt}"
}

cd ${desination}
count ArraysMemSafety.set
count ArraysReach.set
count BitVectorsOverflows.set
count BitVectorsReach.set
count BusyBox.set
count DeviceDriversLinux64.set
count Floats.set

