#!/bin/bash

echo "=== [Nexus-HFT Low-Latency Host Configuration] ==="

# 1. Attempt CPU Frequency Scaling (Falls back silently on WSL2/Virtualization)
echo "Configuring CPU Performance Governors..."
if command -v cpupower &> /dev/null; then
    sudo cpupower frequency-set -g performance 2>/dev/null || true
else
    for dev in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        if [ -f "$dev" ]; then
            echo "performance" | sudo tee $dev > /dev/null 2>&1 || true
        fi
    done
fi

# 2. Tune Network Socket Buffers
echo "Tuning TCP/UDP Network Socket Buffers..."
sudo sysctl -w net.core.rmem_max=16777216 > /dev/null
sudo sysctl -w net.core.wmem_max=16777216 > /dev/null
sudo sysctl -w net.core.rmem_default=16777216 > /dev/null
sudo sysctl -w net.core.busy_poll=50 > /dev/null
sudo sysctl -w net.core.busy_read=50 > /dev/null

echo "Host network tuning applied successfully."
echo "================================================="

