#pragma once
#include <dpp/dpp.h>

class UserList
{
public:
	std::string GetEmbedList();
	std::string GetStringList();
	std::vector<dpp::snowflake> m_Entries;
};

class ReportRecord
{
public:

	ReportRecord() { };
	ReportRecord(dpp::snowflake ServerID, std::vector<dpp::snowflake> Drivers, std::vector<dpp::snowflake> Passengers, std::string Attachment);
	UserList GetDrivers();
	UserList GetPassengers();
	dpp::snowflake GetServerID();
	std::string GetDestination();
	dpp::message GetMessage();
	dpp::snowflake GetIssuingDriver();
	std::string GetAttachment();
	int GetDriverCount();
	void SetMessage(dpp::message);
	void SetDestination(std::string Destination);
	void SetDriverCount(int Count);
	void SetServerID(dpp::snowflake ServerID);
	void SetSubmitted();
	bool WasSubmitted();

private:
	bool m_WasSubmitted;
	int m_DriverCount;
	std::string m_Attachment;
	dpp::message m_Message;
	dpp::snowflake m_ServerID;
	std::string m_BusDestination;
	UserList m_Drivers;
	UserList m_Passengers;
};