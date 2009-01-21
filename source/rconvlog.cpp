#include "hostname.h"
#include "rconvlog.h"
#include "misc.h"

struct FIELDNAMES_W3C
{
	FIELDS type;
	char * str;
};

static FIELDNAMES_W3C fieldnames_w3c[] =
{
	{F_DATE,"date"},
	{F_TIME,"time"},
	{F_C_IP,"c-ip"},
	{F_CS_USERNAME,"cs-username"},
	{F_S_IP,"s-ip"},
	{F_S_PORT,"s-port"},
	{F_CS_METHOD,"cs-method"},
	{F_CS_URI_STEM,"cs-uri-stem"},
	{F_CS_URI_QUERY,"cs-uri-query"},
	{F_SC_STATUS,"sc-status"},
	{F_SC_BYTES,"sc-bytes"},
	{F_CS_BYTES,"cs-bytes"},
	{F_CS_HOST,"cs-host"},
	{F_CS_USER_AGENT,"cs(User-Agent)"},
	{F_CS_REFERER,"cs(Referer)"},
	{F_CS_VERSION,"cs-version"},
    {F_CS_COOKIE,"cs(Cookie)"},
};

#define FIELDNAMES_W3C_NUM (sizeof(fieldnames_w3c)/sizeof(FIELDNAMES_W3C))

/*
 * CLogReader::CLogReader - initializes CLogReader object and parses the command line
 *        argc,argv        = command line arguments
 *        nMaxLineLength    = length of m_sLine string
 *        nMaxFieldCount    = length of p_msFields and m_iFields
 */
CLogReader::CLogReader (int argc, char * argv[], int nMaxLineLength, int nMaxFieldCount, int nReportInterval)
{
	char * sOutputDir = NULL;
	bool bConvertIp = false;
	char * sCacheFile = NULL;
	bool bDisplayHelp = false;
	char * sGmtOffset = NULL;

	// do some initialization
	m_nReportInterval = nReportInterval;
	m_nMaxLineLength = nMaxLineLength;
	m_sLine = new char[nMaxLineLength+1];
	m_nMaxFieldCount = nMaxFieldCount;
	m_psFields = new pchar[nMaxFieldCount+1];
	m_sEmpty = strdup ("-");
	m_nFieldCount = 0;
	memset(m_iFields,-1,sizeof(m_iFields));

	m_ctStartTime = clock ();

	m_ttIgnoreTime = 0;

	m_cInputType = 0;
	m_sGmtOffset = NULL;
	m_bSaveNonWwwEntries = false;
	m_bCookies = false;
	m_bOverwrite = false;
	m_cLogType = 0;
	m_cDateFormat = 0;
	m_oNameRes = NULL;

	m_nFilesCount = 0;
	m_psFiles = new pchar[argc];
	memset (m_psFiles, 0, argc * sizeof(pchar));

	// command line parsing
	if (argc==1)
		bDisplayHelp = true;
	else
	{
		int i;
		for (i=1; i<argc; i++)
		{
			if (strlen(argv[i])==0) continue;

			if (argv[i][0]=='-')
			{
				switch (argv[i][1])
				{
					case 'i': m_cInputType = argv[i][2]; break;
					case 't': sGmtOffset = argv[i+1]; i++; break;
					case 'o': sOutputDir = argv[i+1]; i++; break;
					case 'c': sCacheFile = argv[i+1]; i++; break;
					case 'x': m_bSaveNonWwwEntries = true; break;
					case 'd': bConvertIp = true; break;
					case 'w': m_bOverwrite = true; break;
					case 'l': m_cDateFormat = argv[i][2]-'0'; break;
					case 'b': m_cLogType = argv[i][2]-'0'; break;
					case 'u': m_bCookies = true; break;
					case 'h': m_ttIgnoreTime = GetUnixTime()-3600*atoi(argv[i+1]); i++; break;
					case 'n': m_ttIgnoreTime = StringToUnixTime(argv[i+1]); i++; break;
					default: bDisplayHelp = true; break;
				}
			}
			else
			{
				m_psFiles[m_nFilesCount] = argv[i];
				m_nFilesCount++;
			}
		}
	}


	if (m_cLogType<0 || m_cLogType>3)
		bDisplayHelp = true;

	// display help if command line contains unknown option or filename is missing
	if (m_nFilesCount==0 || bDisplayHelp)
	{
		DisplayHelp();
		throw CError();
	}

	// get timezone string
	if (sGmtOffset)
	{
		if (strncmp(sGmtOffset,"ncsa:+",6)!=0 && strncmp(sGmtOffset,"ncsa:-",6)!=0)
			throw CError ("unsupported gmt offset format");
		if (strlen(sGmtOffset)!=10)
			throw CError ("unsupported timezone format");
		m_sGmtOffset=strdup(sGmtOffset+5);
	}
	else
		m_sGmtOffset = strdup ("+0000");

	// make sure that the last char of output directory is backslash and the length is not too long
	m_nOutputDirLength=0;
	if (sOutputDir)
	{
		int nLen = strlen (sOutputDir);
		if (nLen>=MAX_FILENAME_LENGTH) throw CError ("output path is too long");
		if (nLen>0)
		{
			memcpy (m_sOutputDir,sOutputDir,nLen+1);
			m_nOutputDirLength = nLen;
		}
		if (m_sOutputDir[nLen-1]!=SLASH)
		{
			m_sOutputDir[nLen]=SLASH;
			m_nOutputDirLength++;
		}
	}
	m_sOutputDir[m_nOutputDirLength]=0;

	// if -d option is present, create name resolution object
	if (bConvertIp)
	{
		// use hostname cachefile if given
		// TODO: print the whole path
		if (sCacheFile)
			printf ("\nHostname cache file: %s\n",sCacheFile);
		m_oNameRes = new CNameResolution (sCacheFile);
	}

	m_nLinesTotal = 0;
	m_nLinesWrittenTotal = 0;
}


