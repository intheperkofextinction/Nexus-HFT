#!/bin/bash
set -e

echo "=========================================================="
echo "          NEXUS-HFT END-TO-END PIPELINE LAUNCHER          "
echo "=========================================================="

# 1. Apply Network Tuning
echo "[1/4] Applying Host Kernel & Network Socket Tuning..."
chmod +x setup_host.sh
./setup_host.sh

# 2. Clean & Build Project
echo "[2/4] Compiling Nexus-HFT Engine with -O3 -march=native..."
mkdir -p build && cd build
cmake .. > /dev/null
make -j$(nproc)

# 3. Generate PCAP Feed
echo "[3/4] Generating Binary NASDAQ ITCH 5.0 Network Stream..."
./generate_pcap

# 4. Execute Core Engine with Thread Affinity
echo "[4/4] Launching Nexus-HFT Engine Core..."
echo "=========================================================="
./nexus_hft

