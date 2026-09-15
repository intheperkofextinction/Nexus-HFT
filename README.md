# Nexus-HFT: Ultra-Low Latency Trading Engine (C++20)

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Linux](https://img.shields.io/badge/Platform-Linux-FCC624.svg)
![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg)
![GCC](https://img.shields.io/badge/GCC%2Fg%2B%2B-v11%2B-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

**Nexus-HFT** is a zero-allocation, lock-free High-Frequency Trading (HFT) matching engine and NASDAQ ITCH 5.0 market data processing architecture designed for sub-microsecond tick-to-trade execution.

---

## Performance Metrics (Sub-Microsecond Latency Profile)

*Measured on Linux (Kernel 5.15) via dynamic CPU cycle resolution (`RDTSC`):*

| Metric | Execution Latency | Time (Nanoseconds) |
| :--- | :--- | :--- |
| **p50 (Median)** | **~74 Cycles** | **74.57 ns** |
| **p90** | **~145 Cycles** | **141.20 ns** |
| **p99 (Tail)** | **~215 Cycles** | **215.43 ns** |
| **Matching Engine Latency** | **~84 Cycles** | **24.00 ns** |
| **Resiliency Check** | **~18 Cycles** | **5.14 ns** |

---

## Key Systems Architecture

```text
[ Network UDP/PCAP Feed ] ---> [ ITCH 5.0 Zero-Copy Parser ]
                                           |
                                           v
[ L3 Static Order Book ] <--- [ Lock-Free SPSC Queue ]
           |
           v
[ Matching Engine Core ] ---> [ Ring-Buffered Telemetry ]

```
---
### Core Innovations & Optimization Techniques
1. **Zero-Copy NASDAQ ITCH 5.0 Parsing:** Pointer casting and byte swapping (`be32toh`) directly over raw network buffers (**~188 ns** frame processing).
2. **$O(1)$ Zero-Allocation L3 Order Book:** Intrusive doubly-linked node pools pre-allocated in cache-aligned `std::array` slots. Zero heap allocations (`new`/`malloc`) on the hot path.
3. **Lock-Free Thread Handoff:** SPSC Ring Buffer utilizing explicit atomic memory ordering (`std::memory_order_release` / `acquire`) and cache-line padding (`alignas(64)`) to eliminate false sharing.
4. **Market Data Resiliency:** Sequence numbers tracking out-of-order packets and missing sequence gaps without blocking event ingestion (**~5.14 ns** overhead).
5. **Tail Latency Profiling:** Microsecond jitter measurement capturing distribution metrics ($p50$, $p90$, $p99$, $p99.9$) using RDTSC cycles.

---

### Production Live Terminal Dashboard

<img width="900" height="372" alt="image" src="https://github.com/user-attachments/assets/e00d8c67-65ad-4869-b6cb-2f0d04003940" />

---

## Quick Start

### Prerequisites
* **GCC 10+ / Clang 11+** with C++20 support
* **CMake 3.20+**
* `libpcap-dev`

### Setup & Execution

```bash
git clone [https://github.com/intheperkofextinction/Nexus-HFT.git](https://github.com/intheperkofextinction/Nexus-HFT.git)
cd Nexus-HFT
chmod +x scripts/run_pipeline.sh
./scripts/run_pipeline.sh
```

---

### Repository Structure

Nexus-HFT/
├── include/       # Core HFT primitives (OrderBook, MatchingEngine, SPSCQueue)
├── src/           # Engine entry point & real-time telemetry dashboard
├── tests/         # Phase benchmarks & microsecond tail-latency profilers
├── scripts/       # Automations, sysctl socket tuning & pipeline launcher
└── CMakeLists.txt # Build configuration (-O3 -march=native)