CLogReader::~CLogReader ()
{
	// delete arrays and strings
	delete m_sLine;
	delete m_psFields;
	delete m_sEmpty;
	delete m_sGmtOffset;
	delete m_psFiles;

	// display some useful statistics
	if (m_nLinesTotal>0)
	{
		printf ("\nTotals:\n=======\nTotal Lines Processed: %11d\n",m_nLinesTotal);
		printf ("Total Web Lines Written: %9d\n",m_nLinesWrittenTotal);

		clock_t ctTotalTime = clock() - m_ctStartTime;
		printf ("Total time: %.2f\n",float(ctTotalTime)/float(CLOCKS_PER_SEC));
	}

	// delete name resolution object if present
	if (m_oNameRes) delete m_oNameRes;
}

void CLogReader::ProgressReport ()
{
	printf ("  %d lines processed\n",m_nLines);
}

/*
 * CLogReader::ReadLine - read one log line from input file
 *        (while the line begins with "#", process it and read other line)
 */
int CLogReader::ReadLine ()
{
	int nChar;
	int nLastChar;
	int nCharPos;
	int nFieldPos;
	bool bEol;
	bool bEor;
	bool bFields;
	bool bSkip;
	bool bDate;

	if (feof(m_fInput)) return -1;

	bEor = false;

	do
	{
		nCharPos = 0;
		nFieldPos = 1;
		bEol = false;
		m_psFields[0] = m_sLine;
		nLastChar = 32;

		if (fgets(m_sLine,m_nMaxLineLength,m_fInput)==NULL)
		{
			if (ferror(m_fInput)) throw CError ("read error");
			if (feof(m_fInput)) return 0;
		}
		else
		{
			if (m_sLine[0])
				m_nLines++;

			bEor = true;
			bFields = false;
			bDate = false;
			bSkip = false;
			if (m_sLine[0]=='#')
			{
				bEor = false;
				if (strncmp(m_sLine,"#Fields:",8)==0) bFields = true;
				else if (strncmp(m_sLine,"#Date:",6)==0) bDate = true;
				else bSkip = true;
			}

			if (!bSkip)
			{
				while ((nChar=m_sLine[nCharPos])!=0)
				{
					if (nChar==0xD || nChar==0xA)
					{
						m_sLine[nCharPos]=0;
						break;
					}
					if (nChar==32)
					{
						m_sLine[nCharPos] = 0;
						if (nLastChar!=nChar) if (nFieldPos<m_nMaxFieldCount)
						{
							m_psFields[nFieldPos++] = m_sLine+nCharPos+1;
						}
					}
					nCharPos++;
					nLastChar = nChar;
				}
				m_nFieldCount = nFieldPos;

				if (bFields)
				{
					memset(m_iFields,-1,sizeof(m_iFields));
					for (int i=0; i<FIELDNAMES_W3C_NUM; i++)
					{
						for (int j=1; j<m_nFieldCount; j++)
						{
							if (strcmp(m_psFields[j],fieldnames_w3c[i].str)==0)
							{
								m_iFields[fieldnames_w3c[i].type]=j-1;
								break;
							}
						}
					}
				}
				if (bDate)
				{
					strncpy(m_sLastDate,m_psFields[1],16);
				}
			}
		}
	} while (!bEor);

	return nCharPos;
}

