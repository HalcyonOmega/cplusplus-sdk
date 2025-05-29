#pragma once

#include "../Core/Common.hpp"

namespace MCP::Transport {

/**
 * Server transport for stdio: this communicates with a MCP client by reading from the current process' stdin and writing to stdout.
 *
 * This transport is only available in Node.js environments.
 */
class StdioServerTransport : public Transport {
  private _readBuffer: ReadBuffer = new ReadBuffer();
  private _started = false;

  constructor(
    private _stdin: Readable = process.stdin,
    private _stdout: Writable = process.stdout,
  ) {}

  onclose?: () => void;
  onerror?: (error: Error) => void;
  onmessage?: (message: JSON_RPC_Message) => void;

  // Arrow functions to bind `this` properly, while maintaining function identity.
  _ondata = (chunk: Buffer) => {
    this._readBuffer.append(chunk);
    this.processReadBuffer();
  };
  _onerror = (error: Error) => {
    this.onerror?.(error);
  };

  /**
   * Starts listening for messages on stdin.
   */
  async start(): Promise<void> {
    if (this._started) {
      throw new Error(
        "StdioServerTransport already started! If using Server class, note that connect() calls start() automatically.",
      );
    }

    this._started = true;
    this._stdin.on("data", this._ondata);
    this._stdin.on("error", this._onerror);
  }

  private processReadBuffer() {
    while (true) {
      try {
        const message = this._readBuffer.readMessage();
        if (message === null) {
          break;
        }

        this.onmessage?.(message);
      } catch (error) {
        this.onerror?.(error as Error);
      }
    }
  }

  async close(): Promise<void> {
    // Remove our event listeners first
    this._stdin.off("data", this._ondata);
    this._stdin.off("error", this._onerror);

    // Check if we were the only data listener
    const remainingDataListeners = this._stdin.listenerCount('data');
    if (remainingDataListeners === 0) {
      // Only pause stdin if we were the only listener
      // This prevents interfering with other parts of the application that might be using stdin
      this._stdin.pause();
    }

    // Clear the buffer and notify closure
    this._readBuffer.clear();
    this.onclose?.();
  }

  send(message: JSON_RPC_Message): Promise<void> {
    return new Promise((resolve) => {
      const json = serializeMessage(message);
      if (this._stdout.write(json)) {
        resolve();
      } else {
        this._stdout.once("drain", resolve);
      }
    });
  }
}

struct StdioServerParameters {
  /**
   * The executable to run to start the server.
   */
  command: string;

  /**
   * Command line arguments to pass to the executable.
   */
  args?: string[];

  /**
   * The environment to use when spawning the process.
   *
   * If not specified, the result of getDefaultEnvironment() will be used.
   */
  env?: Record<string, string>;

  /**
   * How to handle stderr of the child process. This matches the semantics of Node's `child_process.spawn`.
   *
   * The default is "inherit", meaning messages to stderr will be printed to the parent process's stderr.
   */
  stderr?: IOType | Stream | number;

  /**
   * The working directory to use when spawning the process.
   *
   * If not specified, the current working directory will be inherited.
   */
  cwd?: string;
};

/**
 * Environment variables to inherit by default, if an environment is not explicitly given.
 */
const DEFAULT_INHERITED_ENV_VARS =
  process.platform === "win32"
    ? [
        "APPDATA",
        "HOMEDRIVE",
        "HOMEPATH",
        "LOCALAPPDATA",
        "PATH",
        "PROCESSOR_ARCHITECTURE",
        "SYSTEMDRIVE",
        "SYSTEMROOT",
        "TEMP",
        "USERNAME",
        "USERPROFILE",
      ]
    : /* list inspired by the default env inheritance of sudo */
      ["HOME", "LOGNAME", "PATH", "SHELL", "TERM", "USER"];

/**
 * Returns a default environment object including only environment variables deemed safe to inherit.
 */
Record<string, string> GetDefaultEnvironment() {
  const env: Record<string, string> = {};

  for (const key of DEFAULT_INHERITED_ENV_VARS) {
    const value = process.env[key];
    if (value === undefined) {
      continue;
    }

    if (value.startsWith("()")) {
      // Skip functions, which are a security risk.
      continue;
    }

    env[key] = value;
  }

  return env;
}

/**
 * Client transport for stdio: this will connect to a server by spawning a process and communicating with it over stdin/stdout.
 *
 * This transport is only available in Node.js environments.
 */
class StdioClientTransport : public Transport {
  private _process?: ChildProcess;
  private _abortController: AbortController = new AbortController();
  private _readBuffer: ReadBuffer = new ReadBuffer();
  private _serverParams: StdioServerParameters;
  private _stderrStream: PassThrough | null = null;

