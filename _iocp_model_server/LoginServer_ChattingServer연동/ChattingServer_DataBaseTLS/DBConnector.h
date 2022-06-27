#pragma once
#ifndef __DATA_BASE_CONNECTOR__
#define __DATA_BASE_CONNECTOR__
#define __UNIV_DEVELOPER_
#pragma comment(lib,"libmysql.lib")
#pragma comment(lib,"mysqlclient.lib")
#pragma comment(lib,"ws2_32.lib")
#include "c_con_include/mysql.h"
#include "c_con_include/errmsg.h"
#include "LogClass.h"

namespace univ_dev
{
	class DBConnector
	{
	private:
		constexpr static int QUERY_STRING_MAX_LEN = 2048;
	public:
		DBConnector(const char* ip, const char* rootName, const char* rootPassword, const char* initSchema,unsigned int slowQuery, unsigned short port = 3306);
		~DBConnector();

		bool DBConnect();

		//가변 파라미터는 무조건 문자열이어야됨
		bool Query(const char* query);
		//가변 파라미터는 무조건 문자열이여야됨
		bool QuerySave(const char* query);
		MYSQL_RES* GetQueryResult();
		MYSQL_ROW FetchRow(MYSQL_RES* result);
		void FreeResult(MYSQL_RES* result);

		unsigned long GetMySqlErrno() const;
		const char* GetMySqlError();

		void SetDBLogDirectory(const WCHAR* directory);
		void DBLogError(const WCHAR* errorLog);
		void DBLogSystem(const WCHAR* systemLog);
		void DBLogWarning(const WCHAR* warningLog);
		void DBLogDebug(const WCHAR* debugLog);

		void DBClose();
	private:

		char _DataBaseStringIP[30];
		USHORT _DataBasePort;
		ULONG _DataBaseIP;
		char _RootName[30];
		char _RootPassword[30];

		char _SchemaName[30];

		unsigned int _SlowQuery;

		MYSQL _Conn;
		MYSQL* _Connection = nullptr;
		LogClass _DBLog;

	};
}


#endif // !__MY_SQL_CONNECTOR__
