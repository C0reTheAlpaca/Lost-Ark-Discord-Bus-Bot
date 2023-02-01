#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <dpp/nlohmann/json.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include "ReportRecord.h"

typedef nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> JsonItems;

enum class RaidType : int
{
	RAID_LEGION,
	RAID_ABYSS,
	RAID_DUNGEON,
};

struct Destination
{
	std::string Name;
	std::string ShortName;
	std::string DisplayName;
	std::string Thumbnail;
	std::vector<std::string> Aliases;
	RaidType Type;
	bool IsSubRaid;
};

struct RaidAchievements
{
	std::vector<int> Thresholds;
	std::vector<uint64_t> Roles;
	std::vector<uint64_t> TierRoles;
};

class ConfigManager
{
public:
	ConfigManager(std::string FileName);
	void Parse(std::string FileName);
	void AddRaid(const JsonItems& Raid, const std::string Name, const std::string DisplayName, const bool IsSubRaid);
	void CreateConfig(std::string FileName);

public:
	int m_MaxReportsPerWeek;
	int m_MessageColor, m_MessageColorWarning, m_MessageColorInfo;
	long m_ReportChannelID, m_AdminChannelID;

	dpp::snowflake m_GuildID;
	std::string m_BotToken;
	std::vector<Destination> m_Destinations;
	std::map<uint64_t, ReportRecord> m_Reports;
	std::map<std::string, RaidType> m_RaidLookup;
	std::map<std::string, RaidAchievements> m_RaidAchievements;

private:
	sql::ConnectOptionsMap m_DBConnection;
	nlohmann::ordered_json m_Config;
};

extern ConfigManager* g_pConfig;
extern sql::Connection* g_pConnection;