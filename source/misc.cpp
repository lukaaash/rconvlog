#include "stdafx.h"


int FileTimeToUnixTime (FILETIME * ft)
{
	// bacha, je tady chyba roku 2038 ;-)
	SYSTEMTIME t;
	FILETIME f;
	FileTimeToSystemTime (ft,&t);
	t.wYear = t.wYear-369;
	SystemTimeToFileTime (&t,&f);
	ULONGLONG lt;
	memcpy (&lt, &f, sizeof(lt));
	return (int)(lt/10000000);
}


int GetUnixTime ()
{
	SYSTEMTIME t;
	FILETIME ft;
	GetLocalTime (&t);
	SystemTimeToFileTime (&t,&ft);
	return FileTimeToUnixTime (&ft);
}


int MakeUnixTime (short y, short m, short d, short h, short n, short s)
{
	SYSTEMTIME t;
	FILETIME ft;
	t.wYear = y;
	t.wMonth = m;
	t.wDayOfWeek = 0;
	t.wDay = d;
	t.wHour = h;
	t.wMinute = n;
	t.wSecond = s;
	t.wMilliseconds = 0;
	SystemTimeToFileTime (&t,&ft);
	return FileTimeToUnixTime (&ft);
}


int StringToUnixTime (char * strTime)
{
	if (strTime==NULL) return 0;
	if (strlen(strTime)!=19) return 0;
	char * t = strdup (strTime);
	if (t==NULL) return 0;

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

	if (y<1970) return 0;

	return MakeUnixTime (y,m,d,h,n,s);
}

