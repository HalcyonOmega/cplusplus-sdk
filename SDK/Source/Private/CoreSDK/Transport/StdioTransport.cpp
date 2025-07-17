#include "CoreSDK/Transport/StdioTransport.h"

#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

#include "CoreSDK/Common/RuntimeError.h"

MCP_NAMESPACE_BEGIN

// StdioClientTransport Implementation
StdioClientTransport::StdioClientTransport(StdioClientTransportOptions InOptions) : m_Options(std::move(InOptions)) {}

StdioClientTransport::~StdioClientTransport()
{
	if (GetState() != ETransportState::Disconnected)
	{
		try
		{
			Disconnect().await_resume();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

VoidTask StdioClientTransport::Connect()
{
	if (GetState() != ETransportState::Disconnected)
	{
		HandleRuntimeError("Transport already started or in progress");
		co_return;
	}

	try
	{
		SetState(ETransportState::Connecting);

		// Create pipes for communication
		m_StdinPipe = std::make_unique<Poco::Pipe>();
		m_StdoutPipe = std::make_unique<Poco::Pipe>();
		if (m_Options.UseStderr)
		{
			m_StderrPipe = std::make_unique<Poco::Pipe>();
		}

		// Prepare process arguments
		const std::vector<std::string> args = m_Options.Arguments;

		// Launch the process
		if (m_Options.UseStderr)
		{
			m_ProcessHandle = std::make_unique<Poco::ProcessHandle>(Poco::Process::launch(m_Options.Command,
				args,
				m_StdinPipe.get(),
				m_StdoutPipe.get(),
				m_StderrPipe.get()));
		}
		else
		{
			m_ProcessHandle = std::make_unique<Poco::ProcessHandle>(Poco::Process::launch(m_Options.Command,
				args,
				m_StdinPipe.get(),
				m_StdoutPipe.get(),
				m_StdoutPipe.get()));
		}

		// Create streams
		m_StdinStream = std::make_unique<Poco::PipeOutputStream>(*m_StdinPipe);
		m_StdoutStream = std::make_unique<Poco::PipeInputStream>(*m_StdoutPipe);
		if (m_Options.UseStderr)
		{
			m_StderrStream = std::make_unique<Poco::PipeInputStream>(*m_StderrPipe);
		}

		// Start reading thread
		m_ShouldStop = false;
		// TODO: @HalcyonOmega - Thread should start automatically
		// m_ReadThread.start(*this);

		SetState(ETransportState::Connected);
	}
	catch (const std::exception& e)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Failed to start stdio transport: " + std::string(e.what()));
		co_return;
	}

	co_return;
}

VoidTask StdioClientTransport::Disconnect()
{
	if (GetState() == ETransportState::Disconnected)
	{
		co_return;
	}

	try
	{
		m_ShouldStop = true;

		// Close streams to wake up reading thread
		if (m_StdinStream)
		{
			m_StdinStream->close();
		}

		// Wait for the reading thread to finish
		if (m_ReadThread.joinable())
		{
			m_ReadThread.join();
		}

		Cleanup();
		SetState(ETransportState::Disconnected);
	}
	catch (const std::exception& e)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Error stopping stdio transport: " + std::string(e.what()));
	}

	co_return;
}

std::string StdioClientTransport::GetConnectionInfo() const { return "Stdio transport to: " + m_Options.Command; }

// TODO: @HalcyonOmega Cleanup
void StdioClientTransport::ReaderThread(std::stop_token InStopToken) {}

void StdioClientTransport::run() { ProcessIncomingData(); }

void StdioClientTransport::ProcessIncomingData()
{
	static constexpr std::chrono::milliseconds DEFAULT_SLEEP_FOR{ 10 };

	std::string line;
	while (!m_ShouldStop)
	{
		try
		{
			if (m_StdoutStream && m_StdoutStream->good())
			{
				std::getline(*m_StdoutStream, line);
				if (!line.empty())
				{
					ProcessLine(line);
				}
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SLEEP_FOR));
			}
		}
		catch (const std::exception& e)
		{
			if (!m_ShouldStop)
			{
				HandleRuntimeError("Error reading from process: " + std::string(e.what()));
			}
			break;
		}
	}
}

