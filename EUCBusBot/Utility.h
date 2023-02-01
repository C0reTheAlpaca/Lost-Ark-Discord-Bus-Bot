#pragma once
#include <regex>
#include <dpp/dpp.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

class Utility
{
public:
	static bool ConvertToRaidQueryName(std::string& Raid, const bool ShortName);
	static void UpdateDriverRole(const dpp::snowflake GuildID, const dpp::snowflake Driver, const std::string RaidShortName, const bool Wipe);
	static void AddUpdateDriver(const bool UpdateLastRaidTime, const dpp::snowflake Driver);
	static void ReplyError(const dpp::interaction_create_t& event, std::string Message);
	static void GetRaidIndexShortName(const std::string Destination, int& Index, std::string& ShortName);
	static bool IsSubRaid(const std::string Destination, std::string& RaidName);
	static std::string ToLower(std::string String);
	static std::vector<dpp::snowflake> FilterMentions(std::string String);
	static std::time_t GetLastWeeklyResetTime();
	static std::unique_ptr<sql::ResultSet> GetDriver(dpp::snowflake Driver);
	static std::vector<std::string> SplitString(std::string String, const std::string& Delimiter);
};