#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
    #include <fcntl.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
#else
    #define NOMINMAX
    #include <windows.h>
#endif

#include "Core.h"

#include <chrono>
#include <errno.h>

MCP_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// ChildProcess : Minimal cross-platform process wrapper. Currently fully
// implemented for POSIX platforms; on Windows it compiles as a no-op stub so
// that the rest of the SDK can build. Extend with a proper CreateProcessW
// implementation as needed.
//------------------------------------------------------------------------------
class ChildProcess {
  public:
    using DataCallback = std::function<void(const std::vector<uint8_t>&)>;

    struct Options {
        std::string Command;
        std::vector<std::string> Args;
        std::unordered_map<std::string, std::string> Environment;
        std::optional<std::string> WorkingDirectory;
        bool PipeStderr = false;
        DataCallback StdoutCallback;
        DataCallback StderrCallback;
    };

  private:
#ifndef _WIN32
    pid_t m_PID{-1};
    int m_StdinFD{-1};
    int m_StdoutFD{-1};
    int m_StderrFD{-1};
#else
    HANDLE m_ProcessHandle{nullptr};
    HANDLE m_StdinWrite{nullptr};
    HANDLE m_StdoutRead{nullptr};
    HANDLE m_StderrRead{nullptr};
#endif

    std::thread m_StdoutThread;
    std::thread m_StderrThread;
    std::atomic<bool> m_Running{false};

  public:
    ChildProcess() = default; // Default stub ctor

    explicit ChildProcess(const Options& opts) {
        Init(opts);
    }

    ~ChildProcess() {
        Terminate();
    }

    // Non-copyable, movable
    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;
    ChildProcess(ChildProcess&&) = delete;
    ChildProcess& operator=(ChildProcess&&) = delete;

    bool IsValid() const {
        return m_Running.load();
    }

    void Write(const std::string& data) {
#ifndef _WIN32
        if (m_StdinFD != -1) {
            const char* ptr = data.data();
            size_t remaining = data.size();
            while (remaining > 0) {
                ssize_t n = ::write(m_StdinFD, ptr, remaining);
                if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // pipe full, wait a bit
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                    break;
                }
                remaining -= static_cast<size_t>(n);
                ptr += n;
            }
        }
#else
        const char* ptr = data.data();
        size_t remaining = data.size();
        while (remaining > 0) {
            DWORD written = 0;
            if (!WriteFile(m_StdinWrite, ptr, static_cast<DWORD>(remaining), &written, nullptr)) {
                // If pipe is busy, wait
                if (GetLastError() == ERROR_NO_DATA || GetLastError() == ERROR_PIPE_BUSY) {
                    Sleep(5);
                    continue;
                }
                break;
            }
            remaining -= written;
            ptr += written;
        }
#endif
    }

    void Terminate() {
#ifndef _WIN32
        if (!m_Running.exchange(false)) { return; }

        // Close stdin to notify child
        if (m_StdinFD != -1) {
            ::close(m_StdinFD);
            m_StdinFD = -1;
        }

        // Wait for child process
        if (m_PID > 0) {
            int status = 0;
            ::waitpid(m_PID, &status, 0);
        }

        if (m_StdoutThread.joinable()) m_StdoutThread.join();
        if (m_StderrThread.joinable()) m_StderrThread.join();
#else
        if (!m_Running.exchange(false)) { return; }

        // Signal stdin EOF
        if (m_StdinWrite) {
            CloseHandle(m_StdinWrite);
            m_StdinWrite = nullptr;
        }

        if (m_ProcessHandle) {
            // Wait up to 5 seconds for the child to exit gracefully
            WaitForSingleObject(m_ProcessHandle, 5000);
            CloseHandle(m_ProcessHandle);
            m_ProcessHandle = nullptr;
        }

        if (m_StdoutThread.joinable()) m_StdoutThread.join();
        if (m_StderrThread.joinable()) m_StderrThread.join();
#endif
    }

    // Blocks until the child exits or timeoutMs (milliseconds) elapses. If timeoutMs is nullopt, waits indefinitely.
    bool WaitForExit(std::optional<int> timeoutMs = std::nullopt) {
#ifndef _WIN32
        if (m_PID <= 0) return true;
        int status = 0;
        if (timeoutMs) {
            // poll with sleep 50ms
            int elapsed = 0;
            while (elapsed < *timeoutMs) {
                pid_t res = ::waitpid(m_PID, &status, WNOHANG);
                if (res == m_PID) return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                elapsed += 50;
            }
            return false;
        } else {
            ::waitpid(m_PID, &status, 0);
            return true;
        }
#else
        if (!m_ProcessHandle) return true;
        DWORD wait = WaitForSingleObject(m_ProcessHandle, timeoutMs ? *timeoutMs : INFINITE);
        return wait == WAIT_OBJECT_0;
#endif
    }

  private:
