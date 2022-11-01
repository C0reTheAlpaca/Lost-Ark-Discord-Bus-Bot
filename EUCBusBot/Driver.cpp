#include "Driver.h"
#include "Utility.h"
#include "ConfigManager.h"

void Driver::InfoGeneral(const dpp::snowflake Driver, const dpp::interaction_create_t& event)
{
	dpp::embed Embed;
	dpp::message Message;
	std::unique_ptr<sql::ResultSet> pDriverResults = Utility::GetDriver(Driver);

	// Get runs of driver
	std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
		"SELECT SUBSTRING_INDEX(Type, ' ', 1) as short_type, SUM(Count1) + SUM(Count2) + SUM(Count3) + SUM(Count4) FROM "
		"`driver_completed_runs` WHERE DriverID = ? GROUP BY short_type;"
	));

	pStmt->setInt64(1, Driver);

	// Store individual runs
	std::map<std::string, int> RunData;
	std::unique_ptr<sql::ResultSet> pRunResults(pStmt->executeQuery());

	while (pRunResults->next())
	{
		RunData[pRunResults->getString(1)] = pRunResults->getInt(2);
	}

	std::size_t TotalRuns = std::accumulate(
		RunData.begin(), RunData.end(), 0,
		[](int Value, const std::map<std::string, int>::value_type& Entry) {
			return Value + Entry.second;
		}
	);

	// Get user from ID
	dpp::user DriverUser;
	DriverUser.id = 0;

	auto It = event.command.resolved.users.find(Driver);

	if (It != event.command.resolved.users.end()) {
		DriverUser = It->second;
	}
	else return;

	// Create message
	dpp::embed_author Author;
	Author.name = DriverUser.format_username();
	Author.icon_url = DriverUser.get_avatar_url();

	Embed.set_author(Author);
	Embed.set_description("Overview of the busdriver:");
	Embed.set_color(g_pConfig->m_MessageColor);
	Embed.add_field("Tag:", DriverUser.get_mention(), true);
	Embed.add_field("\u200B", "\u200B", true);
	Embed.add_field("Busdriver since:", pDriverResults->getString("BusLicenseCreationDate"), true);


	Embed.add_field("Total Run(s):", std::to_string(TotalRuns), true);
	Embed.add_field("\u200B", "\u200B", true);
	Embed.add_field("Vouche(s) : ", pDriverResults->getString("Vouches"), true);

	Embed.add_field("Completed run(s):",
		"Argos: " + std::to_string(RunData["Argos"]) + "\n"
		"Valtan: " + std::to_string(RunData["Valtan"]) + "\n"
		"Vykas: " + std::to_string(RunData["Vykas"]) + "\n",
		true);
	Embed.add_field("\u200B", "\u200B", true);

	Embed.add_field("\u200B",
		"Kakul: " + std::to_string(RunData["Kakul-Saydon"]) + "\n"
		"Brelshaza: " + std::to_string(RunData["Brelshaza"]) + "\n"
		"Akkan: " + std::to_string(RunData["Akkan"]) + "\n",
		true);

	Message.add_embed(Embed);
	event.reply(Message);
}

void Driver::InfoRaid(const dpp::snowflake Driver, const std::string Raid, const dpp::interaction_create_t& event)
{
	dpp::embed Embed;
	dpp::message Message;

	// Get user from ID
	dpp::user DriverUser;
	auto It = event.command.resolved.users.find(Driver);

	if (It != event.command.resolved.users.end()) {
		DriverUser = It->second;
	}
	else return;

	// Get runs of driver
	std::unique_ptr<sql::PreparedStatement>pStmt(g_pConnection->prepareStatement(
		"SELECT * FROM `driver_completed_runs` WHERE DriverID = ? AND SUBSTRING_INDEX(Type, ' ', 1) = ?;"
	));

	pStmt->setInt64(1, Driver);
	pStmt->setString(2, Raid);

	// Store individual runs
	std::unique_ptr<sql::ResultSet> pRunResults(pStmt->executeQuery());

	while (pRunResults->next())
	{
		Embed.add_field(
			pRunResults->getString("Type"),
			"1 driver(s): " + std::to_string(pRunResults->getInt("Count1")) + "\n"
			"2 driver(s): " + std::to_string(pRunResults->getInt("Count2")) + "\n"
			"3 driver(s): " + std::to_string(pRunResults->getInt("Count3")) + "\n"
			"4 driver(s): " + std::to_string(pRunResults->getInt("Count4"))
			, true
		);
	}

	// Create message
	dpp::embed_author Author;
	Author.name = DriverUser.format_username();
	Author.icon_url = DriverUser.get_avatar_url();

	Embed.set_author(Author);
	Embed.set_description(Raid + " raid stats of the driver:");
	Embed.set_color(g_pConfig->m_MessageColor);
	Message.add_embed(Embed);

	event.reply(Message);
}

