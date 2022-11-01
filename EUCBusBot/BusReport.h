#pragma once
#include <dpp/dpp.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include "ReportRecord.h"

class BusReport
{
public:
	dpp::message GenerateReportForm(ReportRecord Record);
	dpp::message GenerateReportMessage(const dpp::embed_author Author, const std::string Description, const int DestinationIndex, ReportRecord Record);
	std::unique_ptr<sql::ResultSet> GetDriver(dpp::snowflake Driver);

	void CreateReportForm(const dpp::slashcommand_t& event);
	void SetBusDestination(const dpp::select_click_t& event);
	void SetBusDriverCount(const dpp::select_click_t& event);
	void CancelReport(const dpp::button_click_t& event);
	void PreviewReport(const dpp::button_click_t& event);
	void EditReport(const dpp::button_click_t& event);
	void ConfirmReport(const dpp::button_click_t& event);
	bool IsOutdatedForm(const uint64_t ReportID, const dpp::button_click_t& event);

private:
	std::map<uint64_t, ReportRecord> m_Reports;
};