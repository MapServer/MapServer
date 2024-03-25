#!/bin/bash
cppcheck --version

LOG_FILE=/tmp/cppcheck_mapserver.txt

SCRIPT_DIR=$(dirname "$0")
case $SCRIPT_DIR in
    "/"*)
        ;;
    ".")
        SCRIPT_DIR=$(pwd)
        ;;
    *)
        SCRIPT_DIR=$(pwd)/$(dirname "$0")
        ;;
esac

TOPDIR="$SCRIPT_DIR/../src"

CPPCHECK_VERSION="$(cppcheck --version | awk '{print $2}')"
CPPCHECK_VERSION="$(cppcheck --version | awk '{print $2}')"
CPPCHECK_MAJOR_VERSION=$(echo "$CPPCHECK_VERSION" | cut -d. -f1)
CPPCHECK_MINOR_VERSION=$(echo "$CPPCHECK_VERSION" | cut -d. -f2)
CPPCHECK_VERSION=$(("$CPPCHECK_MAJOR_VERSION" * 100 + "$CPPCHECK_MINOR_VERSION"))
CPPCHECK_VERSION_GT_2_7=$(expr "$CPPCHECK_VERSION" \>= 207 || /bin/true)
if test "$CPPCHECK_VERSION_GT_2_7" = 1; then
    POSIX="--library=gnu"
else
    POSIX="--std=posix"
fi

echo "" > ${LOG_FILE}
for dirname in ${TOPDIR}/.; do
    echo "Running cppcheck on $dirname ... (can be long)"
    if ! cppcheck --inline-suppr --template='{file}:{line},{severity},{id},{message}' \
        --enable=all --inconclusive --std=c99 --std=c++11 ${POSIX} \
        "-I${TOPDIR}" \
        -DUSE_CURL -DUSE_CAIRO -DUSE_RSVG -DUSE_ICONV -DUSE_FRIBIDI -DUSE_HARFBUZZ -DUSE_EXEMPI -DUSE_GEOS -DUSE_LIBXML2 -DUSE_THREAD -DUSE_FASTCGI -DUSE_WFS_LYR -DUSE_WMS_LYR \
        -DGEOS_VERSION_MAJOR=3 -DGEOS_VERSION_MINOR=8 \
        -DRETURN_FALSE="return false" \
        -Dcppcheck_assert=assert \
        "$dirname" \
        -j 8 >>${LOG_FILE} 2>&1 ; then
        echo "cppcheck failed"
        exit 1
    fi
done

ret_code=0

grep -v "unmatchedSuppression" ${LOG_FILE} \
    | grep -v "/renderers/agg" \
    | grep -v "/third-party" \
    | grep -v "/build" \
    | grep -v "yacc.c" \
    | grep -v "maplexer.c" \
    > ${LOG_FILE}.tmp
mv ${LOG_FILE}.tmp ${LOG_FILE}

if grep "null pointer" ${LOG_FILE} ; then
    echo "Null pointer check failed"
    ret_code=1
fi

if grep "duplicateBreak" ${LOG_FILE} ; then
    echo "duplicateBreak check failed"
    ret_code=1
fi

if grep "duplicateBranch" ${LOG_FILE} ; then
    echo "duplicateBranch check failed"
    ret_code=1
fi

if grep "uninitMemberVar" ${LOG_FILE} ; then
    echo "uninitMemberVar check failed"
    ret_code=1
fi

if grep "useInitializationList" ${LOG_FILE} ; then
    echo "uninitMemberVar check failed"
    ret_code=1
fi

if grep "clarifyCalculation" ${LOG_FILE} ; then
    echo "clarifyCalculation check failed"
    ret_code=1
fi

if grep "invalidPrintfArgType_uint" ${LOG_FILE} ; then
    echo "invalidPrintfArgType_uint check failed"
    ret_code=1
fi

if grep "catchExceptionByValue" ${LOG_FILE} ; then
    echo "catchExceptionByValue check failed"
    ret_code=1
fi