  onclose?: () => void;
  onerror?: (error: Error) => void;
  onmessage?: (message: JSON_RPC_Message) => void;

  constructor(server: StdioServerParameters) {
    this._serverParams = server;
    if (server.stderr === "pipe" || server.stderr === "overlapped") {
      this._stderrStream = new PassThrough();
    }
  }

  /**
   * Starts the server process and prepares to communicate with it.
   */
  async start(): Promise<void> {
    if (this._process) {
      throw new Error(
        "StdioClientTransport already started! If using Client class, note that connect() calls start() automatically."
      );
    }

    return new Promise((resolve, reject) => {
      this._process = spawn(
        this._serverParams.command,
        this._serverParams.args ?? [],
        {
          env: this._serverParams.env ?? getDefaultEnvironment(),
          stdio: ["pipe", "pipe", this._serverParams.stderr ?? "inherit"],
          shell: false,
          signal: this._abortController.signal,
          windowsHide: process.platform === "win32" && isElectron(),
          cwd: this._serverParams.cwd,
        }
      );

      this._process.on("error", (error) => {
        if (error.name === "AbortError") {
          // Expected when close() is called.
          this.onclose?.();
          return;
        }

        reject(error);
        this.onerror?.(error);
      });

      this._process.on("spawn", () => {
        resolve();
      });

      this._process.on("close", (_code) => {
        this._process = undefined;
        this.onclose?.();
      });

      this._process.stdin?.on("error", (error) => {
        this.onerror?.(error);
      });

      this._process.stdout?.on("data", (chunk) => {
        this._readBuffer.append(chunk);
        this.processReadBuffer();
      });

      this._process.stdout?.on("error", (error) => {
        this.onerror?.(error);
      });

      if (this._stderrStream && this._process.stderr) {
        this._process.stderr.pipe(this._stderrStream);
      }
    });
  }

  /**
   * The stderr stream of the child process, if `StdioServerParameters.stderr` was set to "pipe" or "overlapped".
   *
   * If stderr piping was requested, a PassThrough stream is returned _immediately_, allowing callers to
   * attach listeners before the start method is invoked. This prevents loss of any early
   * error output emitted by the child process.
   */
  get stderr(): Stream | null {
    if (this._stderrStream) {
      return this._stderrStream;
    }

    return this._process?.stderr ?? null;
  }

  private processReadBuffer() {
    while (true) {
      try {
        const message = this._readBuffer.readMessage();
        if (message === null) {
          break;
        }

        this.onmessage?.(message);
      } catch (error) {
        this.onerror?.(error as Error);
      }
    }
  }

  async close(): Promise<void> {
    this._abortController.abort();
    this._process = undefined;
    this._readBuffer.clear();
  }

  send(message: JSON_RPC_Message): Promise<void> {
    return new Promise((resolve) => {
      if (!this._process?.stdin) {
        throw new Error("Not connected");
      }

      const json = serializeMessage(message);
      if (this._process.stdin.write(json)) {
        resolve();
      } else {
        this._process.stdin.once("drain", resolve);
      }
    });
  }
}

function isElectron() {
  return "type" in process;
}

} // namespace MCP::Transport