char * CLogReader::Field (FIELDS field)
{
	int nField = m_iFields[field];
	if (nField>=0 && nField<m_nFieldCount) return m_psFields[nField];

	if (nField<0 && field==F_DATE) return m_sLastDate;

	return m_sEmpty;
}

void CLogReader::DisplayHelp()
{
	puts ("Rebex Internet Log Converter v1.6");
	puts ("Converts W3C log files to the NCSA Combined LogFile format");
	puts ("Copyright (C) 2001-2008 Rebex CR s.r.o. (http://www.rebex.net)");
	puts ("Written by Lukas Pokorny (lukas.pokorny@rebex.cz)");
	puts ("Initial Linux port by Christophe Paquin (cpaquin@cwd.fr)");
	puts ("Initial cookies support by interactivate.com");
	puts ("");

	puts ("Usage: rconvlog [options] LogFile1 [LogFile2] [LogFile3] ...");
	puts ("Options:");
/*	puts ("-i<i|n|e> = input logfile type");
	puts ("    i - MS Internet Standard Log File Format");
	puts ("    n - NCSA Common Log File format");
	puts ("    e - W3C Extended Log File Format");*/
	puts ("-o <output directory> default = current directory");
	puts ("-c <hostname cache file>");
	puts ("-t <ncsa[:GMTOffset]>");
//	puts ("-x save non-www entries to a .dmp logfile");
	puts ("-d = convert IP addresses to domain names");
	puts ("-w = overwrite existing files (default is append)");
/*	puts ("-l<0|1|2> = Date locale format for MS Internet Standard");
	puts ("    0 - MM/DD/YY (default e.g. US)");
	puts ("    1 - YY/MM/DD (e.g. Japan)");
	puts ("    2 - DD.MM.YY (e.g. Germany)");*/
	puts ("-b<0|1|2|3> = how to log bytes sent");
	puts ("    0 - compatible with convlog (default)");
	puts ("    1 - bytes sent server-client only (download)");
	puts ("    2 - bytes sent client-server only (upload)");
	puts ("    3 - bytes sent both upload and download");
	puts ("-n YYYY-MM-DDTHH:NN:SS = ignore records older than specified datetime");
	puts ("-u = turn on cookie support");
	puts ("-h H = ignore records older than H hours");
	puts ("");
	puts ("Examples:");
	puts ("rconvlog file.log -w");
	puts ("rconvlog *.log -w -d -t ncsa:+0800");
#ifdef _WIN32
	puts ("rconvlog w3c*.log -w -d -c c:\\temp\\cache.txt");
#else
	puts ("rconvlog w3c*.log -w -d -c /tmp/cache.txt");
#endif
	puts ("rconvlog abcd*.log efgh.log -n 2002-01-01T00:00:00");

}