if grep "memleakOnRealloc" ${LOG_FILE} ; then
    echo "memleakOnRealloc check failed"
    ret_code=1
fi

if grep "arrayIndexOutOfBoundsCond" ${LOG_FILE} ; then
    echo "arrayIndexOutOfBoundsCond check failed"
    ret_code=1
fi

if grep "arrayIndexOutOfBounds," ${LOG_FILE} ; then
    echo "arrayIndexOutOfBounds check failed"
    ret_code=1
fi

if grep "syntaxError" ${LOG_FILE} | grep -v "is invalid C code" ; then
    echo "syntaxError check failed"
    ret_code=1
fi

if grep "memleak," ${LOG_FILE} ; then
    echo "memleak check failed"
    ret_code=1
fi

if grep "eraseDereference" ${LOG_FILE} ; then
    echo "eraseDereference check failed"
    ret_code=1
fi

if grep "memsetClass," ${LOG_FILE} ; then
    echo "memsetClass check failed"
    ret_code=1
fi

if grep "uninitvar," ${LOG_FILE} ; then
    echo "uninitvar check failed"
    ret_code=1
fi

if grep "uninitdata," ${LOG_FILE} ; then
    echo "uninitdata check failed"
    ret_code=1
fi

if grep "va_list_usedBeforeStarted" ${LOG_FILE} ; then
    echo "va_list_usedBeforeStarted check failed"
    ret_code=1
fi

if grep "duplInheritedMember" ${LOG_FILE} ; then
    echo "duplInheritedMember check failed"
    ret_code=1
fi

if grep "terminateStrncpy" ${LOG_FILE} ; then
    echo "terminateStrncpy check failed"
    ret_code=1
fi

if grep "operatorEqVarError" ${LOG_FILE} ; then
    echo "operatorEqVarError check failed"
    ret_code=1
fi

if grep "uselessAssignmentPtrArg" ${LOG_FILE} ; then
    echo "uselessAssignmentPtrArg check failed"
    ret_code=1
fi

if grep "bufferNotZeroTerminated" ${LOG_FILE} ; then
    echo "bufferNotZeroTerminated check failed"
    ret_code=1
fi

if grep "sizeofDivisionMemfunc" ${LOG_FILE} ; then
    echo "sizeofDivisionMemfunc check failed"
    ret_code=1
fi

if grep "selfAssignment" ${LOG_FILE} ; then
    echo "selfAssignment check failed"
    ret_code=1
fi

if grep "invalidPrintfArgType_sint" ${LOG_FILE} ; then
    echo "invalidPrintfArgType_sint check failed"
    ret_code=1
fi

if grep "redundantAssignInSwitch" ${LOG_FILE} ; then
    echo "redundantAssignInSwitch check failed"
    ret_code=1
fi

if grep "publicAllocationError" ${LOG_FILE} ; then
    echo "publicAllocationError check failed"
    ret_code=1
fi

if grep "invalidScanfArgType_int" ${LOG_FILE} ; then
    echo "invalidScanfArgType_int check failed"
    ret_code=1
fi

if grep "invalidscanf," ${LOG_FILE} ; then
    echo "invalidscanf check failed"
    ret_code=1
fi

if grep "moduloAlwaysTrueFalse" ${LOG_FILE} ; then
    echo "moduloAlwaysTrueFalse check failed"
    ret_code=1
fi

if grep "charLiteralWithCharPtrCompare" ${LOG_FILE} ; then
    echo "charLiteralWithCharPtrCompare check failed"
    ret_code=1
fi

if grep "noConstructor" ${LOG_FILE} ; then
    echo "noConstructor check failed"
    ret_code=1
fi

if grep "noExplicitConstructor" ${LOG_FILE} ; then
    echo "noExplicitConstructor check failed"
    ret_code=1
fi

if grep "noCopyConstructor" ${LOG_FILE} ; then
    echo "noCopyConstructor check failed"
    ret_code=1
fi

if grep "passedByValue" ${LOG_FILE} ; then
    echo "passedByValue check failed"
    ret_code=1
