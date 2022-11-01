#include "ConfigManager.h"
#include <fstream>
#include <filesystem>

ConfigManager* g_pConfig = nullptr;
sql::Connection* g_pConnection = nullptr;

ConfigManager::ConfigManager(std::string FileName)
{
	// Create default config if it doesn't exist yet
	if (!std::filesystem::exists(FileName))
		CreateConfig(FileName);

	// Load Config
	Parse(FileName);
}

void ConfigManager::CreateConfig(std::string FileName)
{
	nlohmann::ordered_json Json;

	Json["DBConnection"] = nlohmann::json::object();
	Json["DBConnection"]["Host"] = "172.30.144.1";
	Json["DBConnection"]["User"] = "root";
	Json["DBConnection"]["Password"] = "root";
	Json["DBConnection"]["Schema"] = "busbot";
	Json["DBConnection"]["Port"] = 3306;
	Json["DBConnection"]["Reconnect"] = true;

	Json["Settings"] = nlohmann::json::object();
	Json["Settings"]["GuildID"] = 1019322368935612596;
	Json["Settings"]["BotToken"] = "NI2F3QYTIMNgAxMTNAM0xDD5Oj.meLJdf.xJkMj_7R0cYXLFvcZt4qiVqGU7UNlhlxMM9nIv";
	Json["Settings"]["MessageColor"] = 0xFEE75C;
	Json["Settings"]["MessageColorInfo"] = 0xED7542;
	Json["Settings"]["MessageColorWarning"] = 0xED4245;
	Json["Settings"]["ReportChannelID"] = 1019322368935612599;
	Json["Settings"]["AdminChannelID"] = 1019322368935612599;
	Json["Settings"]["MaxReportsPerWeek"] = 6;

	Json["Raids"] = nlohmann::json::object();
	Json["Raids"]["Argos"]["Image"] = "https://i.imgur.com/GpVk1wu.png";
	Json["Raids"]["Valtan"]["Image"] = "https://i.imgur.com/ww9EZlM.png";
	Json["Raids"]["Vykas"]["Image"] = "https://i.imgur.com/zuRWTea.png";
	Json["Raids"]["Argos"]["Type"] = "RAID_ABYSS";
	Json["Raids"]["Valtan"]["Type"] = "RAID_LEGION";
	Json["Raids"]["Vykas"]["Type"] = "RAID_LEGION";
	Json["Raids"]["Valtan"]["Difficulties"][0] = "Normal";
	Json["Raids"]["Valtan"]["Difficulties"][1] = "Hard";
	Json["Raids"]["Vykas"]["Difficulties"][0] = "Normal";
	Json["Raids"]["Vykas"]["Difficulties"][1] = "Hard";
	Json["Raids"]["Argos"]["Thresholds"][0] = 10;
	Json["Raids"]["Argos"]["RoleIDs"][0] = 1020758523140907078;
	Json["Raids"]["Argos"]["TierRoleIDs"][0] = 1020758523140907078;

	std::string JsonString = Json.dump(4);

	std::ofstream Output(FileName);
	Output << JsonString;
	Output.close();
}

void ConfigManager::Parse(std::string FileName)
{
	std::ifstream Input(FileName);
	std::stringstream Buffer;
	Buffer << Input.rdbuf();

	m_Config = nlohmann::ordered_json::parse(Buffer.str());

	m_RaidLookup = {
		std::make_pair("RAID_ABYSS", RaidType::RAID_ABYSS),
		std::make_pair("RAID_LEGION", RaidType::RAID_LEGION),
		std::make_pair("RAID_DUNGEON", RaidType::RAID_DUNGEON),
	};

	// Load config
	m_GuildID = m_Config["Settings"]["GuildID"].get<uint64_t>();
	m_BotToken = m_Config["Settings"]["BotToken"].get<std::string>();
	m_MessageColor = m_Config["Settings"]["MessageColor"].get<int>();
	m_MessageColorInfo = m_Config["Settings"]["MessageColorInfo"].get<int>();
	m_MessageColorWarning = m_Config["Settings"]["MessageColorWarning"].get<int>();
	m_ReportChannelID = m_Config["Settings"]["ReportChannelID"].get<long>();
	m_AdminChannelID = m_Config["Settings"]["AdminChannelID"].get<long>();
	m_MaxReportsPerWeek = m_Config["Settings"]["MaxReportsPerWeek"].get<int>();

	m_DBConnection["hostName"] = m_Config["DBConnection"]["Host"].get<std::string>();
	m_DBConnection["userName"] = m_Config["DBConnection"]["User"].get<std::string>();
	m_DBConnection["password"] = m_Config["DBConnection"]["Password"].get<std::string>();
	m_DBConnection["schema"] = m_Config["DBConnection"]["Schema"].get<std::string>();
	m_DBConnection["port"] = m_Config["DBConnection"]["Port"].get<int>();
	m_DBConnection["OPT_RECONNECT"] = m_Config["DBConnection"]["Reconnect"].get<bool>();

	for (auto& Raid : m_Config["Raids"].items())
	{
		Destination NewDest;
		NewDest.Name = Raid.key();
		NewDest.ShortName = Raid.key();
		NewDest.Thumbnail = Raid.value()["Image"].get<std::string>();
		NewDest.Type = m_RaidLookup[Raid.value()["Type"].get<std::string>()];

		if (Raid.value().contains("Achievements") &&
			Raid.value()["Achievements"].contains("Thresholds") &&
			Raid.value()["Achievements"].contains("RoleIDs") &&
			Raid.value()["Achievements"].contains("TierRoleIDs"))
		{
			for (auto& Threshold : Raid.value()["Achievements"]["Thresholds"])
			{
				m_RaidAchievements[NewDest.ShortName].Thresholds.push_back(
					Threshold.get<int>()
				);
			}

			for (auto& RoleID : Raid.value()["Achievements"]["RoleIDs"])
			{
				m_RaidAchievements[NewDest.ShortName].Roles.push_back(
					RoleID.get<uint64_t>()
				);
			}

			for (auto& TierRoleID : Raid.value()["Achievements"]["TierRoleIDs"])
			{
				m_RaidAchievements[NewDest.ShortName].TierRoles.push_back(
					TierRoleID.get<uint64_t>()
				);
			}
		}

		if (Raid.value().contains("Difficulties"))
		{
			for (auto& Difficulty : Raid.value()["Difficulties"])
			{
				NewDest.Name = Raid.key() + std::string(" ") + Difficulty.get<std::string>();
				m_Destinations.push_back(NewDest);
			}
		}
		else
		{
			m_Destinations.push_back(NewDest);
			continue;
		}
	}

	// Setup MySQl connection
	sql::Driver* pDriver = get_driver_instance();

	try
	{
		g_pConnection = pDriver->connect(m_DBConnection);
	}
	catch (sql::SQLException& Exception)
	{
		std::cerr << __FILE__ << "(" << __FUNCTION__ << ":"
			<< __LINE__ << ") | e.what(): " << Exception.what() <<
			" (MySQL error code: " << Exception.getErrorCode() <<
			", SQLState: " << Exception.getSQLState() << " )" << std::endl;

		throw 0xDEADDEAD;
	}
}