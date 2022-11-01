#pragma once
#include <regex>
#include <dpp/dpp.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

class Utility
{
public:
	static std::vector<dpp::snowflake> FilterMentions(std::string String);
	static bool IsValidRaid(std::string Raid, bool Short);
	static std::vector<std::string> SplitString(std::string String, const std::string& Delimiter);
	static std::time_t GetLastWeeklyResetTime();
	static void UpdateDriverRole(const dpp::snowflake GuildID, const dpp::snowflake Driver, const std::string Raid, const bool Wipe);
	static void AddUpdateDriver(const bool UpdateLastRaidTime, const dpp::snowflake Driver);
	static std::unique_ptr<sql::ResultSet> GetDriver(dpp::snowflake Driver);
};