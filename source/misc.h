#include "stdafx.h"

int FileTimeToUnixTime (FILETIME * ft);
int GetUnixTime ();
int MakeUnixTime (short y, short m, short d, short h, short n, short s);
int StringToUnixTime (char * strTime);

