#pragma once
#include <dpp/dpp.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

class Driver
{
public:
	void InfoGeneral(const dpp::snowflake Driver, const dpp::slashcommand_t& event);
	void InfoRaid(const dpp::snowflake Driver, const std::string Raid, const dpp::slashcommand_t& event);
	void VouchDriver(const dpp::slashcommand_t& event);
	void ViewDriverInfo(const dpp::slashcommand_t& event);
	void ModifyRuns(const dpp::slashcommand_t& event, bool ModifyCap, bool Add);
	void WipeRuns(const dpp::slashcommand_t& event);
};