void Driver::VouchDriver(const dpp::interaction_create_t& event)
{
	dpp::embed Embed;
	dpp::message Message;
	dpp::command_value UserValue = event.get_parameter("driver");
	dpp::snowflake* pDriver = std::get_if<dpp::snowflake>(&UserValue);

	Message.set_flags(dpp::m_ephemeral);

	if (!pDriver)
	{
		Utility::ReplyError(event, "Invalid driver specified.");
		return;
	}

	Utility::AddUpdateDriver(false, *pDriver);

	// Return if self vouch
	if (*pDriver == event.command.get_issuing_user().id)
	{
		Utility::ReplyError(event, "You can't vouch for yourself.");
		return;
	}

	// Modify points of driver & update time
	std::unique_ptr<sql::PreparedStatement>pStmt(g_pConnection->prepareStatement(
		"INSERT INTO vouches (Voucher, VouchedUser, Time) VALUES (?, ?, ?)"
	));

	pStmt->setUInt64(1, event.command.get_issuing_user().id);
	pStmt->setUInt64(2, *pDriver);
	pStmt->setUInt64(3, std::time(nullptr));

	try
	{
		pStmt->execute();
	}
	catch (sql::SQLException& Exception)
	{
		if (Exception.getErrorCode() == 1062) // DUPLICATE KEY
			Utility::ReplyError(event, "You can only vouch for this user once.");

		return;
	}


	pStmt.reset(g_pConnection->prepareStatement(
		"UPDATE drivers SET Vouches = (SELECT COUNT(*) FROM vouches WHERE VouchedUser = ?) WHERE ID = ?"
	));

	pStmt->setUInt64(1, *pDriver);
	pStmt->setUInt64(2, *pDriver);
	pStmt->execute();

	Embed.set_description("Vouch submitted.");
	Embed.set_color(g_pConfig->m_MessageColor);
	Message.add_embed(Embed);

	event.reply(Message);
}

void Driver::ViewDriverInfo(const dpp::interaction_create_t& event, dpp::snowflake ContextDriver)
{
	dpp::command_value DriverValue = event.get_parameter("driver");
	dpp::command_value RaidValue = event.get_parameter("raid");
	dpp::snowflake* pDriver = std::get_if<dpp::snowflake>(&DriverValue);
	std::string* pRaid = std::get_if<std::string>(&RaidValue);

	if (ContextDriver)
		pDriver = new dpp::snowflake(ContextDriver);

	if (!pDriver)
	{
		Utility::ReplyError(event, "Invalid driver specified.");
		return;
	}

	if (pRaid)
	{
		if (!Utility::IsValidRaid(*pRaid, true))
		{
			Utility::ReplyError(event, "Invalid raid type specified.");
			return;
		}

		InfoRaid(*pDriver, *pRaid, event);

		return;
	}

	InfoGeneral(*pDriver, event);

	if (ContextDriver)
		delete pDriver;
}

