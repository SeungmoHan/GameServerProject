#include <cstring>
#include <iostream>
#include "DBConnector.h"

		
#define QUERY_COPY(queryString, query ,... ){sprintf(queryString, query, ##__VA_ARGS__);}

namespace univ_dev
{
	DBConnector::DBConnector(const char* ip, const char* rootName, const char* rootPassword, const char* initSchema,unsigned int slowQuery, unsigned short port) : _DataBasePort(port) , _SlowQuery(slowQuery)
	{
		this->SetDBLogDirectory(L"ServerLog\\DBLog");
		this->_DBLog.LOG_SET_LEVEL(LogClass::LogLevel::LOG_LEVEL_SYSTEM);
		mysql_init(&this->_Conn);

		this->_DataBaseIP = inet_addr(ip);
		strcpy_s(this->_DataBaseStringIP, ip);
		strcpy_s(this->_RootName, rootName);
		strcpy_s(this->_RootPassword, rootPassword);
		strcpy_s(this->_SchemaName, initSchema);
	}

	DBConnector::~DBConnector()
	{
		this->DBClose();
	}

	bool DBConnector::DBConnect()
	{
		// DB ����
		int sslMode = SSL_MODE_DISABLED;
		mysql_options(&this->_Conn, MYSQL_OPT_SSL_MODE, &sslMode);
		this->_Connection = mysql_real_connect(&_Conn, this->_DataBaseStringIP, this->_RootName, this->_RootPassword, this->_SchemaName, this->_DataBasePort, nullptr, 0);
		if (this->_Connection == nullptr)
		{
			this->DBLogError(L"Connect Error DB Quit");
			return false;
		}

		//�ѱۻ���������߰�.
		mysql_set_character_set(this->_Connection, "utf8");
		return true;
	}

	bool DBConnector::Query(const char* query)
	{
		if (!this->QuerySave(query))
			return false;
		this->FreeResult(this->GetQueryResult());
		return true;
	}

	bool DBConnector::QuerySave(const char* query)
	{
		// From ���� DB�� �����ϴ� ���̺� ������ �����ϼ���
		WCHAR wQuery[this->QUERY_STRING_MAX_LEN];

		size_t querySize = strlen(query);
		mbstowcs_s(&querySize,wQuery, query, this->QUERY_STRING_MAX_LEN);
		DWORD queryBeginTime = timeGetTime();
		int query_stat = mysql_query(this->_Connection, query);
		DWORD queryEndTime = timeGetTime();
		WCHAR errStr[512];
		if (queryEndTime - queryBeginTime > this->_SlowQuery)
		{
			wsprintf(errStr, L"Query is too slow `%d` : %s", queryEndTime - queryBeginTime, wQuery);
			this->DBLogSystem(errStr);
		}
		if (query_stat != 0)
		{
			wsprintf(errStr, L"Query Failed : %s", wQuery);
			this->DBLogError(errStr);
			DBConnect();
			return false;
		}
		return true;
	}
	void DBConnector::SetDBLogDirectory(const WCHAR* directory)
	{
		this->_DBLog.LOG_SET_DIRECTORY(directory);
	}
	void DBConnector::DBLogError(const WCHAR* errorLog)
	{
		this->_DBLog.LOG(errorLog, LogClass::LogLevel::LOG_LEVEL_ERROR);
	}

	void DBConnector::DBLogSystem(const WCHAR* systemLog)
	{
		this->_DBLog.LOG(systemLog, LogClass::LogLevel::LOG_LEVEL_SYSTEM);
	}

	void DBConnector::DBLogWarning(const WCHAR* warningLog)
	{
		this->_DBLog.LOG(warningLog, LogClass::LogLevel::LOG_LEVEL_WARNING);
	}

	void DBConnector::DBLogDebug(const WCHAR* debugLog)
	{
		this->_DBLog.LOG(debugLog, LogClass::LogLevel::LOG_LEVEL_DEBUG);
	}

	MYSQL_RES* DBConnector::GetQueryResult()
	{
		MYSQL_RES* sqlResult;
		sqlResult = mysql_store_result(this->_Connection);		// ��� ��ü�� �̸� ������
		//	sql_result=mysql_use_result(connection);		// fetch_row ȣ��� 1���� ������
		return sqlResult;
	}

	MYSQL_ROW DBConnector::FetchRow(MYSQL_RES* sqlResult)
	{
		return mysql_fetch_row(sqlResult);
	}

	void DBConnector::FreeResult(MYSQL_RES* result)
	{
		mysql_free_result(result);
	}


	unsigned long DBConnector::GetMySqlErrno() const
	{
		return mysql_errno(this->_Connection);
	}

	//buffer�� �˳��� �غ��Ͻÿ�
	const char* DBConnector::GetMySqlError()
	{
		return mysql_error(this->_Connection);
	}


	void DBConnector::DBClose()
	{
		mysql_close(this->_Connection);
	}

}