static char * g_sMonths[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static int ConvertDate (char * sDateIn, char * sDateOut)
{
	if (strlen(sDateIn)!=10) return 0;
	int nMonth = (sDateIn[5]-'0')*10 + (sDateIn[6]-'0') - 1;
	if (nMonth>11 || nMonth<0) return 0;
	sDateOut[0] = sDateIn[8];
	sDateOut[1] = sDateIn[9];
	sDateOut[2] = '/';
	sDateOut[3] = g_sMonths[nMonth][0];
	sDateOut[4] = g_sMonths[nMonth][1];
	sDateOut[5] = g_sMonths[nMonth][2];
	sDateOut[6] = '/';
	sDateOut[7] = sDateIn[0];
	sDateOut[8] = sDateIn[1];
	sDateOut[9] = sDateIn[2];
	sDateOut[10] = sDateIn[3];
	sDateOut[11] = 0;
	return 1;
}



void CLogReader::Convert(char * sFilename)
{
	struct stat sp;
	stat (sFilename, &sp);
//	printf ("%d >= %d\n",sp.st_mtime,m_nIgnoreTime);
	if (sp.st_mtime<m_ttIgnoreTime)
	{
		printf ("Ignoring %s\n", sFilename);
		return;
	}

	m_fInput = fopen (sFilename,"rb");
	if (!m_fInput) throw CError ("unable to open input file %s", sFilename);

	printf ("\nOpening file %s for processing\n", sFilename);

	if (m_nOutputDirLength+strlen(sFilename)+9>=MAX_FILENAME_LENGTH) throw CError ("logfile path is too long");

	char * sFilenamePart = strrchr (sFilename, SLASH);
	if (sFilenamePart != NULL)
		sFilenamePart++;
	else
		sFilenamePart = sFilename;

	strcpy (m_sOutputDir+m_nOutputDirLength,sFilenamePart);
	if (m_oNameRes)
		strncat (m_sOutputDir,".ncsa.dns",9);
	else
		strncat (m_sOutputDir,".ncsa",5);

	FILE * fOutput;
	if (m_bOverwrite)
	{
		fOutput = fopen (m_sOutputDir,"wt");
		printf ("Writing");
	}
	else
	{
		fOutput = fopen (m_sOutputDir,"at");
		printf ("Appending");
	}
	printf (" file %s\n",m_sOutputDir);
	if (!fOutput)
		throw CError ("unable to open output file");

	char sDateNcsa[12];
	int nLineLength;
	char * sIp;
	char * s;
	char * sMethod;
	int nCsBytes;
	int nBytes;
	char * sCsBytes;
	char * sBytes;
	char * sCsVersion;
	char * sQuery;
	char * sDate;
	char * sTime;
	char strTime[20];
	int nLinesWritten = 0;
	int nLinesIgnored = 0;
	time_t t = 0;

	m_nLines = 0;
	time_t ttLastReport = GetUnixTime();

	while ((nLineLength=ReadLine())>=0) if (nLineLength>=m_nFieldCount*2)
	{
		if ((m_nLines%100)==0)
		{
			t = GetUnixTime();
//			printf ("%d - %d - %d.\n",m_nLines,t,ttLastReport);
			if ((t-ttLastReport)>m_nReportInterval)
			{
				ttLastReport = t;
				ProgressReport ();
			}
		}

		sDate = Field(F_DATE);
		sTime = Field(F_TIME);

		if (strlen(sDate)!=10 || strlen(sTime)!=8)
			continue;

		memcpy (strTime,sDate,10);
		strTime[10] = 32;
		memcpy (strTime+11,sTime,8);
		strTime[19] = 0;

		if (!ConvertDate (Field(F_DATE),sDateNcsa))
			continue;

//		printf ("%d < %d\n",StringToUnixTime(strTime),m_nIgnoreTime);
		if (StringToUnixTime(strTime)<m_ttIgnoreTime)
		{
			nLinesIgnored++;
			continue;
		}

		sMethod = Field(F_CS_METHOD);

		sBytes = Field(F_SC_BYTES);
		sCsBytes = Field(F_CS_BYTES);
		if (sBytes[0]=='-') nBytes = -1; else nBytes = atoi(sBytes);
		if (sCsBytes[0]=='-') nCsBytes = -1; else nCsBytes = atoi(sCsBytes);

		sCsVersion = Field(F_CS_VERSION);
		sQuery = Field(F_CS_URI_QUERY);

		sIp = Field(F_C_IP);
		if (m_oNameRes)
		{
			s = m_oNameRes->Resolve (sIp);
			if (s) sIp = s;
		}
		fprintf (fOutput,"%s ",sIp);
		fprintf (fOutput,"- %s ", Field(F_CS_USERNAME));
		fprintf (fOutput,"[%s:%s %s] ",sDateNcsa, sTime, m_sGmtOffset);
		fprintf (fOutput,"\"");
		fprintf (fOutput,"%s ", sMethod);
		if (sQuery[0]=='-')
			fprintf (fOutput,"%s", Field(F_CS_URI_STEM));
		else
			fprintf (fOutput,"%s?%s", Field(F_CS_URI_STEM), sQuery);

		if (sCsVersion[0]!='-')
			fprintf (fOutput," %s", sCsVersion);
		else
			fprintf (fOutput," HTTP/1.0");
		fprintf (fOutput,"\" ");
		fprintf (fOutput,"%s ", Field(F_SC_STATUS));

		switch (m_cLogType)
		{
			case 1:
				break;
			case 2:
				nBytes = nCsBytes;
				break;
			case 3:
				if (nCsBytes>=0)
				{
					if (nBytes<0)
						nBytes = nCsBytes;
					else
						nBytes+=nCsBytes;
				}
				break;
			default:
				if (strcmp(sMethod,"POST")==0)
					nBytes = nCsBytes;
				break;
		}

		if (nBytes>=0)
			fprintf (fOutput,"%d", nBytes);
		else
			fprintf (fOutput,"-");

		fprintf (fOutput," \"%s\"", Field(F_CS_REFERER));
		fprintf (fOutput," \"%s\"", Field(F_CS_USER_AGENT));
		if (m_bCookies)
			fprintf (fOutput," \"%s\"", Field(F_CS_COOKIE)); //xxx
		fprintf (fOutput,"\n");
		nLinesWritten++;
	}

	fclose (fOutput);
	fclose (m_fInput);

	ProgressReport();

	printf ("%s completed, %d lines processed.\n",sFilenamePart,m_nLines);
	printf ("%d web lines written\n",nLinesWritten);
	printf ("%d web lines ignored\n",nLinesIgnored);
	printf ("%d non-www lines discarded\n",m_nLines-nLinesWritten-nLinesIgnored);

	m_nLinesWrittenTotal += nLinesWritten;
	m_nLinesTotal += m_nLines;
}


void CLogReader::Run ()
{
#ifdef _WIN32
	WIN32_FIND_DATA wfData;
	HANDLE hFind;
	char * sFilename;
	char sPath[MAX_FILENAME_LENGTH];
	int nPathLen;

	for (int i=0; i<m_nFilesCount; i++)
	{
		sFilename = m_psFiles[i];

		if (strchr(sFilename,'?')==NULL && strchr(sFilename,'*')==NULL)
		{
			Convert (sFilename);
		}
		else
		{
			char * p = strrchr(sFilename,SLASH);
			if (p==NULL)
				nPathLen = 0;
			else
				nPathLen = p-sFilename+1;

			if (nPathLen>=MAX_FILENAME_LENGTH)
				throw CError ("filename %s is too long", sFilename);

			memcpy (sPath, sFilename, nPathLen);

			hFind = FindFirstFile (sFilename, &wfData);
			if (hFind==INVALID_HANDLE_VALUE)
				throw CError ("no files found (%s)", sFilename);

			do
			{
				if ((wfData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)
					continue;

				int len = strlen(wfData.cFileName);
				if (nPathLen+len>=MAX_FILENAME_LENGTH)
					throw CError ("filename %s is too long", sFilename);

				memcpy (sPath+nPathLen, wfData.cFileName, len+1);

				Convert (sPath);
			} while (FindNextFile (hFind,&wfData));

			FindClose(hFind);
		}
	}
#else
	for (int i=0; i<m_nFilesCount; i++)
	{
		Convert (m_psFiles[i]);
	}
#endif
}


int main(int argc, char * argv[])
{
	try
	{
		CLogReader oLogReader (argc,argv);
		oLogReader.Run();
	}
	catch (CError oError)
	{
		oError.Display();
		return 0;
	}

	return 1;
}