void Driver::ModifyRuns(const dpp::interaction_create_t& event, bool ModifyCap, bool Add)
{
	dpp::embed Embed;
	dpp::message Message;
	dpp::command_value UserValue = event.get_parameter("driver");
	dpp::snowflake* pDriver = std::get_if<dpp::snowflake>(&UserValue);
	dpp::command_interaction CommandData = std::get<dpp::command_interaction>(event.command.data);

	Message.set_flags(dpp::m_ephemeral);

	if (!pDriver)
	{
		Utility::ReplyError(event, "Invalid driver specified.");
		return;
	}

	Utility::AddUpdateDriver(false, *pDriver);

	dpp::command_value RaidValue = event.get_parameter("raid");
	dpp::command_value AmountValue = event.get_parameter("amount");
	dpp::command_value DriverCountValue = event.get_parameter("driver_count");

	int64_t* pAmount = std::get_if<int64_t>(&AmountValue);
	int64_t* pDriverCount = std::get_if<int64_t>(&DriverCountValue);
	std::string* pRaid = std::get_if<std::string>(&RaidValue);

	if (!pAmount || (!ModifyCap && !pDriverCount) || !pRaid)
		return;

	if (*pDriverCount > 4 || *pDriverCount < 1)
	{
		Utility::ReplyError(event, "Invalid driver count.");
		return;
	}

	if (!Utility::IsValidRaid(*pRaid, false))
	{
		Utility::ReplyError(event, "Invalid raid type specified.");
		return;
	}

	// Get info of driver about this raid
	std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
		"SELECT * FROM `driver_completed_runs` WHERE DriverID = ? AND Type = ?;"
	));

	pStmt->setInt64(1, *pDriver);
	pStmt->setString(2, *pRaid);

	std::unique_ptr<sql::ResultSet> pRaidResults(pStmt->executeQuery());

	// Insert if this is the first raid of this type for this driver
	if (!pRaidResults->next())
	{
		std::string DriverCount = std::to_string(*pDriverCount);

		pStmt.reset(g_pConnection->prepareStatement(
			"INSERT IGNORE INTO driver_completed_runs (DriverID, Type, Count" + DriverCount + ", CountThisWeek, LastRaidTime) VALUES (?, ?, 0, 0, ?);"
		));

		pStmt->setInt64(1, *pDriver);
		pStmt->setString(2, *pRaid);
		pStmt->setInt64(3, std::time(nullptr));
		pStmt->execute();
	}

	if (ModifyCap)
	{
		std::string Query = "UPDATE driver_completed_runs SET CountThisWeek = CountThisWeek + ? WHERE DriverID = ? AND LOWER(Type) LIKE LOWER(?);";

		if (!Add) {
			Query = "UPDATE driver_completed_runs SET CountThisWeek = CountThisWeek - ? WHERE DriverID = ? AND LOWER(Type) LIKE LOWER(?);";
		}

		pStmt.reset(g_pConnection->prepareStatement(
			Query
		));

	} else {
		std::string DriverCount = std::to_string(*pDriverCount);
		std::string Query = "UPDATE driver_completed_runs SET Count" + DriverCount + " = Count" + DriverCount + " + ? WHERE DriverID = ? AND LOWER(Type) LIKE LOWER(?);";

		if (!Add) {
			Query = "UPDATE driver_completed_runs SET Count" + DriverCount + " = Count" + DriverCount + " - ? WHERE DriverID = ? AND LOWER(Type) LIKE LOWER(?);";
		}

		pStmt.reset(g_pConnection->prepareStatement(
			Query
		));
	}

	pStmt->setInt(1, *pAmount);
	pStmt->setInt64(2, *pDriver);
	pStmt->setString(3, *pRaid);
	pStmt->execute();

	Utility::UpdateDriverRole(event.command.guild_id, *pDriver, *pRaid, false);

	Embed.set_description(ModifyCap ? "Driver run cap adjusted." : "Driver runs modified.");
	Embed.set_color(g_pConfig->m_MessageColor);
	Message.add_embed(Embed);

	event.reply(Message);
}

void Driver::WipeRuns(const dpp::interaction_create_t& event)
{
	dpp::embed Embed;
	dpp::message Message;
	dpp::command_value RaidValue = event.get_parameter("raid");
	dpp::command_value UserValue = event.get_parameter("driver");
	std::string* pRaid = std::get_if<std::string>(&RaidValue);
	dpp::snowflake* pDriver = std::get_if<dpp::snowflake>(&UserValue);

	Message.set_flags(dpp::m_ephemeral);

	if (!pDriver)
	{
		Utility::ReplyError(event, "Invalid driver specified.");
		return;
	}

	Utility::AddUpdateDriver(false, *pDriver);

	if (pRaid && !Utility::IsValidRaid(*pRaid, false))
	{
		Utility::ReplyError(event, "Invalid raid type specified.");
		return;
	}

	if (pRaid)
	{
		// Get info of driver about this raid
		std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
			"SELECT * FROM `driver_completed_runs` WHERE DriverID = ? AND Type = ?;"
		));

		pStmt->setInt64(1, *pDriver);
		pStmt->setString(2, *pRaid);
		std::unique_ptr<sql::ResultSet> pRaidResults(pStmt->executeQuery());

		// Insert if this is the first raid of this type for this driver
		if (!pRaidResults->next())
		{
			Utility::ReplyError(event, "The driver has no raids to wipe.");
			return;
		}

		pStmt.reset(g_pConnection->prepareStatement(
			"UPDATE driver_completed_runs SET Count1 = 0, Count2 = 0, Count3 = 0, Count4 = 0 WHERE DriverID = ? AND Type = ?;"
		));

		pStmt->setInt64(1, *pDriver);
		pStmt->setString(2, *pRaid);
		pStmt->execute();

		Utility::UpdateDriverRole(event.command.guild_id, *pDriver, *pRaid, true);
	}
	else
	{
		std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
			"UPDATE driver_completed_runs SET Count1 = 0, Count2 = 0, Count3 = 0, Count4 = 0 WHERE DriverID = ?;"
		));

		pStmt->setInt64(1, *pDriver);
		pStmt->execute();

		Utility::UpdateDriverRole(event.command.guild_id, *pDriver, "", true);
	}

	Embed.set_description("Driver runs wiped.");
	Embed.set_color(g_pConfig->m_MessageColor);
	Message.add_embed(Embed);

	event.reply(Message);
}