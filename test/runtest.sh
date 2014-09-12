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
  $ECHON "* COMPILE ${ofile}...$NNL"
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
  echo "* RUNTEST ${ofile}:"  
  bin/${ofile} > out/${ofile}.out
  if [ ! $? = 0 ]; then
    echo "  FAIL(run)"
    exit 1
  fi
  if [ -e ${ofile}.stdout ]; then
    diff -q ${ofile}.stdout out/${ofile}.out >/dev/null
    if [ ! $? = 0 ]; then
      echo "  FAIL(diff)"
      diff -c ${ofile}.stdout out/${ofile}.out
      rm -f out/${ofile}.out
      exit 1
    fi
  fi
  echo "* ENDTEST ${ofile}"
  rm -f out/${ofile}.out
done
