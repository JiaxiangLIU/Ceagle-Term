#/bin/sh

# 由于benchexec没有提供可用的批量统计witness得分的功能
# 因此只能先用脚本来统计witness得分情况，并调试witness做错的文件，仅供参考

FILE_NAME=0
FILE_STATUS=1
CPU_TIME=2
WALL_TIME=3
EXPECTED_STATUS=4
ACTUAL_STATUS=5
BENCH_DIR="/home/sv/sv-ceagle/examples/sv"
CPACHECKER_DIR="/home/sv/cpachecker"
WITNESS_RESULT_DIR="/home/sv/sv-ceagle/scripts"

# Usage: nohup witness_statistics.sh /home/dexi/Documents/works/beagle/sv-ceagle/script/20161208_dd_all.csv
main(){
	# 切换到CPA目录下验证
	cd $CPACHECKER_DIR
	date=`date +%Y%m%d`
	src_name=$1
	tmp_src_file_name=${src_name%.*}
	src_file_name=${tmp_src_file_name##*/}
	witness_result_file=${WITNESS_RESULT_DIR}/${src_file_name}_witness_result.csv
	echo "sv/,status,cputime (s),walltime (s),Should be,Compare,Witness Compare" >> $witness_result_file
    for line in $(awk -F , '{ if($6=="OK") print $0 }' $src_name);
    do
        IFS=','
        file_infos=(${line})
        source_file_name=${BENCH_DIR}/${file_infos[FILE_NAME]}
		witness_file_name=${source_file_name}.xml
        expected_status=${file_infos[EXPECTED_STATUS]}
        unset IFS
        cmd="scripts/cpa.sh -config ${CPACHECKER_DIR}/config/components/sv-comp16--witness-validation.properties -spec ${witness_file_name} -spec ${BENCH_DIR}/DeviceDriversLinux64.prp ${source_file_name}" 
        #echo "file_name:$file_name status: $expected_status"
        echo -e "$cmd\n"
		parent_log_dir=./output/$date
		if [ ! -x $parent_log_dir ];then
			mkdir -p $parent_log_dir
		fi
		log_file=$parent_log_dir/${file_infos[FILE_NAME]}.log
		if [ ! -f $log_file ];then
			log_dir=`dirname $log_file`
			if [ ! -x $log_dir ];then
				mkdir -p $log_dir
			fi
			touch $log_file
		fi
		$cmd >> $log_file 2>&1

		witness_true_status=`grep "Verification result: TRUE" $log_file`
		witness_unknown_status=`grep  "Verification result: UNKNOWN" $log_file`
		witness_false_status=`grep "Verification result: FALSE" $log_file`
		witness_out_of_memory_status=`grep "java.lang.OutOfMemoryError" $log_file`
		witness_timeout_status=`grep "The CPU-time limit of 900s has elapsed" $log_file`
		witness_parse_error_status=`grep "Error: Invalid configuration (Could not load automaton from file" $log_file`
        
		if [ $expected_status = "TRUE" ];then
			if [ -n "$witness_true_status" ];then
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},OK" >> $witness_result_file 
			elif [ -n "$witness_unknown_status" ];then
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},UNKNOWN" >> $witness_result_file
			elif [ -n "$witness_out_of_memory_status" ];then
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},OUT OF MEMORY" >> $witness_result_file
			elif [ -n "$witness_timeout_status" ];then
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},TIMEOUT" >> $witness_result_file
			elif [ -n "$witness_false_status" ];then
                echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},WRONG-TRUE" >> $witness_result_file
            elif [ -n "$witness_parse_error_status" ];then
            	echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},PARSE ERROR" >> $witness_result_file
            else
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},ERROR" >> $witness_result_file
			fi
		fi

		if [ $expected_status = "false(reach)" ];then
			if [ -n "$witness_false_status" ];then
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},OK" >> $witness_result_file
			elif [ -n "$witness_unknown_status" ];then
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},UNKNOWN" >> $witness_result_file
			elif [ -n "$witness_out_of_memory_status" ];then
                echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},OUT OF MEMORY" >> $witness_result_file
            elif [ -n "$witness_timeout_status" ]; then
                echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},TIMEOUT" >> $witness_result_file
            elif [ -n "$witness_true_status" ];then
                echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},WRONG-FALSE" >> $witness_result_file
            elif [ -n "$witness_parse_error_status" ];then
            	echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},PARSE ERROR" >> $witness_result_file
            else
				echo "${file_infos[FILE_NAME]},${file_infos[FILE_STATUS]},${file_infos[CPU_TIME]},${file_infos[WALL_TIME]},${file_infos[EXPECTED_STATUS]},${file_infos[ACTUAL_STATUS]},ERROR" >> $witness_result_file
			fi
		fi
    done
	cd - # return back original dir
}

main $1;
