#!/bin/bash

source=/home/dexi/Documents/sv-benchmarks/c/
desination=/home/dexi/Documents/sv/

mkdir ${desination}
cd ${source}
for file in `cat ArraysMemSafety.set ArraysReach.set BitVectorsOverflows.set BitVectorsReach.set BusyBox.set DeviceDriversLinux64.set Floats.set `; do cp --parent "$file" ${desination} ; done
cp ArraysReach.set ArraysReach.prp ArraysMemSafety.set ArraysMemSafety.prp BitVectorsReach.set BitVectorsReach.prp BitVectorsOverflows.set BitVectorsOverflows.prp BusyBox.set BusyBox.prp DeviceDriversLinux64.set DeviceDriversLinux64.prp Floats.set Floats.prp ${desination}
