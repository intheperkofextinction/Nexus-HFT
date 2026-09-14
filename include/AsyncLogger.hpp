#pragma once
#include <fstream>
#include <thread>
#include <atomic>
#include <pthread.h>
#include "SPSCQueue.hpp"

// Packed 24-byte binary log record
#pragma pack(push, 1)
struct LogRecord {
    uint64_t timestamp;
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
};
#pragma pack(pop)

class NonBlockingLogger {
public:
    NonBlockingLogger(const std::string& filename, int logger_cpu_core = 3) 
        : running_(true), log_file_(filename, std::ios::binary | std::ios::out) {
        
        // Spawn asynchronous background worker thread for disk I/O
        worker_thread_ = std::thread(&NonBlockingLogger::process_logs, this);

        // Pin background logger thread to a dedicated CPU core using POSIX CPU affinity
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(logger_cpu_core, &cpuset);
        pthread_setaffinity_np(worker_thread_.native_handle(), sizeof(cpu_set_t), &cpuset);
    }

    ~NonBlockingLogger() {
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        if (log_file_.is_open()) {
            log_file_.flush();
            log_file_.close();
        }
    }

    // Hot-path call: Pushes event into ring buffer in under ~5ns
    inline bool log(uint64_t ts, uint64_t id, uint32_t price, uint32_t qty) noexcept {
        return queue_.push(LogRecord{ts, id, price, qty});
    }

private:
    void process_logs() {
        LogRecord record;
        while (running_ || queue_.pop(record)) {
            if (queue_.pop(record)) {
                log_file_.write(reinterpret_cast<const char*>(&record), sizeof(LogRecord));
            } else {
                asm volatile("pause" ::: "memory");
            }
        }
    }

    SPSCQueue<LogRecord, 4096> queue_;
    std::atomic<bool> running_;
    std::ofstream log_file_;
    std::thread worker_thread_;
};
