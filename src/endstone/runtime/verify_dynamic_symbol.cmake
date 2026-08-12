execute_process(
        COMMAND "${NM}" -D --defined-only "${LIBRARY}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE symbols)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "Unable to inspect dynamic symbols in ${LIBRARY}")
endif ()

string(FIND "${symbols}" "${SYMBOL}" position)
if (position EQUAL -1)
    message(FATAL_ERROR "Required runtime hook symbol ${SYMBOL} is not exported by ${LIBRARY}")
endif ()
