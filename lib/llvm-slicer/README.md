###LLVM slicer
This is a static slicer based on the Mark Weiser's algorithm in [1]. It is
augmented to perform an inter-procedural analysis. 

### How To Build
  - `mkdir LLVMSlicer.obj`
  - `cd !$`
  - `CC=gcc CXX=g++ ../LLVMSlicer/configure --with-llvmsrc=<> --with-llvmobj=<>
  - `CXXFLAGS+=-std=c++11 make ENABLE_OPTIMIZED=1`

###How To Run
  - To get a mapping of binary to llvm-ir. This pass numbers the llvm-irs in a specific function (-mapping-function) and for each of them specifies the info of the corresponding binary mneumonics. This analysis pass used the metadata add by mcsema to infer this mapping.
    * Eg. `opt -load=LVMSlicer.so  -srcline-mapping -mapping-function=main -mapping-output=mapping.txt  test/example1.ll -o /tmp/example1.analysed.bc` 
    * Eg. `opt -load=LVMSlicer.so  -srcline-mapping -mapping-function=sub_8048420 -mapping-output=mapping.txt  test/example2.ll -o /tmp/example2.analysed.bc` 
      * -mapping-function: The function we want to focus on.
      * -mapping-output: The Output file with mapping. If not specifed the output will be dumped in stdout
  -  To obtain backward slicing using a function, line number of llvm-ir and variable name as slicing criterion. 
      * The line number (specified as -criterion-line) can be chosen using the above mapping, as it specifies the line numbers of the llvm-ir.
      * Eg. `opt -load=LLVMSlicer.so  -criterion-function=main -criterion-line=43 -criterion-variable=product  -slice-inter  test/example1.ll  -o /tmp/example1.sliced.bc`
      * Eg. `opt -load=LLVMSlicer.so  -criterion-function=sub_8048420 -criterion-line=256 -criterion-variable=ESP_val  -slice-inter  test/example2.ll  -o /tmp/example2.sliced.bc`



###Acknowledgement
- This worked is borrowed from Jiri Slaby <jirislaby@gmail.com>
  * https://github.com/jirislaby/LLVMSlicer
