#include "Bot.h"
#include "ConfigManager.h"
#include "BusReport.h"
#include "Driver.h"

dpp::cluster* g_pCluster = nullptr;

Bot::Bot()
{
	// Load config
	g_pConfig = new ConfigManager("config.json");
	std::string Token = g_pConfig->GetToken();

	// Setup command handlers
	m_pDriver = new Driver();
	m_pBusReport = new BusReport();

	// Setup cluster
	g_pCluster = new dpp::cluster(Token);
	g_pCluster->on_log(dpp::utility::cout_logger());

	// Register
	RegisterEvents();

	g_pCluster->on_ready([this](const dpp::ready_t& event)
		{
			if (dpp::run_once<struct register_bot_commands>())
				RegisterCommands();
		}
	);

	// Run
	g_pCluster->start(dpp::st_wait);
}

Bot::~Bot()
{
	delete m_pDriver;
	delete m_pBusReport;
}

void Bot::RegisterCommands()
{
	std::vector<Command> Commands 
	{
		Command({ "bus", "Report a bus ride", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_attachment, "screenshot", "Upload a screenshot", true),
			dpp::command_option(dpp::co_string, "other_drivers", "Enter all drivers with @", true),
			dpp::command_option(dpp::co_string, "passengers", "Enter all passengers with @", true),
		}),

		Command({ "driver", "Gives an overview of the driver", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
			dpp::command_option(dpp::co_string, "raid", "Argos, Valtan, Vykas, Kakul, Brelshaza, Akkan", false),
		}),

		Command({ "vouch", "Vouch for a driver", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
		}),

		Command({ "add_runs", "Adds runs to driver", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
			dpp::command_option(dpp::co_integer, "amount", "Amount of runs to add", true),
			dpp::command_option(dpp::co_integer, "driver_count", "Number of drivers", true),
			dpp::command_option(dpp::co_string, "raid", "Argos, Valtan, Vykas, Kakul, Brelshaza, Akkan", true),
		}),

		Command({ "remove_runs", "Removes runs from driver", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
			dpp::command_option(dpp::co_integer, "amount", "Amount of runs to remove", true),
			dpp::command_option(dpp::co_integer, "driver_count", "Number of drivers", true),
			dpp::command_option(dpp::co_string, "raid", "Argos, Valtan, Vykas, Kakul, Brelshaza, Akkan", true),
		}),

		Command({ "wipe_runs", "Wipes runs of specific raid. Wipes all runs if 'raid' is left empty.", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
			dpp::command_option(dpp::co_string, "raid", "Argos, Valtan Normal, Valtan Hard, ...", false),
		}),

		Command({ "add_cap", "Adds runs to driver", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
			dpp::command_option(dpp::co_integer, "amount", "Amount of weekly cap to add", true),
			dpp::command_option(dpp::co_string, "raid", "Argos, Valtan, Vykas, Kakul, Brelshaza, Akkan", true),
		}),

		Command({ "remove_cap", "Removes runs from driver", g_pCluster->me.id },
		{
			dpp::command_option(dpp::co_user, "driver", "Enter the driver with @", true),
			dpp::command_option(dpp::co_integer, "amount", "Amount of weekly cap to remove", true),
			dpp::command_option(dpp::co_string, "raid", "Argos, Valtan, Vykas, Kakul, Brelshaza, Akkan", true),
		}),
	};

	for (Command& Command : Commands)
	{
		for (const dpp::command_option& Option : Command.m_Options)
		{
			Command.m_Command.add_option(Option);
		}

		g_pCluster->global_command_create(Command.m_Command);
	}
}

void Bot::RegisterEvents()
{
	g_pCluster->on_slashcommand([this](const dpp::slashcommand_t& event)
		{
			if (event.command.get_command_name() == "bus") {
				m_pBusReport->CreateReportForm(event);
			} else if (event.command.get_command_name() == "driver") {
				m_pDriver->ViewDriverInfo(event);
			} else if (event.command.get_command_name() == "vouch") {
				m_pDriver->VouchDriver(event);
			} else if (event.command.get_command_name() == "add_runs" || event.command.get_command_name() == "remove_runs") {
				m_pDriver->ModifyRuns(event, false, event.command.get_command_name() == "add_runs" ? true : false);
			} else if (event.command.get_command_name() == "wipe_runs") {
				m_pDriver->WipeRuns(event);
			} else if (event.command.get_command_name() == "add_cap" || event.command.get_command_name() == "remove_cap") {
				m_pDriver->ModifyRuns(event, true, event.command.get_command_name() == "add_cap" ? true : false);
			}
		}
	);

	g_pCluster->on_button_click([this](const dpp::button_click_t& event)
		{
			if (event.custom_id == "createbus") {
				m_pBusReport->PreviewReport(event);
			} else if (event.custom_id == "crashbus") {
				m_pBusReport->CancelReport(event);
			} else if (event.custom_id == "confirmbus") {
				m_pBusReport->ConfirmReport(event);
			} else if (event.custom_id == "editbus") {
				m_pBusReport->EditReport(event);
			}
		}
	);

	g_pCluster->on_select_click([this](const dpp::select_click_t& event)
		{
			if (event.custom_id == "busdestination") {
				m_pBusReport->SetBusDestination(event);
			} else if (event.custom_id == "drivercount") {
				m_pBusReport->SetBusDriverCount(event);
			}
		}
	);
}
