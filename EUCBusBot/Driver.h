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
	void InfoGeneral(const dpp::snowflake Driver, const dpp::interaction_create_t& event);
	void InfoRaid(const dpp::snowflake Driver, const std::string Raid, const dpp::interaction_create_t& event);
	void VouchDriver(const dpp::interaction_create_t& event);
	void ViewDriverInfo(const dpp::interaction_create_t& event, dpp::snowflake ContextDriver = 0);
	void ModifyRuns(const dpp::interaction_create_t& event, bool ModifyCap, bool Add);
	void WipeRuns(const dpp::interaction_create_t& event);
};