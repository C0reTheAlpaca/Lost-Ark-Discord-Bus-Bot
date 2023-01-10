#include "BusReport.h"
#include "ConfigManager.h"
#include "Utility.h"
#include "Bot.h"

dpp::message BusReport::GenerateReportForm(ReportRecord Record)
{
	dpp::embed Embed;
	dpp::message Message;
	dpp::component DestinationDropdown, DriverCountDropdown, Buttons;

	Message.set_flags(dpp::m_ephemeral);

	Embed.set_title("Report System Rules");
	Embed.set_description(

		"Make sure you have tagged every driver,\n"
		"if you didn't do so hit Cancel and re-do your report!\n\n"

		"When selecting the number of drivers,\n"
		"you have to select the number of drivers in the run,\n"
		"even if it isn't equal to the number of drivers tagged.\n\n"

		"If you submit your report and you filled something out wrong,\n"
		"you must open a ticket in #Contact - Staff in the server and tell us about it.\n"
		"Not doing so could be seen as cheating the system and result in punishment.\n\n"

		"You can only report 6 of each boss(6argos, 6 valtan, ...) every week.\n"
		"You cannot report more then that for each boss.\n"
	);

	Embed.set_color(g_pConfig->m_MessageColor);
	Embed.add_field("Busdriver(s)", Record.GetDrivers().GetEmbedList(), true);
	Embed.add_field("Passenger(s)", Record.GetPassengers().GetEmbedList(), true);

	dpp::component DestinationOptions;
	DestinationOptions.set_type(dpp::cot_selectmenu);
	DestinationOptions.set_placeholder("Select destination");
	DestinationOptions.set_id("busdestination");

	dpp::component DriverOptions;
	DriverOptions.set_type(dpp::cot_selectmenu);
	DriverOptions.set_placeholder("Select number of drivers");
	DriverOptions.set_id("drivercount");
	DriverOptions.add_select_option(dpp::select_option("1 driver(s)", "1"));
	DriverOptions.add_select_option(dpp::select_option("2 driver(s)", "2"));
	DriverOptions.add_select_option(dpp::select_option("3 driver(s)", "3"));
	DriverOptions.add_select_option(dpp::select_option("4 driver(s)", "4"));
	DriverOptions.add_select_option(dpp::select_option("5 driver(s)", "5"));
	DriverOptions.add_select_option(dpp::select_option("6 driver(s)", "6"));

	for (auto Destination : g_pConfig->m_Destinations)
	{
		DestinationOptions.add_select_option(dpp::select_option(Destination.DisplayName, Destination.Name));
	}

	DestinationDropdown.add_component(DestinationOptions);
	DriverCountDropdown.add_component(DriverOptions);

	Buttons.add_component(
		dpp::component().
		set_type(dpp::cot_button).
		set_label("Preview").
		set_style(dpp::cos_primary).
		set_id("createbus")
	);

	Buttons.add_component(
		dpp::component().
		set_type(dpp::cot_button).
		set_label("Cancel").
		set_style(dpp::cos_danger).
		set_id("crashbus")
	);

	Message.add_embed(Embed);
	Message.add_component(DestinationDropdown);
	Message.add_component(DriverCountDropdown);
	Message.add_component(Buttons);

	return Message;
}

dpp::message BusReport::GenerateReportMessage(const dpp::embed_author Author, const std::string Description, const int DestinationIndex, ReportRecord Record)
{
	dpp::embed Embed;
	dpp::message Message;

	// Setup server notice
	Embed.set_author(Author);
	Embed.set_color(g_pConfig->m_MessageColor);
	Embed.set_thumbnail(g_pConfig->m_Destinations[DestinationIndex].Thumbnail);
	Embed.set_description(Description);
	Embed.add_field("Busdriver(s)", Record.GetDrivers().GetEmbedList(), true);
	Embed.add_field("Passenger(s)", Record.GetPassengers().GetEmbedList(), true);
	Embed.add_field("Where did the bus go?", g_pConfig->m_Destinations[DestinationIndex].DisplayName, false);
	Embed.add_field("Number of drivers:", std::to_string(Record.GetDriverCount()), false);
	Embed.set_image(Record.GetAttachment());
	Embed.set_timestamp(time(0));

	Message.set_channel_id(g_pConfig->m_ReportChannelID);
	Message.add_embed(Embed);

	return Message;
}

