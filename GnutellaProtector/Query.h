// Query.h
#pragma once

class Query
{
public:
	// Public Member Functions
	Query();
	~Query();
	void Clear();

	int GetBufferLength();
	int WriteToBuffer(char *buf);
	int ReadFromBuffer(char *buf);

	// Public Data Members
	GUID m_guid;
	string m_query;

	unsigned int m_spoofs_sent;
	unsigned int m_poisons_sent;
	unsigned int m_dists_sent;
	unsigned char m_hops;
	unsigned char m_ttl;
	unsigned int m_ip;
	int m_track;
	
	CTime m_timestamp;
	string m_project;
};