#ifndef _WIN32
    static void ReaderLoop(int fd, const DataCallback& cb) {
        std::vector<uint8_t> buf(4096);
        while (true) {
            ssize_t n = ::read(fd, buf.data(), buf.size());
            if (n > 0) {
                cb({buf.begin(), buf.begin() + n});
            } else {
                break; // EOF or error
            }
        }
        ::close(fd);
    }

    void Init(const Options& opts) {
        // Build argv
        std::vector<char*> argvC;
        argvC.push_back(const_cast<char*>(opts.Command.c_str()));
        for (const auto& a : opts.Args) argvC.push_back(const_cast<char*>(a.c_str()));
        argvC.push_back(nullptr);

        // Create pipes
        int stdinPipe[2]{};
        int stdoutPipe[2]{};
        int stderrPipe[2]{};
        if (::pipe(stdinPipe) == -1 || ::pipe(stdoutPipe) == -1) {
            throw std::runtime_error("Failed to create I/O pipes");
        }
        if (opts.PipeStderr && ::pipe(stderrPipe) == -1) {
            throw std::runtime_error("Failed to create stderr pipe");
        }

        m_PID = ::fork();
        if (m_PID == -1) { throw std::runtime_error("fork() failed"); }

        if (m_PID == 0) {
            // ---- Child ----
            ::dup2(stdinPipe[0], STDIN_FILENO);
            ::dup2(stdoutPipe[1], STDOUT_FILENO);
            if (opts.PipeStderr) ::dup2(stderrPipe[1], STDERR_FILENO);

            // Close fds inherited from parent
            ::close(stdinPipe[1]);
            ::close(stdoutPipe[0]);
            if (opts.PipeStderr) ::close(stderrPipe[0]);

            // Build environment
            std::vector<std::string> envStr;
            std::vector<char*> envp;
            envStr.reserve(opts.Environment.size());
            envp.reserve(opts.Environment.size() + 1);
            for (const auto& [k, v] : opts.Environment) { envStr.emplace_back(k + "=" + v); }
            for (auto& s : envStr) envp.push_back(s.data());
            envp.push_back(nullptr);

            if (opts.WorkingDirectory) { ::chdir(opts.WorkingDirectory->c_str()); }

            ::execve(opts.Command.c_str(), argvC.data(), envp.data());
            _exit(127); // exec failed
        }

        // ---- Parent ----
        // Close unused ends
        ::close(stdinPipe[0]);
        ::close(stdoutPipe[1]);
        if (opts.PipeStderr) ::close(stderrPipe[1]);

        m_StdinFD = stdinPipe[1];
        m_StdoutFD = stdoutPipe[0];
        if (opts.PipeStderr) m_StderrFD = stderrPipe[0];

        m_Running = true;

        if (opts.StdoutCallback) {
            m_StdoutThread = std::thread(ReaderLoop, m_StdoutFD, opts.StdoutCallback);
        }
        if (opts.PipeStderr && opts.StderrCallback) {
            m_StderrThread = std::thread(ReaderLoop, m_StderrFD, opts.StderrCallback);
        }
    }
