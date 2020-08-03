#!/bin/bash

# This script runs a series of placement jobs. It, along with
# main_template.cpp, need to be deployed to the root directory of the
# repository. This script must also be called from that directory (I'm too lazy
# to write the chdir logic).
#
# Read and understand before running.
set -e
set -x

TEMPLATE_SOURCE="main_template.cpp"
TEMPLATE_TARGET="src/main.cpp"
ITERATIONS=5e9

# Mouse mode disables all logging and intermediate fitness computation, and
# simply prints out the runtime of the placement job to a text file.
MOUSE_MODE=1
MOUSE_FILE="mouse_out_${ITERATIONS}.txt"

if [ ${MOUSE_MODE} -eq 1 ]; then
    > "${MOUSE_FILE}"
fi

# Run one serial placement job (THREAD_COUNT = 0), and one parallel placement
# job for different numbers of compute workers.
for THREAD_COUNT in 0 1 $(seq 4 4 64); do

    # Deploy template, and provision.
    cp "${TEMPLATE_SOURCE}" "${TEMPLATE_TARGET}"
    sed -i "s|{{ITERATIONS}}|$ITERATIONS|" "${TEMPLATE_TARGET}"

    if [ ${MOUSE_MODE} -eq 1 ]; then
	    sed -i "s|{{MOUSE_MODE}}|true|" "${TEMPLATE_TARGET}"
    else
	    sed -i "s|{{MOUSE_MODE}}|false|" "${TEMPLATE_TARGET}"
    fi

    if [ ${THREAD_COUNT} -eq 0 ]; then
        sed -i "s|{{SERIAL_MODE}}|true|" "${TEMPLATE_TARGET}"
        sed -i "s|{{NUM_THREADS}}|1|" "${TEMPLATE_TARGET}"
    else
        sed -i "s|{{SERIAL_MODE}}|false|" "${TEMPLATE_TARGET}"
        sed -i "s|{{NUM_THREADS}}|$THREAD_COUNT|" "${TEMPLATE_TARGET}"
    fi

    # Build
	scons -j 4

    # Run
    if [ ${MOUSE_MODE} -eq 1 ]; then
        # Pushing output to file.
	    printf "${THREAD_COUNT}," >> ${MOUSE_FILE}
	    ./psap-run >> ${MOUSE_FILE}
    else
        # Log angrily, moving results afterwards.
	    ./psap-run
	    mv output/grid_poets output/grid_poets_${THREAD_COUNT}_$(date --iso-8601=date)_$(git rev-parse HEAD)
    fi
done
