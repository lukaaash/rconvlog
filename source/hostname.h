#ifndef _HOSTNAME_H_
#define _HOSTNAME_H_

#include "StdAfx.h"

class BT_REC
{
public:
	unsigned long m_nIp;
	char * m_sHostname;
	BT_REC * more;
	BT_REC * less;
public:
	~BT_REC();
};

class CNameResolution
{
private:
	WSADATA m_wsadata;
	hostent * m_pHostEnt;
public:
	BT_REC * m_oCache;
	CNameResolution ();
	~CNameResolution ();
	char * Resolve (const char * ip);
};

#endif