#else
    static void ReaderLoopWin(HANDLE hPipe, const DataCallback& cb) {
        std::vector<uint8_t> buf(4096);
        DWORD bytesRead = 0;
        while (ReadFile(hPipe, buf.data(), static_cast<DWORD>(buf.size()), &bytesRead, nullptr) &&
               bytesRead > 0) {
            cb({buf.begin(), buf.begin() + bytesRead});
        }
        CloseHandle(hPipe);
    }

    void Init(const Options& opts) {
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE stdinRead = nullptr;
        HANDLE stdoutWrite = nullptr;
        HANDLE stderrWrite = nullptr;

        if (!CreatePipe(&stdinRead, &m_StdinWrite, &sa, 0)) {
            throw std::runtime_error("CreatePipe stdin failed");
        }
        if (!SetHandleInformation(m_StdinWrite, HANDLE_FLAG_INHERIT, 0)) {
            throw std::runtime_error("SetHandleInformation failed");
        }

        HANDLE stdoutReadTmp = nullptr;
        if (!CreatePipe(&m_StdoutRead, &stdoutWrite, &sa, 0)) {
            throw std::runtime_error("CreatePipe stdout failed");
        }
        if (!SetHandleInformation(m_StdoutRead, HANDLE_FLAG_INHERIT, 0)) {
            throw std::runtime_error("SetHandleInformation failed");
        }

        if (opts.PipeStderr) {
            if (!CreatePipe(&m_StderrRead, &stderrWrite, &sa, 0)) {
                throw std::runtime_error("CreatePipe stderr failed");
            }
            SetHandleInformation(m_StderrRead, HANDLE_FLAG_INHERIT, 0);
        }

        // Build command line with robust quoting rules per Windows docs
        auto QuoteArg = [](const std::string& s) -> std::wstring {
            std::wstring ws(s.begin(), s.end());
            if (ws.empty()) return L"\"\"";

            bool needQuotes = ws.find_first_of(L" \t\"") != std::wstring::npos;
            if (!needQuotes) return ws;

            std::wstring result = L"\"";
            size_t backslashes = 0;
            for (wchar_t ch : ws) {
                if (ch == L'\\') {
                    backslashes++;
                } else if (ch == L'\"') {
                    result.append(backslashes * 2 + 1, L'\\');
                    result.push_back(L'\"');
                    backslashes = 0;
                } else {
                    result.append(backslashes, L'\\');
                    backslashes = 0;
                    result.push_back(ch);
                }
            }
            // Escape trailing backslashes
            result.append(backslashes * 2, L'\\');
            result.push_back(L'\"');
            return result;
        };

        std::wstring cmdLineW;
        cmdLineW += QuoteArg(opts.Command);
        for (const auto& a : opts.Args) {
            cmdLineW.push_back(L' ');
            cmdLineW += QuoteArg(a);
        }

        // Environment block
        std::wstring envBlock;
        for (const auto& [k, v] : opts.Environment) {
            std::wstring wkv(k.begin(), k.end());
            envBlock += wkv;
            envBlock += L'=';
            std::wstring wv(v.begin(), v.end());
            envBlock += wv;
            envBlock.push_back(L'\0');
        }
        envBlock.push_back(L'\0'); // double null terminator

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = stdinRead;
        si.hStdOutput = stdoutWrite;
        si.hStdError = opts.PipeStderr ? stderrWrite : GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION pi{};

        BOOL ok = CreateProcessW(nullptr, cmdLineW.data(), nullptr, nullptr, TRUE,
                                  0, envBlock.empty() ? nullptr : envBlock.data(),
                                  opts.WorkingDirectory ? std::wstring(opts.WorkingDirectory->begin(), opts.WorkingDirectory->end()).c_str() : nullptr,
                                  &si, &pi);
        CloseHandle(stdinRead);
        CloseHandle(stdoutWrite);
        if (opts.PipeStderr) CloseHandle(stderrWrite);

        if (!ok) {
            CloseHandle(m_StdinWrite);
            CloseHandle(m_StdoutRead);
            if (opts.PipeStderr) CloseHandle(m_StderrRead);
            throw std::runtime_error("CreateProcessW failed");
        }

        m_ProcessHandle = pi.hProcess;
        CloseHandle(pi.hThread);

        m_Running = true;

        if (opts.StdoutCallback) {
            m_StdoutThread = std::thread(ReaderLoopWin, m_StdoutRead, opts.StdoutCallback);
        }
        if (opts.PipeStderr && opts.StderrCallback) {
            m_StderrThread = std::thread(ReaderLoopWin, m_StderrRead, opts.StderrCallback);
        }
    }
#endif
};

using ChildProcessPtr = std::unique_ptr<ChildProcess>;

MCP_NAMESPACE_END