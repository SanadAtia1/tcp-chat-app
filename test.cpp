// http_client.cpp
//
// A minimal HTTP/1.1 GET client, rewritten in idiomatic C++ from the
// classic C sockets example (Hunt/Stevens-style "UNP" code).
//
// Behavior is unchanged: connect to a server on port 80, send a bare
// "GET / HTTP/1.1" request, and print whatever bytes come back.
// One real bug from the original is fixed (see the comment in the
// read loop) and is called out explicitly rather than silently
// changed.

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Standard (unencrypted) HTTP port. Using 80 means this only works
// against plain HTTP servers, not HTTPS -- TLS isn't handled here.
constexpr int SERVER_PORT = 80;

// Size of the buffer used for each individual read() call. This is
// NOT a limit on total response size -- we keep reading in a loop
// until the peer closes the connection.
constexpr size_t MAXLINE = 4096;

// err_n_die(): print a formatted message (and, if errno is set, the
// matching strerror() text) to stdout, then terminate the process
// with a non-zero exit code.
//
// Kept as a C-style variadic function (rather than converted to a
// C++ exception type) because that's the simplest faithful port and
// this program has no cleanup to do beyond what the OS reclaims on
// exit() -- there's nothing an exception's stack unwinding would
// buy us here.
void err_n_die(const char *fmt, ...)
{
    // Any library/system call below could have set errno, so grab a
    // copy of it immediately -- printf() and friends could clobber
    // it before we get a chance to report it.
    const int errno_save = errno;

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stdout, fmt, ap);
    va_end(ap);
    std::fprintf(stdout, "\n");
    std::fflush(stdout);

    if (errno_save != 0)
    {
        std::fprintf(stdout, "(errno = %d) : %s\n", errno_save,
                     std::strerror(errno_save));
        std::fprintf(stdout, "\n");
        std::fflush(stdout);
    }

    std::exit(1);
}

// RAII wrapper around a raw socket file descriptor. This is the main
// piece of "C++-ification": in the original C code, any early
// err_n_die() after socket() succeeds would leak the fd (not fatal
// since the process exits anyway, but it's a habit worth breaking).
// Wrapping it means the destructor always closes the socket, no
// matter how we leave scope.
class TcpSocket
{
public:
    TcpSocket()
    {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0)
        {
            err_n_die("Error while creating socket!");
        }
    }

    ~TcpSocket()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
        }
    }

    // Non-copyable (a socket fd shouldn't be duplicated and closed
    // twice), but movable if you ever need to hand ownership off.
    TcpSocket(const TcpSocket &) = delete;
    TcpSocket &operator=(const TcpSocket &) = delete;
    TcpSocket(TcpSocket &&other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    TcpSocket &operator=(TcpSocket &&other) noexcept
    {
        if (this != &other)
        {
            if (fd_ >= 0) ::close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int fd() const { return fd_; }

private:
    int fd_ = -1;
};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        err_n_die("usage: %s <server address>", argv[0]);
    }

    // Create the socket. RAII takes care of closing it on every exit
    // path (normal return, or the process being torn down after
    // err_n_die's exit()).
    TcpSocket sock;

    // Build the destination address. bzero() is deprecated in favor
    // of memset(); the effect is identical -- zero the struct so
    // padding/reserved fields don't contain garbage.
    struct sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT); // network byte order

    // Translate the dotted-decimal address string (argv[1]) into the
    // binary form sockaddr_in expects. Note: inet_pton only accepts
    // numeric IPv4 addresses here, not hostnames -- passing a domain
    // name like "example.com" will fail this call. (A hostname-aware
    // version would use getaddrinfo() instead.)
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        err_n_die("inet_pton error for %s", argv[1]);
    }

    // Establish the TCP connection.
    if (connect(sock.fd(), reinterpret_cast<struct sockaddr *>(&servaddr),
                sizeof(servaddr)) < 0)
    {
        err_n_die("connect failed!");
    }

    // Build the request. Using std::string avoids the fixed-size
    // sendline buffer and sprintf() from the original -- there's no
    // way to overflow it, though with a hardcoded literal like this
    // it was never actually at risk.
    //
    // Note this is a bare-bones HTTP/1.1 request: it's missing the
    // mandatory "Host:" header, so many real servers (anything doing
    // name-based virtual hosting) will reject it with a 400. That
    // matches the original code's behavior -- not fixed here since
    // it'd change what gets sent, not just how it's written.
    const std::string request = "GET / HTTP/1.1\r\n\r\n";

    // Send the request, verifying that every byte was written.
    // write() can legitimately do a short write (especially on a
    // socket), so in production code you'd loop here and retry the
    // remainder; the original bails on any mismatch, and this keeps
    // that same simplifying assumption.
    ssize_t sent = write(sock.fd(), request.data(), request.size());
    if (sent < 0 || static_cast<size_t>(sent) != request.size())
    {
        err_n_die("write error");
    }

    // Read the response. Each read() call can return anywhere from 1
    // up to MAXLINE-1 bytes; we keep calling it until it returns 0
    // (peer closed the connection) or a negative value (error).
    std::vector<char> recvbuf(MAXLINE);
    ssize_t n;
    while ((n = read(sock.fd(), recvbuf.data(), MAXLINE - 1)) > 0)
    {
        // Bug fix vs. the original: the original only zeroed
        // recvline once, *before* the loop. If a later read()
        // returned fewer bytes than a previous one, printf("%s", ...)
        // would keep printing until it hit the null terminator left
        // over from that earlier, longer read -- i.e. it could print
        // stale trailing bytes from the previous iteration.
        // Null-terminating at exactly `n` here (the actual number of
        // bytes just read) is correct and avoids that.
        recvbuf[static_cast<size_t>(n)] = '\0';
        std::fputs(recvbuf.data(), stdout);
    }

    if (n < 0)
    {
        err_n_die("read error");
    }

    return 0; // successful completion
}