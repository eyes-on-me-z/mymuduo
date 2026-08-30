#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{

struct Config
{
    std::string host = "127.0.0.1";
    std::string port = "8080";
    int total = 10000;
    int concurrency = 100;
    int messageSize = 16;
    int timeoutMs = 1000;
};

struct Stats
{
    std::atomic<int> success{0};
    std::atomic<int> connectFail{0};
    std::atomic<int> sendFail{0};
    std::atomic<int> recvFail{0};
    std::atomic<long long> bytesRead{0};
};

void usage(const char* program)
{
    std::cout
        << "Usage: " << program << " [host] [port] [total] [concurrency] [message_size] [timeout_ms]\n"
        << "Example:\n"
        << "  " << program << " 127.0.0.1 8080 50000 500 64 1000\n";
}

bool parseInt(const char* text, int* value)
{
    char* end = nullptr;
    long result = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || result <= 0 || result > 100000000)
    {
        return false;
    }
    *value = static_cast<int>(result);
    return true;
}

bool parseArgs(int argc, char* argv[], Config* config, bool* showHelp)
{
    if (argc > 1)
    {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help")
        {
            *showHelp = true;
            usage(argv[0]);
            return false;
        }
        config->host = arg;
    }
    if (argc > 2) config->port = argv[2];
    if (argc > 3 && !parseInt(argv[3], &config->total)) return false;
    if (argc > 4 && !parseInt(argv[4], &config->concurrency)) return false;
    if (argc > 5 && !parseInt(argv[5], &config->messageSize)) return false;
    if (argc > 6 && !parseInt(argv[6], &config->timeoutMs)) return false;
    if (argc > 7) return false;
    config->concurrency = std::min(config->concurrency, config->total);
    return true;
}

bool resolveAddress(const Config& config, sockaddr_storage* addr, socklen_t* addrLen)
{
    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int rc = getaddrinfo(config.host.c_str(), config.port.c_str(), &hints, &result);
    if (rc != 0)
    {
        std::cerr << "getaddrinfo failed: " << gai_strerror(rc) << "\n";
        return false;
    }

    bool ok = false;
    for (addrinfo* p = result; p != nullptr; p = p->ai_next)
    {
        if (p->ai_addrlen <= sizeof(sockaddr_storage))
        {
            std::memcpy(addr, p->ai_addr, p->ai_addrlen);
            *addrLen = static_cast<socklen_t>(p->ai_addrlen);
            ok = true;
            break;
        }
    }

    freeaddrinfo(result);
    return ok;
}

void setTimeout(int fd, int timeoutMs)
{
    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

bool writeAll(int fd, const char* data, size_t len)
{
    size_t written = 0;
    while (written < len)
    {
        ssize_t n = send(fd, data + written, len - written, 0);
        if (n <= 0) return false;
        written += static_cast<size_t>(n);
    }
    return true;
}

bool readExpected(int fd, size_t expected, long long* bytesRead)
{
    std::vector<char> buf(4096);
    size_t total = 0;
    while (total < expected)
    {
        size_t want = std::min(buf.size(), expected - total);
        ssize_t n = recv(fd, buf.data(), want, 0);
        if (n <= 0) return false;
        total += static_cast<size_t>(n);
    }
    *bytesRead += static_cast<long long>(total);
    return true;
}

void worker(const Config& config,
            const sockaddr_storage& addr,
            socklen_t addrLen,
            const std::string& message,
            std::atomic<int>* next,
            Stats* stats,
            std::vector<long long>* latenciesUs,
            std::mutex* latencyMutex)
{
    for (;;)
    {
        int id = next->fetch_add(1);
        if (id >= config.total) break;

        auto begin = std::chrono::steady_clock::now();
        int fd = socket(addr.ss_family, SOCK_STREAM, 0);
        if (fd < 0)
        {
            stats->connectFail.fetch_add(1);
            continue;
        }

        setTimeout(fd, config.timeoutMs);

        if (connect(fd, reinterpret_cast<const sockaddr*>(&addr), addrLen) != 0)
        {
            stats->connectFail.fetch_add(1);
            close(fd);
            continue;
        }

        if (!writeAll(fd, message.data(), message.size()))
        {
            stats->sendFail.fetch_add(1);
            close(fd);
            continue;
        }

        long long bytes = 0;
        if (!readExpected(fd, message.size(), &bytes))
        {
            stats->recvFail.fetch_add(1);
            close(fd);
            continue;
        }

        close(fd);
        auto end = std::chrono::steady_clock::now();
        long long latency = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

        stats->bytesRead.fetch_add(bytes);
        stats->success.fetch_add(1);
        std::lock_guard<std::mutex> lock(*latencyMutex);
        latenciesUs->push_back(latency);
    }
}

long long percentile(const std::vector<long long>& values, double p)
{
    if (values.empty()) return 0;
    size_t index = static_cast<size_t>((values.size() - 1) * p);
    return values[index];
}

} // namespace

