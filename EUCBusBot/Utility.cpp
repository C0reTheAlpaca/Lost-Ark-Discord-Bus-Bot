#include "Utility.h"
#include "ConfigManager.h"
#include "Bot.h"

std::vector<dpp::snowflake> Utility::FilterMentions(std::string String)
{
	std::smatch Match;
	std::vector<dpp::snowflake> Mentions;
	const std::regex MentionRegex("<@!?(\\d+)>");

	while (std::regex_search(String, Match, MentionRegex))
	{
		Mentions.push_back(dpp::snowflake(Match[1]));
		String = Match.suffix();
	}

	return Mentions;
}

bool Utility::IsValidRaid(std::string Raid, bool Short)
{
	auto IsType = [&](Destination Destination)
	{
		std::string DestinationName = Short ? Destination.ShortName : Destination.Name;
		std::string RaidName = Raid;

		std::transform(DestinationName.begin(), DestinationName.end(), DestinationName.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);

		std::transform(RaidName.begin(), RaidName.end(), RaidName.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);

		return DestinationName == RaidName;
	};

	return std::find_if(g_pConfig->m_Destinations.begin(), g_pConfig->m_Destinations.end(), IsType) != g_pConfig->m_Destinations.end();
}

std::vector<std::string> Utility::SplitString(std::string String, const std::string& Delimiter)
{
	std::vector<std::string> Strings;
	std::size_t Position = 0;
	std::string Token;

	while ((Position = String.find(Delimiter)) != std::string::npos)
	{
		Token = String.substr(0, Position);
		Strings.push_back(Token);
		String.erase(0, Position + Delimiter.length());
	}

	return Strings;
}

std::time_t Utility::GetLastWeeklyResetTime()
{
	std::time_t SomePastResetDay = 1663149600; // (GMT): Wednesday, September 14, 2022 10:00:00 
	std::time_t Time = std::time(nullptr);
	std::time_t SinceReset = (Time - SomePastResetDay) % (7L * 24 * 60 * 60);

	return Time - SinceReset;
}

void Utility::UpdateDriverRole(const dpp::snowflake GuildID, const dpp::snowflake Driver, const std::string Raid, const bool Wipe)
{
	dpp::guild_member Member = g_pCluster->guild_get_member_sync(GuildID, Driver);
	std::string RaidShortName = "";

	for (auto It = g_pConfig->m_Destinations.begin(); It != g_pConfig->m_Destinations.end(); It++)
	{
		if (It->Name == Raid)
			RaidShortName = It->ShortName;
	}

	// Not sure if this can happen but better be safe
	if (Member.user_id == (dpp::snowflake)0)
		return;

	if (!g_pConfig->m_RaidAchievements.count(RaidShortName))
		return;

	std::vector<int> Thresholds = g_pConfig->m_RaidAchievements[RaidShortName].Thresholds;

	std::array<std::vector<uint64_t>, 2> RoleSets = {
		 g_pConfig->m_RaidAchievements[RaidShortName].Roles,
		 g_pConfig->m_RaidAchievements[RaidShortName].TierRoles
	};

	// Get runs of driver
	std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
		"SELECT SUBSTRING_INDEX(Type, ' ', 1) as short_type, SUM(Count1) + SUM(Count2) + SUM(Count3) + SUM(Count4)"
		"FROM `driver_completed_runs` WHERE DriverID = ? GROUP BY short_type;"
	));

	pStmt->setInt64(1, Driver);
	std::unique_ptr<sql::ResultSet> pRunResults(pStmt->executeQuery());

	// Store individual runs
	std::map<std::string, int> RunResults;

	int HighestCount = 0;

	while (pRunResults->next())
	{
		int Count = pRunResults->getInt(2);

		if (Count > HighestCount)
			HighestCount = Count;

		RunResults[pRunResults->getString(1)] = Count;
	}

	for (int i = 0; i < 2; i++)
	{
		bool Found = false;
		for (auto It = RoleSets[i].rbegin(); It != RoleSets[i].rend(); ++It)
		{
			int Index = std::distance(It, RoleSets[i].rend()) - 1;

			// Use raid specific count or highest count of all raids depending on role set
			int Count = i == 0 ? RunResults[Raid] : HighestCount;

			if (Found || HighestCount < Thresholds[Index])
			{
				if (std::find(Member.roles.begin(), Member.roles.end(), *It) != Member.roles.end())
					g_pCluster->guild_member_delete_role(Member.guild_id, Member.user_id, *It);

				continue;
			}

			if (HighestCount >= Thresholds[Index])
			{
				uint64_t Role = *It;

				if (std::find(Member.roles.begin(), Member.roles.end(), Role) == Member.roles.end())
				{
					g_pCluster->guild_member_add_role(Member.guild_id, Member.user_id, Role);
				}

				Found = true;
			}
		}
	}
}

