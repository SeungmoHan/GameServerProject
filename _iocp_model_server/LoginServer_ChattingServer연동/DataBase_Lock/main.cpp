#include "LoginServer.h"

#define LOGIN_SERVER_PORT 10435
#define MAX_SESSION_COUNT 20000
#define TIME_OUT_CLOCK_TIME 3000

int main()
{
	univ_dev::LoginServer server(10435, SOMAXCONN, 10, 10, false, MAX_SESSION_COUNT, TIME_OUT_CLOCK_TIME);
}