int main(int argc, char* argv[])
{
    Config config;
    bool showHelp = false;
    if (!parseArgs(argc, argv, &config, &showHelp))
    {
        if (!showHelp)
        {
            usage(argv[0]);
        }
        return showHelp ? 0 : 1;
    }

    sockaddr_storage addr;
    socklen_t addrLen = 0;
    if (!resolveAddress(config, &addr, &addrLen)) return 1;

    std::string message(static_cast<size_t>(config.messageSize), 'x');
    Stats stats;
    std::atomic<int> next{0};
    std::vector<long long> latenciesUs;
    latenciesUs.reserve(static_cast<size_t>(config.total));
    std::mutex latencyMutex;

    std::cout << "target=" << config.host << ':' << config.port
              << " total=" << config.total
              << " concurrency=" << config.concurrency
              << " message_size=" << config.messageSize
              << " timeout_ms=" << config.timeoutMs << "\n";

    auto begin = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(config.concurrency));
    for (int i = 0; i < config.concurrency; ++i)
    {
        threads.emplace_back(worker,
                             std::cref(config),
                             std::cref(addr),
                             addrLen,
                             std::cref(message),
                             &next,
                             &stats,
                             &latenciesUs,
                             &latencyMutex);
    }

    for (auto& thread : threads) thread.join();
    auto end = std::chrono::steady_clock::now();

    double seconds = std::chrono::duration<double>(end - begin).count();
    int success = stats.success.load();
    int failed = stats.connectFail.load() + stats.sendFail.load() + stats.recvFail.load();

    std::sort(latenciesUs.begin(), latenciesUs.end());
    long long sumLatency = 0;
    for (long long latency : latenciesUs) sumLatency += latency;

    double avgLatencyMs = success == 0 ? 0.0 : (sumLatency / 1000.0 / success);
    double qps = seconds <= 0.0 ? 0.0 : success / seconds;
    double throughputMiB = seconds <= 0.0 ? 0.0 : stats.bytesRead.load() / seconds / 1024.0 / 1024.0;

    std::cout << "\n";
    std::cout << "success=" << success << " failed=" << failed << "\n";
    std::cout << "connect_fail=" << stats.connectFail.load()
              << " send_fail=" << stats.sendFail.load()
              << " recv_fail=" << stats.recvFail.load() << "\n";
    std::cout << "elapsed_sec=" << seconds << "\n";
    std::cout << "qps=" << qps << "\n";
    std::cout << "read_throughput_MiBps=" << throughputMiB << "\n";
    std::cout << "latency_ms avg=" << avgLatencyMs
              << " p50=" << percentile(latenciesUs, 0.50) / 1000.0
              << " p95=" << percentile(latenciesUs, 0.95) / 1000.0
              << " p99=" << percentile(latenciesUs, 0.99) / 1000.0
              << " max=" << percentile(latenciesUs, 1.00) / 1000.0 << "\n";

    return failed == 0 ? 0 : 2;
}