void BusReport::CreateReportForm(const dpp::interaction_create_t& event)
{
	std::string ScreenshotURL;
	std::vector<dpp::snowflake> Drivers;
	std::vector<dpp::snowflake> Passengers;

	// Put issuing user into driver seat
	Drivers.push_back(event.command.get_issuing_user().id);

	dpp::command_value DriverValue = event.get_parameter("other_drivers");
	dpp::command_value PassengerValue = event.get_parameter("passengers");
	dpp::command_value AttachmentValue = event.get_parameter("screenshot");
	std::string* pString = std::get_if<std::string>(&DriverValue);

	if (pString)
	{
		std::vector<dpp::snowflake> OtherDrivers = Utility::FilterMentions(*pString);
		Drivers.insert(Drivers.end(), OtherDrivers.begin(), OtherDrivers.end());
	}

	pString = std::get_if<std::string>(&PassengerValue);

	if (pString)
		Passengers = Utility::FilterMentions(*pString);

	dpp::snowflake* pFileID = std::get_if<dpp::snowflake>(&AttachmentValue);

	if (!pFileID)
		return;

	// get screenshot url
	auto It = event.command.resolved.attachments.find(*pFileID);
	if (It != event.command.resolved.attachments.end()) {
		ScreenshotURL = It->second.url;
	}
	else return;

	// Remove duplicates from drivers & passengers to avoid cheating
	std::sort(Drivers.begin(), Drivers.end());
	std::sort(Passengers.begin(), Passengers.end());
	Drivers.erase(std::unique(Drivers.begin(), Drivers.end()), Drivers.end());
	Passengers.erase(std::unique(Passengers.begin(), Passengers.end()), Passengers.end());

	// Put issuing user into driver seat again
	auto DriverIt = std::find(Drivers.begin(), Drivers.end(), event.command.get_issuing_user().id);
	std::rotate(Drivers.begin(), DriverIt, Drivers.end());

	// Create a new record
	ReportRecord NewRecord(event.command.guild_id, Drivers, Passengers, ScreenshotURL);

	// Create form
	dpp::message Message = GenerateReportForm(NewRecord);
	NewRecord.SetMessage(Message);

	// Save record for issuing driver
	m_Reports[event.command.get_issuing_user().id] = NewRecord;

	event.reply(Message);
}

void BusReport::SetBusDestination(const dpp::select_click_t& event)
{
	event.reply();

	uint64_t ReportID = event.command.get_issuing_user().id;
	m_Reports[ReportID].SetDestination(event.values[0]);
}

void BusReport::SetBusDriverCount(const dpp::select_click_t& event)
{
	event.reply();

	std::string CountString = event.values[0];

	if (CountString.empty() || std::any_of(CountString.begin(), CountString.end(), ::isalpha))
		return;

	uint64_t ReportID = event.command.get_issuing_user().id;
	m_Reports[ReportID].SetDriverCount(std::stoi(CountString));
}

void BusReport::CancelReport(const dpp::button_click_t& event)
{
	event.reply();
	event.edit_original_response(dpp::message("https://tenor.com/view/the-simpsons-accident-bus-crash-explosion-gif-20418233"));

	uint64_t ReportID = event.command.get_issuing_user().id;
	m_Reports.erase(ReportID);
}

