#include "StdAfx.h"
#include "hostname.h"
#include "rconvlog.h"


void BT_REC::~BT_REC()
{
	if (more) delete more;
	if (less) delete less;
}


char * CNameResolution::Resolve (const char * sIp)
{
	unsigned long nIp;
	nIp = inet_addr(sIp);

	BT_REC * bt_ins;
	BT_REC ** bt_next;
	bt_next = &m_oCache;
	do {
		bt_ins = *bt_next;
		if (bt_ins==NULL) break;
	
		if (bt_ins->m_nIp==nIp) return bt_ins->m_sHostname;
		else if (bt_ins->m_nIp < nIp) bt_next = &bt_ins->more;
		else bt_next = &bt_ins->less;
	} while (1);

	*bt_next = new BT_REC;
	(*bt_next)->m_nIp = nIp;
	(*bt_next)->more = NULL;
	(*bt_next)->less = NULL;

//	printf ("Resolving %s\n",sIp);
	if (!(m_pHostEnt = gethostbyaddr ((const char *)&nIp,4,AF_INET))) {
		(*bt_next)->m_sHostname = strdup(sIp);
	} else (*bt_next)->m_sHostname = strdup(m_pHostEnt->h_name);

	return (*bt_next)->m_sHostname;
}


CNameResolution::CNameResolution()
{
	if (WSAStartup (MAKEWORD(2,0),&m_wsadata)) {
		throw CError ("unable to init winsock");
	    return;
	}
	m_oCache = NULL;
}
 

CNameResolution::~CNameResolution()
{
    WSACleanup ();
}


