#!/bin/bash

# Function to run USIMM and extract the EDP value
run_and_get_edp() {
    local cfg=$1
    local trace1=$2
    local trace2=$3
    local edp

    # Run USIMM, find the line with EDP, extract the value
    edp=$(bin/usimm "$cfg" "$trace1" "$trace2" | grep "Energy Delay product" | awk -F'= ' '{print $2}' | awk '{print $1}')
    echo "$edp"
}

# Run both simulations and get EDPs
echo "Running first USIMM simulation..."
edp1=$(run_and_get_edp input/1channel.cfg input/comm1 input/comm2)

echo "Running second USIMM simulation..."
edp2=$(run_and_get_edp input/1channel.cfg input/black input/freq)

# Calculate average EDP
average_edp=$(awk -v a="$edp1" -v b="$edp2" 'BEGIN { printf "%.9f\n", (a + b) / 2 }')

# Print results
echo ""
echo "EDP 1: $edp1 J.s"
echo "EDP 2: $edp2 J.s"
echo "Average EDP: $average_edp J.s"