void BusReport::PreviewReport(const dpp::button_click_t& event)
{
	event.reply();

	dpp::component Buttons;
	dpp::user IssuingDriver = event.command.get_issuing_user();
	uint64_t ReportID = IssuingDriver.id;

	// Prevent response to forms with lost data due to bot restart
	if (IsOutdatedForm(ReportID, event))
		return;

	ReportRecord Record = m_Reports[ReportID];
	std::string Dest = Record.GetDestination();

	if (Dest.empty())
	{
		// Error message when destination select wasn't used
		event.edit_original_response(
			Record.GetMessage().add_embed(
				dpp::embed().set_description("Please select the ride destination.").set_color(g_pConfig->m_MessageColorWarning)
			)
		);

		event.reply();
		return;
	}

	int DestinationIndex;
	std::string RaidShortName;
	std::string RaidName = Record.GetDestination();

	Utility::GetRaidIndexShortName(RaidName, DestinationIndex, RaidShortName);

	// Create Author
	dpp::embed_author Author;
	Author.name = IssuingDriver.format_username() + " [PREVIEW]";
	Author.icon_url = IssuingDriver.get_avatar_url();
	Author.proxy_icon_url = IssuingDriver.get_avatar_url();
	Author.url = IssuingDriver.get_avatar_url();

	// Create preview message
	dpp::message Message = GenerateReportMessage(Author, "Has reported their bus ride:", DestinationIndex, Record);
	Message.set_flags(dpp::m_ephemeral);

	// Add buttons to preview
	Buttons.add_component(
		dpp::component().
		set_type(dpp::cot_button).
		set_label("Submit").
		set_style(dpp::cos_success).
		set_id("confirmbus")
	);

	Buttons.add_component(
		dpp::component().
		set_type(dpp::cot_button).
		set_label("Edit").
		set_style(dpp::cos_primary).
		set_id("editbus")
	);

	Buttons.add_component(
		dpp::component().
		set_type(dpp::cot_button).
		set_label("Cancel").
		set_style(dpp::cos_danger).
		set_id("crashbus")
	);

	Message.add_component(Buttons);

	dpp::embed_footer Footer;
	Footer.icon_url = "https://cdn.discordapp.com/emojis/780519656288288819.gif?size=48&quality=lossless";
	Footer.text = "Warning: Please make sure your report is correct. False reports can be punished!";
	Message.embeds[0].set_footer(Footer);
	Message.embeds[0].set_timestamp(0);

	// Edit original message
	event.edit_original_response(Message);
}

void BusReport::EditReport(const dpp::button_click_t& event)
{
	event.reply();

	uint64_t ReportID = event.command.get_issuing_user().id;

	if (IsOutdatedForm(ReportID, event))
		return;

	ReportRecord Record = m_Reports[ReportID];

	// resets message to original form message
	event.edit_original_response(Record.GetMessage());
}

