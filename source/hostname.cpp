#include "hostname.h"
#include "rconvlog.h"
#include "misc.h"

CNameNode::~CNameNode()
{
	delete m_sHostname;
	if (more) delete more;
	if (less) delete less;
}

void CNameNode::Print (FILE * fOut)
{
	if (!m_bCached)
		fprintf (fOut,"%d.%d.%d.%d\t%s\n",m_nIp&255,(m_nIp>>8)&255,(m_nIp>>16)&255,m_nIp>>24,m_sHostname);
	if (more)
		more->Print(fOut);
	if (less)
		less->Print(fOut);
}

char * CNameResolution::Resolve (const char * sIp)
{
	unsigned long nIp;
	nIp = inet_addr(sIp);

	m_nHostnamesTotal++;

	CNameNode * bt_ins;
	CNameNode ** bt_next;
	bt_next = &m_oCache;
	while ((bt_ins=*bt_next)!=NULL)
	{
		if (bt_ins->m_nIp==nIp)
			return bt_ins->m_sHostname;
		else
			if (bt_ins->m_nIp < nIp)
				bt_next = &bt_ins->more;
			else
				bt_next = &bt_ins->less;
	}

	*bt_next = new CNameNode;
	if ((*bt_next)==NULL)
		throw CError ("not enough memory");

	(*bt_next)->m_nIp = nIp;
	(*bt_next)->m_bCached = false;
	(*bt_next)->more = NULL;
	(*bt_next)->less = NULL;

//	printf ("Resolving %s\n",sIp);
	m_ctTimeWaiting -= clock();

	if (!(m_pHostEnt = gethostbyaddr ((const char *)&nIp,4,AF_INET)))
		(*bt_next)->m_sHostname = strdup(sIp);
	else
		(*bt_next)->m_sHostname = strdup(m_pHostEnt->h_name);

	m_ctTimeWaiting += clock();
	m_nHostnamesResolved++;

	return (*bt_next)->m_sHostname;
}

void CNameResolution::Insert (const char * sIp, const char * sHostname)
{
	unsigned long nIp;
	nIp = inet_addr(sIp);

	CNameNode * bt_ins;
	CNameNode ** bt_next;
	bt_next = &m_oCache;
	while ((bt_ins=*bt_next)!=NULL)
	{
		if (bt_ins->m_nIp==nIp)
			return;
		else
			if (bt_ins->m_nIp < nIp)
				bt_next = &bt_ins->more;
			else
				bt_next = &bt_ins->less;
	}

	*bt_next = new CNameNode;
	if ((*bt_next)==NULL)
		throw CError ("not enough memory");

	(*bt_next)->m_nIp = nIp;
	(*bt_next)->m_sHostname = strdup(sHostname);
	(*bt_next)->m_bCached = true;
	(*bt_next)->more = NULL;
	(*bt_next)->less = NULL;

	m_nHostnamesRead++;
}


CNameResolution::CNameResolution (char * sCacheFile)
{
#ifdef _WIN32
	if (WSAStartup (MAKEWORD(2,0),&m_wsadata))
		throw CError ("unable to init winsock");
#endif
	m_oCache = NULL;

	m_nHostnamesRead = 0;
	m_nHostnamesResolved = 0;
	m_ctTimeWaiting = 0;
	m_nHostnamesTotal = 0;

	if (!sCacheFile)
	{
		m_sCacheFile = NULL;
		return;
	}
	m_sCacheFile = strdup (sCacheFile);

	FILE * fCache = fopen (sCacheFile,"rt");
	if (fCache)
	{
		char sLine[MAX_FILENAME_LENGTH];
		char * sHostname;

		while (!feof(fCache))
		{
			if (fgets(sLine,MAX_FILENAME_LENGTH,fCache)==NULL)
			{
				if (ferror(fCache))
					throw CError ("error reading cache file");
			}
			else
			{
				sHostname = NULL;
				for (int i=0; sLine[i]; i++)
				{
					if (sLine[i]=='\t')
					{
						sLine[i]=0;
						sHostname = sLine+i+1;
					}
					else if (sLine[i]<32)
					{
						sLine[i]=0;
						break;
					}
				}
				if (sHostname) Insert (sLine,sHostname);
			}
		}
		fclose (fCache);
	}
}


CNameResolution::~CNameResolution()
{
#ifdef _WIN32
	WSACleanup ();
#endif

	if (m_sCacheFile)
	{
		FILE * fCache = fopen (m_sCacheFile,"at");
		if (!fCache)
			throw CError ("unable to open cache file for appending");

		if (m_oCache)
			m_oCache->Print (fCache);

		fclose (fCache);
		printf ("Hostnames read from cache: %d\n",m_nHostnamesRead);
	}

	if (m_nHostnamesTotal>0)
	{
		printf ("Hostnames resolved: %d\n",m_nHostnamesResolved);
		printf ("Resolving time: %6.2fs\n",float(m_ctTimeWaiting)/float(CLOCKS_PER_SEC));
	}

	if (m_sCacheFile)
		delete m_sCacheFile;
	
	if (m_oCache)
		delete m_oCache;
}
