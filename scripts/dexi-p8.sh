rm -r ~/Downloads/sv/
mkdir ~/Downloads/sv/

cp --parents array-examples/*_false-unreach-call*.i array-examples/*_true-unreach-call*.i array-industry-pattern/*_false-unreach-call*.i array-industry-pattern/*_true-unreach-call*.i reducercommutativity/*_false-unreach-call*.i reducercommutativity/*_true-unreach-call*.i array-memsafety/*_false-valid-deref*.i array-memsafety/*_true-valid-memsafety*.i ~/Downloads/sv/
cp ArraysMemSafety.prp ArraysReach.prp ArraysMemSafety.set ArraysReach.set ~/Downloads/sv/

#rm -r ~/Downloads/sv/floats
#mkdir ~/Downloads/sv/floats
cp --parents floats-cdfpl/*_false-unreach-call*.i floats-cdfpl/*_true-unreach-call*.i floats-cbmc-regression/*_true-unreach-call*.i float-benchs/*_false-unreach-call.c float-benchs/*_true-unreach-call.c floats-esbmc-regression/*_true-unreach-call*.i floats-esbmc-regression/*_false-unreach-call*.i ~/Downloads/sv/
cp Floats.prp Floats.set ~/Downloads/sv/

#rm -r ~/Downloads/sv/bitvectors
#mkdir ~/Downloads/sv/bitvectors
cp --parents bitvector/*_false-unreach-call*.i bitvector/*_true-unreach-call*.i bitvector/*_false-unreach-call*.BV.c.cil.c bitvector/*_true-unreach-call*.c.cil.c bitvector-regression/*_false-unreach-call*.i bitvector-regression/*_true-unreach-call*.i bitvector-loops/*_false-unreach-call*.i signedintegeroverflow-regression/*_false-no-overflow.c.i signedintegeroverflow-regression/*_true-no-overflow.c.i ~/Downloads/sv/
cp BitVectorsOverflows.prp BitVectorsOverflows.set BitVectorsReach.prp BitVectorsReach.set ~/Downloads/sv/

#rm -r ~/Downloads/sv/ss
#mkdir ~/Downloads/sv/ss
cp --parents ldv-linux-3.0/*_false-unreach-call*.i ldv-linux-3.0/*_true-unreach-call*.i ldv-linux-3.4-simple/*_false-unreach-call*.c ldv-linux-3.4-simple/*_true-unreach-call*.c ldv-linux-3.7.3/*_false-unreach-call*.c ldv-linux-3.7.3/*_true-unreach-call*.c ldv-commit-tester/*_false-unreach-call*.c ldv-commit-tester/*_true-unreach-call*.c ldv-consumption/*_false-unreach-call*.c ldv-consumption/*_true-unreach-call*.c ldv-linux-3.12-rc1/*_false-unreach-call*.c ldv-linux-3.12-rc1/*_true-unreach-call*.c ldv-linux-3.16-rc1/*_false-unreach-call*.c ldv-linux-3.16-rc1/*_true-unreach-call*.c ldv-validator-v0.6/*_false-unreach-call*.c ldv-validator-v0.8/*_false-unreach-call*.c ldv-linux-4.2-rc1/*_false-unreach-call*.c ldv-linux-4.2-rc1/*_true-unreach-call*.c ldv-challenges/*_false-unreach-call*.c ldv-challenges/*_true-unreach-call*.c ldv-linux-3.14/*_false-unreach-call*.c ldv-linux-3.14/*_true-unreach-call*.c ldv-linux-4.0-rc1-mav/*_false-unreach-call*.c ldv-linux-4.0-rc1-mav/*_true-unreach-call*.c busybox-1.22.0/*_false-unreach-call.i busybox-1.22.0/*_true-unreach-call.i ~/Downloads/sv/
cp BusyBox.prp BusyBox.set DeviceDriversLinux64.prp DeviceDriversLinux64.set ~/Downloads/sv/