void Utility::AddUpdateDriver(const bool UpdateLastRaidTime, const dpp::snowflake Driver)
{
	if (UpdateLastRaidTime)
	{
		std::time_t LastRaidTime = 0;

		// Get the last time the driver reported a raid
		std::unique_ptr<sql::PreparedStatement>pStmt(g_pConnection->prepareStatement(
			"SELECT * FROM drivers WHERE ID = ?;"
		));

		pStmt->setInt64(1, Driver);
		std::unique_ptr<sql::ResultSet>pResult(pStmt->executeQuery());

		if (pResult->next())
		{
			LastRaidTime = pResult->getInt64("LastRaidTime");
		}

		// Check how frequently the driver was part of a raid
		double Seconds = std::difftime(std::time(nullptr), LastRaidTime);
		double Minutes = std::floor(Seconds / 60);

		if (LastRaidTime && Minutes < 5)
		{
			std::string Time = Minutes ? std::to_string((int)Minutes) + " minute(s)" : std::to_string((int)Seconds) + " second(s)";

			// Setup warning
			dpp::embed Embed;
			Embed.set_color(g_pConfig->m_MessageColorWarning);
			Embed.set_description("<@" + std::to_string(Driver) + "> repeatedly reported rides within " + Time + ".");
			Embed.set_timestamp(std::time(nullptr));

			// Send warning to admin channel
			g_pCluster->message_create(dpp::message(g_pConfig->m_AdminChannelID, Embed));
		}

		// Update time
		pStmt.reset(g_pConnection->prepareStatement(
			"INSERT INTO drivers(ID) VALUES(?) ON DUPLICATE KEY UPDATE LastRaidTime = ?;"
		));

		pStmt->setInt64(1, Driver);
		pStmt->setInt64(2, std::time(nullptr));
		pStmt->execute();
	}
	else
	{
		// insert or ignore
		std::unique_ptr<sql::PreparedStatement>pStmt(g_pConnection->prepareStatement(
			"INSERT INTO drivers(ID) VALUES(?) ON DUPLICATE KEY UPDATE LastRaidTime = LastRaidTime;"
		));

		pStmt->setInt64(1, Driver);
		pStmt->execute();
	}
}

std::unique_ptr<sql::ResultSet> Utility::GetDriver(dpp::snowflake Driver)
{
	// Add driver if doesn't exist
	Utility::AddUpdateDriver(false, Driver);

	// Get driver
	std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
		"SELECT * FROM drivers WHERE ID = ?;"
	));

	pStmt->setInt64(1, Driver);
	std::unique_ptr<sql::ResultSet> pDriverResults(pStmt->executeQuery());

	pDriverResults->next();

	return pDriverResults;
}

void Utility::ReplyError(const dpp::interaction_create_t& event, std::string Message)
{
	dpp::embed Embed;

	Embed.set_description(Message);
	Embed.set_color(g_pConfig->m_MessageColorWarning);

	event.reply(dpp::message().add_embed(Embed));
}