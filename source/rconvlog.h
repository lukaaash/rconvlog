#ifndef _RCONVLOG_H_
#define _RCONVLOG_H_

#include "StdAfx.h"

class CError
{
private:	
	char * m_sMessage;
public:
	CError (char * sMessage) {m_sMessage = sMessage;}
	CError () {m_sMessage = NULL;}
	void Display() {if (m_sMessage) printf ("error: %s\n",m_sMessage);}
};

#endif