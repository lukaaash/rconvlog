#ifndef _HOSTNAME_H_
#define _HOSTNAME_H_

#include "StdAfx.h"

class CNameNode
{
public:
	unsigned long m_nIp;
	char * m_sHostname;
	bool m_bCached;
	CNameNode * more;
	CNameNode * less;
public:
	~CNameNode();
	void Print (FILE * fOut);
};

class CNameResolution
{
private:
	WSADATA m_wsadata;
	hostent * m_pHostEnt;
	int m_nHostnamesRead;
	int m_nHostnamesResolved;
	int m_nHostnamesTotal;
	int m_nTimeWaiting;
	char * m_sCacheFile;
public:
	CNameNode * m_oCache;
	CNameResolution (char * sCacheFile);
	~CNameResolution ();
	char * Resolve (const char * ip);
	void Insert (const char * sIp, const char * sHostname);
};

#endif
