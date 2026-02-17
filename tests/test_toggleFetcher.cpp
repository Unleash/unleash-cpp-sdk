#include <gtest/gtest.h>

#include "unleash/Fetcher/toggleFetcher.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// POSIX sockets
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

static std::string kSampleFeaturesJson()
{

    return R"JSON(
{
  "version": 1,
  "toggles": [
    {
      "name": "flagA",
      "description": "test flag",
      "enabled": true,
      "strategies": [],
      "variants": []
    }
  ]
}
)JSON";
}

static std::optional<std::string> getHeaderValueLowerKey(const std::string& request, const std::string& lowerKey)
{

    auto toLower = [](std::string s) {
        for (auto& c : s) c = (char)std::tolower((unsigned char)c);
        return s;
    };

    std::istringstream iss(request);
    std::string line;

    // Skip request-line
    std::getline(iss, line);

    while (std::getline(iss, line)) {
        if (line == "\r" || line.empty())
            break;
        // strip trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        auto pos = line.find(':');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        // trim left space
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(val.begin());

        if (toLower(key) == lowerKey) return val;
    }
    return std::nullopt;
}

class MiniHttpServer {
public:
    struct Observations {
        std::atomic<int> requests{0};
        std::atomic<bool> sawIfNoneMatchOnSecond{false};
        std::string lastIfNoneMatch;
    };

    MiniHttpServer()
    {
        _listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_listenFd < 0) throw std::runtime_error("socket() failed");

        int yes = 1;
        ::setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
        addr.sin_port = htons(0); // ephemeral