void StdioClientTransport::ProcessLine(const std::string& InLine) { CallMessageRouter(JSONData::parse(InLine)); }

void StdioClientTransport::TransmitMessage(const JSONData& InMessage,
	const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
{
	(void)InConnectionIDs;

	if (!m_StdinStream || !IsConnected())
	{
		HandleRuntimeError("Transport not connected");
		return;
	}

	try
	{
		std::lock_guard lock(m_WriteMutex);

		const std::string MessageStr = InMessage.dump() + "\n";
		m_StdinStream->write(MessageStr.c_str(), MessageStr.length());
		m_StdinStream->flush();
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Error writing message: " + std::string(e.what()));
	}
}

void StdioClientTransport::Cleanup()
{
	// Terminate the process if still running
	if (m_ProcessHandle)
	{
		try
		{
			Poco::Process::kill(*m_ProcessHandle);
		}
		catch (...)
		{
			// Ignore errors during cleanup
		}
	}

	// Close streams
	m_StdinStream.reset();
	m_StdoutStream.reset();
	m_StderrStream.reset();

	// Close pipes
	m_StdinPipe.reset();
	m_StdoutPipe.reset();
	m_StderrPipe.reset();

	m_ProcessHandle.reset();
}

// StdioServerTransport Implementation
StdioServerTransport::StdioServerTransport() {}

StdioServerTransport::~StdioServerTransport()
{
	if (GetState() != ETransportState::Disconnected)
	{
		try
		{
			Disconnect().await_resume();
		}
		catch (...)
		{
			// Ignore errors during destruction
		}
	}
}

VoidTask StdioServerTransport::Connect()
{
	if (GetState() != ETransportState::Disconnected)
	{
		HandleRuntimeError("Transport already started");
		co_return;
	}

	try
	{
		SetState(ETransportState::Connecting);

		// Server uses stdin/stdout directly
		m_ShouldStop = false;
		// TODO: @HalcyonOmega This should start automatically
		// m_ReadThread.start(*this);

		SetState(ETransportState::Connected);
	}
	catch (const std::exception& e)
	{
		SetState(ETransportState::Error);
	}

	co_return;
}

VoidTask StdioServerTransport::Disconnect()
{
	if (GetState() == ETransportState::Disconnected)
	{
		co_return;
	}

	try
	{
		m_ShouldStop = true;

		// Wait for the reading thread to finish
		if (m_ReadThread.joinable())
		{
			m_ReadThread.join();
		}

		SetState(ETransportState::Disconnected);
	}
	catch (const std::exception& e)
	{
		SetState(ETransportState::Error);
		HandleRuntimeError("Error stopping stdio server transport: " + std::string(e.what()));
	}

	co_return;
}

std::string StdioServerTransport::GetConnectionInfo() const { return "Stdio server transport (stdin/stdout)"; }

void StdioServerTransport::run() { ProcessIncomingData(); }

void StdioServerTransport::ProcessIncomingData()
{
	std::string line;
	while (!m_ShouldStop)
	{
		try
		{
			if (std::getline(std::cin, line))
			{
				if (!line.empty())
				{
					ProcessLine(line);
				}
			}
			else
			{
				// EOF reached
				break;
			}
		}
		catch (const std::exception& e)
		{
			if (!m_ShouldStop)
			{
				HandleRuntimeError("Error reading from stdin: " + std::string(e.what()));
			}
			break;
		}
	}
}

void StdioServerTransport::ProcessLine(const std::string& InLine) { CallMessageRouter(JSONData::parse(InLine)); }

void StdioServerTransport::TransmitMessage(const JSONData& InMessage,
	const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
{
	(void)InConnectionIDs;

	try
	{
		std::lock_guard lock(m_WriteMutex);

		const std::string MessageStr = InMessage.dump() + "\n";
		std::cout << MessageStr;
		std::cout.flush();
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Error writing message: " + std::string(e.what()));
	}
}

// Factory functions
std::unique_ptr<ITransport> CreateStdioClientTransportImpl(const StdioClientTransportOptions& InOptions)
{
	return std::make_unique<StdioClientTransport>(InOptions);
}

MCP_NAMESPACE_END