fi

if grep "postfixOperator" ${LOG_FILE} ; then
    echo "postfixOperator check failed"
    ret_code=1
fi

if grep "redundantCopy" ${LOG_FILE} ; then
    echo "redundantCopy check failed"
    ret_code=1
fi

if grep "stlIfStrFind" ${LOG_FILE} ; then
    echo "stlIfStrFind check failed"
    ret_code=1
fi

if grep "functionStatic" ${LOG_FILE} ; then
    echo "functionStatic check failed"
    ret_code=1
fi

if grep "knownConditionTrueFalse" ${LOG_FILE} ; then
    echo "knownConditionTrueFalse check failed"
    ret_code=1
fi

if grep "arrayIndexThenCheck" ${LOG_FILE} ; then
    echo "arrayIndexThenCheck check failed"
    ret_code=1
fi

if grep "unusedPrivateFunction" ${LOG_FILE} ; then
    echo "unusedPrivateFunction check failed"
    ret_code=1
fi

if grep "redundantCondition" ${LOG_FILE} ; then
    echo "redundantCondition check failed"
    ret_code=1
fi

if grep "unusedStructMember" ${LOG_FILE} ; then
    echo "unusedStructMember check failed"
    ret_code=1
fi

if grep "multiCondition" ${LOG_FILE} ; then
    echo "multiCondition check failed"
    ret_code=1
fi

if grep "duplicateExpression" ${LOG_FILE} ; then
    echo "duplicateExpression check failed"
    ret_code=1
fi

if grep "operatorEq" ${LOG_FILE} ; then
    echo "operatorEq check failed"
    ret_code=1
fi

if grep "truncLongCastAssignment" ${LOG_FILE} ; then
    echo "truncLongCastAssignment check failed"
    ret_code=1
fi

if grep "exceptRethrowCopy" ${LOG_FILE} ; then
    echo "exceptRethrowCopy check failed"
    ret_code=1
fi

if grep "unusedVariable" ${LOG_FILE} ; then
    echo "unusedVariable check failed"
    ret_code=1
fi

if grep "unsafeClassCanLeak" ${LOG_FILE} ; then
    echo "unsafeClassCanLeak check failed"
    ret_code=1
fi

if grep "unsignedLessThanZero" ${LOG_FILE} ; then
    echo "unsignedLessThanZero check failed"
    ret_code=1
fi

if grep "unpreciseMathCall" ${LOG_FILE} ; then
    echo "unpreciseMathCall check failed"
    ret_code=1
fi

if grep "unreachableCode" ${LOG_FILE} ; then
    echo "unreachableCode check failed"
    ret_code=1
fi

if grep "clarifyCondition" ${LOG_FILE} ; then
    echo "clarifyCondition check failed"
    ret_code=1
fi

if grep "redundantIfRemove" ${LOG_FILE} ; then
    echo "redundantIfRemove check failed"
    ret_code=1
fi

if grep "unassignedVariable" ${LOG_FILE} ; then
    echo "unassignedVariable check failed"
    ret_code=1
fi

if grep "redundantAssignment" ${LOG_FILE} ; then
    echo "redundantAssignment check failed"
    ret_code=1
fi

if grep "unreadVariable" ${LOG_FILE} ; then
    echo "unreadVariable check failed"
    ret_code=1
fi

if grep "AssignmentAddressToInteger" ${LOG_FILE} ; then
    echo "AssignmentAddressToInteger check failed"
    ret_code=1
fi


# Check any remaining errors
if grep "error," ${LOG_FILE} | grep -v "uninitvar" | \
    grep -v "memleak," | grep -v "memleakOnRealloc" | \
    grep -v "is invalid C code" ; then

    echo "Errors check failed"
    ret_code=1
fi

# Check any remaining warnings
if grep "warning," ${LOG_FILE}; then
    echo "Warnings check failed"
    ret_code=1
fi

if [ ${ret_code} = 0 ]; then
    echo "cppcheck succeeded"
fi

exit ${ret_code}
