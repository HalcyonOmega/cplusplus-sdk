#pragma once

#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>

#include <future>
#include <mutex>
#include <thread>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Transport/ITransport.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

class StdioClientTransport final: public ITransport
{
public:
	explicit StdioClientTransport(const StdioClientTransportOptions& InOptions);
	~StdioClientTransport() noexcept override;

	// ITransport interface
	VoidTask Connect() override;
	VoidTask Disconnect() override;

	void TransmitMessage(
		const JSONData& InMessage, const std::optional<std::vector<ConnectionID>>& InConnectionIDs) override;

	std::string GetConnectionInfo() const override;

protected:
	void ReaderThread(std::stop_token InStopToken);

private:
	void ProcessIncomingData();
	void ProcessLine(const std::string& InLine);
	void Cleanup();

	StdioClientTransportOptions m_Options;
	std::unique_ptr<Poco::ProcessHandle> m_ProcessHandle;
	std::unique_ptr<Poco::Pipe> m_StdinPipe;
	std::unique_ptr<Poco::Pipe> m_StdoutPipe;
	std::unique_ptr<Poco::Pipe> m_StderrPipe;
	std::unique_ptr<Poco::PipeOutputStream> m_StdinStream;
	std::unique_ptr<Poco::PipeInputStream> m_StdoutStream;
	std::unique_ptr<Poco::PipeInputStream> m_StderrStream;

	bool m_ShouldStop{ false };
	std::jthread m_ReadThread;
	std::string m_Buffer;

	mutable std::mutex m_WriteMutex;
};

class StdioServerTransport final : public ITransport
{
public:
	StdioServerTransport();
	~StdioServerTransport() noexcept override;

	// ITransport interface
	VoidTask Connect() override;
	VoidTask Disconnect() override;

	void TransmitMessage(
		const JSONData& InMessage, const std::optional<std::vector<ConnectionID>>& InConnectionIDs) override;
	Task<JSONData> TransmitRequest(
		const JSONData& InRequest, const std::optional<std::vector<ConnectionID>>& InConnectionIDs) override;

	std::string GetConnectionInfo() const override;

protected:
	void ReaderThread(std::stop_token InStopToken);

private:
	void ProcessIncomingData();
	void ProcessLine(const std::string& InLine);

	std::jthread m_ReadThread;
	std::string m_Buffer;

	mutable std::mutex m_WriteMutex;
};

MCP_NAMESPACE_END