mkdir /home/dexi/Documents/sv/
for file in `cat ArraysMemSafety.set ArraysReach.set BitVectorsOverflows.set BitVectorsReach.set BusyBox.set DeviceDriversLinux64.set Floats.set `; do cp --parent "$file" /home/dexi/Documents/works/beagle/sv-ceagle/examples/sv/ ; done
cp ArraysReach.set ArraysReach.prp ArraysMemSafety.set ArraysMemSafety.prp BitVectorsReach.set BitVectorsReach.prp BitVectorsOverflows.set BitVectorsOverflows.prp BusyBox.set BusyBox.prp DeviceDriversLinux64.set DeviceDriversLinux64.prp Floats.set Floats.prp /home/dexi/Documents/sv/
