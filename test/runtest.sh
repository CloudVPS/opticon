#!/bin/sh
TEST=`echo -n ""`
if [ -z "$TEST" ]; then
  ECHON="echo -n"
  NNL=""
else
  ECHON="echo"
  NNL="\c"
fi

CC="$*"
for test in test_*.c; do
  echo "====================================================================="
  ofile=$(echo "$test" | sed -e 's/\.c$//')
         
  $ECHON "[BUILD] Compiling ${ofile}...$NNL"
  make ${ofile}.o >out/${ofile}.build 2>&1
  if [ ! $? = 0 ]; then
    echo " FAIL"
    cat out/${ofile}.build
    rm -f out/${ofile}.build
    exit 1
  fi
  $CC -o bin/$ofile ${ofile}.o ../lib/*.a -lz >>out/${ofile}.build 2>&1
  if [ ! $? = 0 ]; then
    echo " FAIL"
    cat out/${ofile}.build
    rm -f out/${ofile}.build
    exit 1
  fi
  rm -f out/${ofile}.build
  echo " OK"
  echo "[CHECK] Running ${ofile}:"  
  bin/${ofile} > out/${ofile}.out
  if [ ! $? = 0 ]; then
    echo "[FAIL ] Process terminated with error status"
    exit 1
  fi
  if [ -e ${ofile}.stdout ]; then
    diff -q ${ofile}.stdout out/${ofile}.out >/dev/null
    if [ ! $? = 0 ]; then
      echo "[FAIL ] Output differed from expected output:"
      diff -c ${ofile}.stdout out/${ofile}.out
      rm -f out/${ofile}.out
      exit 1
    fi
  fi
  echo "[DONE ] Test succeeded"
  rm -f out/${ofile}.out
done
echo "====================================================================="
echo "All tests succeeded. This software is now guaranteed defect free,"
echo "provided it is not connected to a network (or power)."
echo ""
echo "Have a nice day."