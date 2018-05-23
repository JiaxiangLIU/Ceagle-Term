#!/bin/bash

# many thanks to Lifan Su!
# 20160827 2213

if [ $# -lt 3 ] ;then
	echo "Usage:./dd_generate_function_call.sh {output} {input}..."
	exit
fi	

output=$1
shift 1

for i in "$@" ; do
	mkdir -p "$(dirname "${output}/${i}.txt")"
	sv-ceagle --compiler=/home/dexi/Documents/works/llvm/llvm-3.7.0.src/build/bin/clang --generate-function-call-of=__VERIFIER_error "$i" > "${output}/${i}".txt
done

