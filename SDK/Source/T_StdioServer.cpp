// main.cpp for a Poco-based Stdio Application
#include <iostream>
#include <string>
#include <vector>

#include "Poco/Exception.h"
#include "Poco/Util/Application.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"

// This class uses the Poco Application framework to structure a command-line
// tool that communicates over standard input and standard output.
class StdioApplication : public Poco::Util::Application
{
public:
	StdioApplication() : _helpRequested(false) {}

protected:
	// This method is used to initialize the application and its subsystems.
	void initialize(Application& self)
	{
		// Load any configuration files if necessary (not used in this example).
		loadConfiguration();
		Application::initialize(self);
	}

	// This method is used to define the command-line options the application accepts.
	void defineOptions(Poco::Util::OptionSet& options)
	{
		Application::defineOptions(options);

		// Add a "--help" option.
		options.addOption(Poco::Util::Option("help", "h", "Display help information.")
				.required(false)
				.repeatable(false)
				.callback(Poco::Util::OptionCallback<StdioApplication>(this, &StdioApplication::handleHelp)));
	}

	// Callback method for the "--help" option.
	void handleHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		// Stop further option processing.
		stopOptionsProcessing();
	}

	// The main entry point for the application's logic.
	// This is called by the framework after initialization and option parsing.
	int main(const std::vector<std::string>& args)
	{
		// If help was requested, display it and exit.
		if (_helpRequested)
		{
			Poco::Util::HelpFormatter helpFormatter(options());
			helpFormatter.setCommand(commandName());
			helpFormatter.setUsage("OPTIONS");
			helpFormatter.setHeader("A simple stdio echo application using the Poco framework.");
			helpFormatter.format(std::cout);
			return Application::EXIT_OK;
		}

		// --- Main Stdio Processing Loop ---
		// This is the core logic of your Model Context Protocol SDK.

		logger().information("Application started. Waiting for input on stdin...");

		std::string line;

		// The loop reads from standard input until the stream is closed (EOF).
		while (std::getline(std::cin, line))
		{
			// Log the received message using the Poco logger.
			logger().information("Received: " + line);

			// --- Your Protocol Logic Goes Here ---
			// For this example, we just echo the message back.
			std::string response = "ECHO: " + line;

			// Write the response to standard output and flush the buffer.
			std::cout << response << std::endl;
		}

		logger().information("Input stream closed. Shutting down.");

		return Application::EXIT_OK;
	}

private:
	bool _helpRequested;
};

// The standard C++ main function.
// It creates an instance of our application class and runs it.
int main(int argc, char** argv)
{
	try
	{
		StdioApplication app;
		return app.run();
	}
	catch (Poco::Exception& exc)
	{
		std::cerr << "A Poco exception occurred: " << exc.displayText() << std::endl;
		return Poco::Util::Application::EXIT_SOFTWARE;
	}
	catch (std::exception& exc)
	{
		std::cerr << "A standard exception occurred: " << exc.what() << std::endl;
		return Poco::Util::Application::EXIT_SOFTWARE;
	}
}