        if (::bind(_listenFd, (sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");

        socklen_t len = sizeof(addr);
        if (::getsockname(_listenFd, (sockaddr*)&addr, &len) < 0)
            throw std::runtime_error("getsockname() failed");

        _port = ntohs(addr.sin_port);

        if (::listen(_listenFd, 16) < 0)
            throw std::runtime_error("listen() failed");

        _thread = std::thread([this] { this->loop(); });
    }

    ~MiniHttpServer()
    {
        _stop.store(true);
        if (_listenFd >= 0) ::shutdown(_listenFd, SHUT_RDWR);
        if (_thread.joinable()) _thread.join();
        if (_listenFd >= 0) ::close(_listenFd);
    }

    int port() const { return _port; }
    Observations& obs() { return _obs; }

    void setForce500OnNext(bool v) { _force500Next.store(v); }

private:
    void loop()
    {
        while (!_stop.load()) {
            sockaddr_in client{};
            socklen_t clen = sizeof(client);
            int cfd = ::accept(_listenFd, (sockaddr*)&client, &clen);
            if (cfd < 0) {
                if (_stop.load()) break;
                continue;
            }

            std::string req = readRequest(cfd);
            int idx = ++_obs.requests;

            if (_force500Next.exchange(false)) {
                sendResponse(cfd, 500, "Internal Server Error", "text/plain", "boom", std::nullopt);
                ::close(cfd);
                continue;
            }

            if (idx == 1) {
                sendResponse(
                    cfd,
                    200,
                    "OK",
                    "application/json",
                    kSampleFeaturesJson(),
                    std::string("W/\"unit-test-etag-1\"")
                );
            } else if (idx == 2) {
                auto inm = getHeaderValueLowerKey(req, "if-none-match");
                if (inm && *inm == "W/\"unit-test-etag-1\"") {
                    _obs.sawIfNoneMatchOnSecond.store(true);
                    _obs.lastIfNoneMatch = *inm;
                    // 304 should not include a body. Content-Length 0 is fine.
                    sendResponse(cfd, 304, "Not Modified", "text/plain", "", std::string("W/\"unit-test-etag-1\""));
                } else {
                    // If client didn't send header, fallback to 200
                    sendResponse(
                        cfd,
                        200,
                        "OK",
                        "application/json",
                        kSampleFeaturesJson(),
                        std::string("W/\"unit-test-etag-1\"")
                    );
                }
            } else {
                // For any extra requests, keep returning 200
                sendResponse(
                    cfd,
                    200,
                    "OK",
                    "application/json",
                    kSampleFeaturesJson(),
                    std::string("W/\"unit-test-etag-1\"")
                );
            }

            ::close(cfd);
        }
    }

    static std::string readRequest(int fd)
    {
        std::string data;
        data.reserve(4096);

        char buf[1024];
        while (true) {
            ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            data.append(buf, buf + n);

            // stop when headers ended
            if (data.find("\r\n\r\n") != std::string::npos) break;
        }
        return data;
    }

    static void sendAll(int fd, const std::string& s)
    {
        const char* p = s.data();
        size_t left = s.size();
        while (left > 0) {
            ssize_t n = ::send(fd, p, left, 0);
            if (n <= 0) return;
            p += (size_t)n;
            left -= (size_t)n;
        }
    }

    static void sendResponse(
        int fd,
        int status,
        const std::string& statusText,
        const std::string& contentType,
        const std::string& body,
        const std::optional<std::string>& etag
    ) {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status << " " << statusText << "\r\n";
        oss << "Connection: close\r\n";
        if (etag) oss << "ETag: " << *etag << "\r\n";
        oss << "Content-Type: " << contentType << "\r\n";
        oss << "Content-Length: " << body.size() << "\r\n";
        oss << "\r\n";
        oss << body;

        sendAll(fd, oss.str());
    }

private:
    int _listenFd{-1};
    int _port{0};
    std::thread _thread;
    std::atomic<bool> _stop{false};
    std::atomic<bool> _force500Next{false};
    Observations _obs{};
};

} // namespace

TEST(ToggleFetcher, Fetch200ReturnsTogglesAndCachesEtag_Then304WhenNotModified)
{
    MiniHttpServer server;


    std::string baseUrl = "http://127.0.0.1:" + std::to_string(server.port());
    std::string baseKey = "dummy-client-key";
    std::string baseNameApp = "unitApp";
    

    unleash::ClientConfig cfg(baseUrl, baseKey, baseNameApp);


    unleash::Context ctx("unitApp", "dev" ,"sess-1");   
    ctx.setCurrentTime()
    .setUserId("user-1")
    .setRemoteAddress("127.0.0.1");

    unleash::ToggleFetcher fetcher(cfg);

    // 1) First fetch: expect 200 + toggles
    auto r1 = fetcher.fetch(ctx);
    std::cout<<"#######################################################################\n";
    std::cout<<"Status is: "<<r1.status<<" and error is: "<<(r1.error.has_value()? r1.error.value(): "<No value>")<<std::endl;
    EXPECT_EQ(r1.status, 200);
    EXPECT_FALSE(r1.error.has_value());
    EXPECT_TRUE(r1.toggles.has_value());
    EXPECT_EQ(r1.toggles.value().size(), 1u);
    // Second fetch: server expects If-None-Match and returns 304
    auto r2 = fetcher.fetch(ctx);
    EXPECT_EQ(r2.status, 304);
    EXPECT_FALSE(r2.error.has_value());

    // Typical behavior: no new toggles on 304
    EXPECT_FALSE(r2.toggles.has_value());

    // Ensure If-None-Match was actually sent on second request
    EXPECT_TRUE(server.obs().sawIfNoneMatchOnSecond.load());
}

TEST(ToggleFetcher, Fetch500ReturnsError)
{
    MiniHttpServer server;

    const std::string baseUrl = "http://127.0.0.1:" + std::to_string(server.port());
    unleash::ClientConfig cfg(baseUrl, "dummy-client-key", "unitApp");
    unleash::Context ctx("unitApp","dev", "sess-1");
    ctx.setCurrentTime()
        .setUserId("user-1")
        .setRemoteAddress("127.0.0.1");

    unleash::ToggleFetcher fetcher(cfg);

    server.setForce500OnNext(true);

    auto r = fetcher.fetch(ctx);
    EXPECT_EQ(r.status, 500);

    EXPECT_FALSE(r.toggles.has_value());

    if (r.error.has_value()) {
        EXPECT_FALSE(r.error->empty());
    }
}
