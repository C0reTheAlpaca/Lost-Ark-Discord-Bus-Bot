#pragma once
#include <dpp/dpp.h>
#include <vector>
#include <map>

class Driver;
class BusReport;
class ConfigManager;

class Command
{
public:
	Command(const dpp::slashcommand Command, const std::vector<dpp::command_option> Options)
	{
		m_Command = Command;
		m_Options = Options;
	}

	dpp::slashcommand m_Command;
	std::vector<dpp::command_option> m_Options;
};

class Bot
{
public:
	Bot();
	~Bot();
	void RegisterCommands();
	void RegisterEvents();

private:
	Driver* m_pDriver;
	BusReport* m_pBusReport;
};

extern dpp::cluster* g_pCluster;