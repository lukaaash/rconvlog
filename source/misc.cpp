#include "misc.h"

time_t GetUnixTime ()
{
	return time(NULL);
}

time_t MakeUnixTime (short y, short m, short d, short h, short n, short s)
{
	struct tm mtime;
	mtime.tm_sec = s;
	mtime.tm_min = n;
	mtime.tm_hour = h;
	mtime.tm_mday = d;
	mtime.tm_mon = m-1;
	mtime.tm_year = y-1900;
	mtime.tm_wday = 0;
	mtime.tm_yday = 0;
	mtime.tm_isdst = 0;
	time_t t = mktime (&mtime);
	return t;
}

time_t StringToUnixTime (char * strTime)
{
	if (strTime==NULL)
		return 0;

	if (strlen(strTime)!=19)
		return 0;
	
	char * t = strdup (strTime);
	if (t==NULL)
		return 0;

	int y,m,d,h,n,s;
	t[4] = 0;
	t[7] = 0;
	t[10] = 0;
	t[13] = 0;
	t[16] = 0;

	y = atoi(t+0);
	m = atoi(t+5);
	d = atoi(t+8);
	h = atoi(t+11);
	n = atoi(t+14);
	s = atoi(t+17);
	free(t);

	if (y<1970)
		return 0;

	return MakeUnixTime (y,m,d,h,n,s);
}
