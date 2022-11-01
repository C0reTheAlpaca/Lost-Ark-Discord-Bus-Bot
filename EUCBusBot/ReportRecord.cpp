#include "ReportRecord.h"

std::string UserList::GetEmbedList()
{
	std::string EmbedList;

	for (dpp::snowflake User : m_Entries)
	{
		EmbedList += "• <@" + std::to_string(User) + ">\n";
	}

	if (EmbedList.empty())
		EmbedList = "/";

	return EmbedList;
}

std::string UserList::GetStringList()
{
	std::string EmbedList;

	for (auto It = m_Entries.begin(); It != m_Entries.end(); It++)
	{
		int Index = std::distance(m_Entries.begin(), It);
		EmbedList += (Index ? "," : "") + std::to_string(*It);
	}

	return EmbedList;
}

ReportRecord::ReportRecord(dpp::snowflake ServerID, std::vector<dpp::snowflake> Drivers, std::vector<dpp::snowflake> Passengers, std::string Attachment)
{
	m_WasSubmitted = false;
	m_ServerID = ServerID;
	m_Drivers.m_Entries = Drivers;
	m_Passengers.m_Entries = Passengers;
	m_Attachment = Attachment;
	m_DriverCount = Drivers.size();
}

dpp::snowflake ReportRecord::GetIssuingDriver()
{
	return m_Drivers.m_Entries.front();
}

std::string ReportRecord::GetAttachment()
{
	return m_Attachment;
}

int ReportRecord::GetDriverCount()
{
	return m_DriverCount;
}

UserList ReportRecord::GetDrivers()
{
	return m_Drivers;
}

UserList ReportRecord::GetPassengers()
{
	return m_Passengers;
}

dpp::snowflake ReportRecord::GetServerID()
{
	return m_ServerID;
}

void ReportRecord::SetServerID(dpp::snowflake ServerID)
{
	m_ServerID = ServerID;
}

void ReportRecord::SetSubmitted()
{
	m_WasSubmitted = true;
}

bool ReportRecord::WasSubmitted()
{
	return m_WasSubmitted;
}

std::string ReportRecord::GetDestination()
{
	return m_BusDestination;
}

void ReportRecord::SetDestination(std::string Destination)
{
	m_BusDestination = Destination;
}

void ReportRecord::SetDriverCount(int Count)
{
	if (Count < m_Drivers.m_Entries.size())
		return;

	m_DriverCount = Count;
}

dpp::message ReportRecord::GetMessage()
{
	return m_Message;
}

void ReportRecord::SetMessage(dpp::message Message)
{
	m_Message = Message;
}