void BusReport::ConfirmReport(const dpp::button_click_t& event)
{
	event.reply();

	dpp::user IssuingDriver = event.command.get_issuing_user();
	uint64_t ReportID = IssuingDriver.id;

	if (m_Reports[ReportID].WasSubmitted())
		return;

	m_Reports[ReportID].SetSubmitted();

	// Prevent response to forms with last data due to bot restart
	if (IsOutdatedForm(ReportID, event))
		return;

	ReportRecord Record = m_Reports[ReportID];
	std::string Dest = Record.GetDestination();

	int DestinationIndex;
	std::string RaidShortName;
	std::string RaidName = Record.GetDestination();

	Utility::GetRaidIndexShortName(RaidName, DestinationIndex, RaidShortName);
	std::string DriverCount = std::to_string(Record.GetDriverCount());

	for (dpp::snowflake Driver : Record.GetDrivers().m_Entries)
	{
		// Add driver to db or update last raid time
		Utility::AddUpdateDriver(true, Driver);

		// Get info of driver about this raid
		std::unique_ptr<sql::PreparedStatement> pStmt(g_pConnection->prepareStatement(
			"SELECT SUM(CountThisWeek) as CountTotal, MAX(LastRaidTime) as HighestRaidTime FROM `driver_completed_runs` "
			"WHERE DriverID = ? AND SUBSTRING_INDEX(Type, ' ', 1) = ? GROUP BY SUBSTRING_INDEX(Type, ' ', 1);"
		));

		pStmt->setInt64(1, Driver);
		pStmt->setString(2, RaidShortName);
		std::unique_ptr<sql::ResultSet> pResults(pStmt->executeQuery());

		// Get info of driver about this raid
		pStmt.reset(g_pConnection->prepareStatement(
			"SELECT * FROM `driver_completed_runs` WHERE DriverID = ? AND Type = ?;"
		));

		pStmt->setInt64(1, Driver);
		pStmt->setString(2, RaidName);
		std::unique_ptr<sql::ResultSet> pRaidResults(pStmt->executeQuery());

		// Insert if this is the first raid of this type for this driver
		if (!pRaidResults->next())
		{
			pStmt.reset(g_pConnection->prepareStatement(
				"INSERT IGNORE INTO driver_completed_runs (DriverID, Type, Count" + DriverCount + ", CountThisWeek, LastRaidTime) "
				"VALUES (?, ?, 1, 1, ?);"
			));

			pStmt->setInt64(1, Driver);
			pStmt->setString(2, RaidName);
			pStmt->setInt64(3, std::time(nullptr));
			pStmt->execute();

			continue;
		}

		if (pResults->next())
		{
			std::time_t TimeRaid = (std::time_t)pResults->getUInt64("HighestRaidTime");
			std::time_t TimeNow = std::time(nullptr);
			std::time_t ResetTime = Utility::GetLastWeeklyResetTime();
			std::tm* pResetTM = std::gmtime(&ResetTime);

			int RaidTypeCount = pResults->getInt("CountTotal");
			bool WeeklyReset = TimeRaid <= ResetTime;

			// Reset raid count if weekly reset happened
			if (WeeklyReset)
			{
				pStmt.reset(g_pConnection->prepareStatement(
					"UPDATE driver_completed_runs SET CountThisWeek = 0 WHERE DriverID = ? AND SUBSTRING_INDEX(Type, ' ', 1) = ?;"
				));

				pStmt->setInt64(1, Driver);
				pStmt->setString(2, RaidShortName);
				pStmt->execute();
			}

			// Skip this driver if he already was reported 6 times this week
			if (RaidTypeCount >= 6 && !WeeklyReset)
			{
				// Setup warning
				dpp::embed Embed;
				Embed.set_color(g_pConfig->m_MessageColorInfo);
				Embed.set_description("<@" + std::to_string(Driver) + "> was part of a reported raid in which he already reached the weekly cap.");
				Embed.set_timestamp(std::time(nullptr));

				// Send warning to admin channel
				g_pCluster->message_create(dpp::message(g_pConfig->m_AdminChannelID, Embed));

				continue;
			}

			// Update existing raid 
			pStmt.reset(g_pConnection->prepareStatement(
				"UPDATE driver_completed_runs SET Count" + DriverCount + " = Count" + DriverCount + " + 1, "
				"CountThisWeek = CountThisWeek + 1, LastRaidTime = ? WHERE DriverID = ? AND Type = ? ; "
			));

			pStmt->setInt64(1, std::time(nullptr));
			pStmt->setInt64(2, Driver);
			pStmt->setString(3, RaidName);
			pStmt->execute();
		}

		Utility::UpdateDriverRole(event.command.guild_id, Driver, RaidShortName, false);
	}

	// Create mentions
	std::string MentionString;
	dpp::component Buttons;
	std::vector<std::pair<dpp::user, dpp::guild_member >> Mentions;
	std::vector<dpp::snowflake> Roles;
	std::vector<dpp::snowflake> Users = Record.GetDrivers().m_Entries;

	for (dpp::snowflake Driver : Record.GetDrivers().m_Entries)
	{
		dpp::user User = g_pCluster->user_get_sync(Driver);
		dpp::guild_member Member = g_pCluster->guild_get_member_sync(event.command.guild_id, Driver);

		// Not sure if this can happen but better be safe
		if (Member.user_id == (dpp::snowflake)0 || User.id == (dpp::snowflake)0)
			continue;

		MentionString += User.get_mention() + " ";
		Mentions.push_back(std::make_pair(User, Member));
	}

	// Create Author
	dpp::embed_author Author;
	Author.name = IssuingDriver.format_username();
	Author.icon_url = IssuingDriver.get_avatar_url();
	Author.proxy_icon_url = IssuingDriver.get_avatar_url();
	Author.url = IssuingDriver.get_avatar_url();

	// Create report message
	dpp::message Message = GenerateReportMessage(Author, "Has reported their bus ride:", DestinationIndex, Record);

	// Add & allow mentions
	Message.set_content(MentionString);
	Message.set_allowed_mentions(false, false, false, false, Users, Roles);
	Message.mentions = Mentions;

	// Create server notice
	g_pCluster->message_create(Message);

	// Edit original message to prevent spamming
	event.edit_original_response(
		dpp::message().add_embed(
			dpp::embed().set_description("You successfully reported a raid.").set_color(g_pConfig->m_MessageColor)
		)
	);

	m_Reports.erase(ReportID);
}

bool BusReport::IsOutdatedForm(const uint64_t ReportID, const dpp::button_click_t& event)
{
	if (m_Reports.count(ReportID))
		return false;

	// Edit message if form is outdated
	dpp::embed Embed;
	dpp::message Message;

	Embed.set_description("Form outdated, please create a new report.");
	Embed.set_color(g_pConfig->m_MessageColorWarning);
	Message.add_embed(Embed);

	event.edit_original_response(Message);

	m_Reports.erase(ReportID);

	return true;
}