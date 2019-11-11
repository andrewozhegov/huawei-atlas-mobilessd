#!/bin/sh

export LD_LIBRARY_PATH=/home/HwHiAiUser/tools/che/ddk/ddk/uihost/lib/
TOP_DIR=${PWD}
BUILD_DIR=${TOP_DIR}/build
LOCAL_OBJ_PATH=${TOP_DIR}/out
cd ${TOP_DIR} && make
if [ $? != 0 ]
then
  rm ${LOCAL_OBJ_PATH}/*.o ${LOCAL_OBJ_PATH}/main
  exit
fi

DATASET_SIZE=2226
DEVICE_N=4
#for ((deviceID=0; deviceID<$DEVICE_N; deviceID++)); do
for deviceID in 0 1 2 3; do
    from=$(( ($DATASET_SIZE / $DEVICE_N) * $deviceID + 1 ))
    to=$(( ($DATASET_SIZE / $DEVICE_N) * ($deviceID + 1) ))
    tmpfile="$(mktemp)"
    awk -v deviceID=$deviceID -v from=$from -v to=$to '{
        if (!match($0,"selectImages")) {
            if (match($0,"graph_id")) $2="10"deviceID
            if (match($0,"device_id")) $2="\"deviceID\""
            print $0;
            next;
        } else {
            print $0; getline;
            list=from;
            for (i=++from; i<=to; i++) {
                list=list","i
            };
            $2="\""list"\"";
            print $0
        }
    }' graph.config > "$tmpfile"
    mv "$tmpfile" './out/graph'$deviceID'.config'
done

cd ${BUILD_DIR}
if [ -d "host" ];then
  if [ -f "host/Makefile" ];then
    cd host && make install
    if [ $? != 0 ]
    then
      rm ${LOCAL_OBJ_PATH}/*.o ${LOCAL_OBJ_PATH}/main
      rm ${LOCAL_OBJ_PATH}/*.so
      exit
    fi
  fi
fi

cd ${BUILD_DIR}

if [ -d "device" ];then
  if [ -f "device/Makefile" ];then
    cd device && make install
    if [ $? != 0 ]
    then
      rm ${LOCAL_OBJ_PATH}/*.o ${LOCAL_OBJ_PATH}/main
      rm ${LOCAL_OBJ_PATH}/*.so
      exit
    fi
  fi
fi
echo "============ build success ! ============"
