#include "stdafx.h"
#include "hostname.h"
#include "rconvlog.h"


enum FIELDS
{
	F_UNKNOWN,

	F_DATE,
	F_TIME,
	F_C_IP,
	F_CS_USERNAME,
	F_S_IP,
	F_S_PORT,
	F_CS_METHOD,
	F_CS_URI_STEM,
	F_CS_URI_QUERY,
	F_SC_STATUS,
	F_SC_BYTES,
	F_CS_BYTES,
	F_CS_HOST,
	F_CS_USER_AGENT,
	F_CS_REFERER,
	F_CS_VERSION,

	F_MAX
};


struct FIELDNAMES_W3C
{
	FIELDS type;
	char * str;
} fieldnames_w3c[] = {
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
};

#define FIELDNAMES_W3C_NUM (sizeof(fieldnames_w3c)/sizeof(FIELDNAMES_W3C))

typedef char * pchar;


class CLogReader
{
private:
	FILE * m_fInput;					// input file
	char * m_sLine;						// last line read from input file (parsed into fields)
	pchar * m_psFields;					// pointers to fields in m_sLine
	int m_iFields[F_MAX];				// IDs of fields in m_psFields
	int m_nMaxLineLength;				// length of m_sLine string
	int m_nMaxFieldCount;				// length of p_msFields and m_iFields
	int m_nFieldCount;					// actual count of fields in current line
	char m_sOutputDir[MAX_FILENAME_LENGTH+16];	// output directory + current filename
	int m_nOutputDirLength;				// length of directory part of m_sOutputDir
	char * m_sEmpty;					// pointer to "-" string
	CNameResolution * m_oNameRes;		// pointer to name resolution object or NULL
	char * m_sFilename;					// filename given in command line (may include wildcards)
	
	int m_nLines;						// no. of lines read while processing current file
	int m_nLinesWritten;				// no. of lines written while processing current file
	int m_nLinesTotal;					// total no. of lines read
	int m_nLinesWrittenTotal;			// total no. of lines written

	bool m_bOverwrite;					// append/rewrite flag - open file using "wt" if true and "at" if false
	char m_cInputType; // unused
	char * m_sGmtOffset; // unused
	bool m_bSaveNonWwwEntries; // unused
	int m_cLogType; // unused
	int m_cDateFormat; // unused

	int m_nStartTime;					// starting time (used to calculate total processing time)

	int ReadLine();						// reads a log line and fills m_sLine, m_psFields and m_iFields
	char * Field (FIELDS field);		// returns pointer to field content
	void Convert (char * sFilename);	// converts single file from W3C log to NCSA combined log
	void DisplayHelp();					// displays help
public:
	CLogReader (int argc, char * argv[], int nMaxLineLength=1024, int nMaxFieldCount=(F_MAX+4));
	~CLogReader();						
	void Run();							// does all the processing
};


/*
 * CLogReader::CLogReader - initializes CLogReader object and parses the command line
 *		argc,argv		= command line arguments
 *		nMaxLineLength	= length of m_sLine string
 *		nMaxFieldCount	= length of p_msFields and m_iFields
 */
CLogReader::CLogReader (int argc, char * argv[], int nMaxLineLength, int nMaxFieldCount)
{
	char * sOutputDir = NULL;
	bool bConvertIp = false;
	char * sCacheFile = NULL;
	bool bDisplayHelp = false;

	// do some initialization
	m_nMaxLineLength = nMaxLineLength;
	m_sLine = new char[nMaxLineLength+1];
	m_nMaxFieldCount = nMaxFieldCount;
	m_psFields = new pchar[nMaxFieldCount+1];
	m_sEmpty = strdup ("-");
	m_nFieldCount = 0;
	m_nStartTime = timeGetTime();

	m_cInputType = 0;
	m_sGmtOffset = NULL;
	m_bSaveNonWwwEntries = false;
	m_bOverwrite = false;
	m_cLogType = 0;
	m_cDateFormat = 0;
	m_oNameRes = NULL;
	m_sFilename = NULL;

	// command line parsing
	if (argc==1) bDisplayHelp = true;
	else {
		int i;
		for (i=1; i<argc; i++) if (strlen(argv[i])>0) { 
			if (argv[i][0]=='-') {
				switch (argv[i][1]) {
				case 'i': m_cInputType = argv[i][2]; break;
				case 't': m_sGmtOffset = argv[i+1]; i++; break;
				case 'o': sOutputDir = argv[i+1]; i++; break;
				case 'c': sCacheFile = argv[i+1]; i++; break;
				case 'x': m_bSaveNonWwwEntries = true; break;
				case 'd': bConvertIp = true; break;
				case 'w': m_bOverwrite = true; break;
				case 'l': m_cDateFormat = argv[i][2]-'0'; break;
				case 'b': m_cLogType = argv[i][2]-'0'; break;
				default: bDisplayHelp = true; break;
				}
			} else {
				m_sFilename = argv[i];
			}
		}
	}


	// display help if command line contains unknown option or filename is missing
	if (m_sFilename==NULL || bDisplayHelp) {
		DisplayHelp();
		throw CError();
	}

	// if -d option is present, create name resolution object 
	if (bConvertIp) {
		// use hostname cachefile if given
		if (strchr(sCacheFile,'\\')==NULL) {
			char sPath[MAX_FILENAME_LENGTH];
			int nLastBackslash;
			if (GetModuleFileName(GetModuleHandle(NULL),sPath,MAX_FILENAME_LENGTH)==NULL) throw CError ("unable to get rconvlog path");
			for (int i=0; sPath[i]; i++) {
				if (sPath[i]=='\\') nLastBackslash=i;
			}
			if ((i+strlen(sCacheFile)+1)>=MAX_FILENAME_LENGTH) throw CError ("cachefile path is too long");
			strcpy (sPath+nLastBackslash+1,sCacheFile);
			sCacheFile = sPath;
		}
		if (sCacheFile) printf ("\nHostname cache file: %s\n",sCacheFile);
		m_oNameRes = new CNameResolution (sCacheFile);
	}

	// make sure that the last char of output directory is backslash and the length is not too long
	m_nOutputDirLength=0;
	if (sOutputDir) {
		int nLen = strlen (sOutputDir);
		if (nLen>=MAX_FILENAME_LENGTH) throw CError ("output path is too long");
		if (nLen>0) {
			memcpy (m_sOutputDir,sOutputDir,nLen+1);
			m_nOutputDirLength = nLen;
		}
		if (m_sOutputDir[nLen-1]!='\\') {
			m_sOutputDir[nLen]='\\';
			m_nOutputDirLength++;
		}
	}
	m_sOutputDir[m_nOutputDirLength]=0;

	m_nLinesTotal = 0;
	m_nLinesWrittenTotal = 0;
}


CLogReader::~CLogReader ()
{
	// delete arrays and strings
	delete m_sLine;
	delete m_psFields;
	delete m_sEmpty;

	// display some useful statistics
	if (m_nLinesTotal>0) {
		printf ("\nTotals:\n=======\nTotal Lines Processed: %11d\n",m_nLinesTotal);
		printf ("Total Web Lines Written: %9d\n",m_nLinesWrittenTotal);
		printf ("Total time: %.2f\n",float((timeGetTime()-m_nStartTime))/1000.0f);
	}
	
	// delete name resolution object if present
	if (m_oNameRes) delete m_oNameRes;
}


/*
 * CLogReader::ReadLine - read one log line from input file
 *		(while the line begins with "#", process it and read other line)
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

	if (feof(m_fInput)) return 0;

	bEor = false;

	do {
		nCharPos = 0;
		nFieldPos = 1;
		bEol = false;
		m_psFields[0] = m_sLine;
		nLastChar = 32;

		if (fgets(m_sLine,m_nMaxLineLength,m_fInput)==NULL) {
			if (ferror(m_fInput)) throw CError ("read error");
			if (feof(m_fInput)) return 0;
		} else {
			if (m_sLine[0]) {
				m_nLines++;
				if (((m_nLines<1000) && ((m_nLines%100)==0)) || ((m_nLines%1000)==0)) {
					printf ("  %d lines processed\n",m_nLines);
				}
			}

			bEor = true;
			bFields = false;
			bSkip = false;
			if (m_sLine[0]=='#') {
				bEor = false;
				if (strncmp(m_sLine,"#Fields:",8)==0) bFields = true;
				else bSkip = true;
			}

			if (!bSkip) {
				while ((nChar=m_sLine[nCharPos])!=0) {
					if (nChar==0xD || nChar==0xA) {
						m_sLine[nCharPos]=0;
						break;
					}
					if (nChar==32) {
						m_sLine[nCharPos] = 0;
						if (nLastChar!=nChar) if (nFieldPos<m_nMaxFieldCount) {
							m_psFields[nFieldPos++] = m_sLine+nCharPos+1;
						}
					}
					nCharPos++;
					nLastChar = nChar;
				}
				m_nFieldCount = nFieldPos;

				if (bFields) {
					for (int i=0; i<FIELDNAMES_W3C_NUM; i++) {
						m_iFields[fieldnames_w3c[i].type]=-1;
						for (int j=1; j<m_nFieldCount; j++) {
							if (strcmp(m_psFields[j],fieldnames_w3c[i].str)==0) {
								m_iFields[fieldnames_w3c[i].type]=j-1;
								break;
							}
						}
					}
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
	return m_sEmpty;
}


void CLogReader::DisplayHelp()
{
	puts ("Rebex Internet Log Converter v0.8");
	puts ("Converts Microsoft Internet Information Services log files");
	puts ("to the NCSA Combined LogFile format");
	puts ("Copyright (C) 2001 Rebex s.r.o.\n");

	puts ("Usage: rconvlog [options] LogFile");
	puts ("Options:");
/*	puts ("-i<i|n|e> = input logfile type");
	puts ("    i - MS Internet Standard Log File Format");
	puts ("    n - NCSA Common Log File format");
	puts ("    e - W3C Extended Log File Format");
	puts ("-t <ncsa[:GMTOffset] | none> default is ncsa");*/
	puts ("-o <output directory> default = current directory");
	puts ("-c <hostname cache filename>");
//	puts ("-x save non-www entries to a .dmp logfile");
	puts ("-d = convert IP addresses to DNS");
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
}



static char * g_sMonths[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static int ConvertDate (char * sDateIn, char * sDateOut)
{
	if (strlen(sDateIn)!=10) return 0;
	int nMonth = (sDateIn[5]-'0')*10 + (sDateIn[6]-'0') - 1;
	if (nMonth>12 || nMonth<1) return 0;
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
	m_nLines = 0;
	m_nLinesWritten = 0;

	m_fInput = fopen (sFilename,"rt");
	if (!m_fInput) throw CError ("unable to open input file");

	printf ("\nOpening file %s for processing\n", sFilename);

	// TODO: check buffer overflow...
	if (m_nOutputDirLength+strlen(sFilename)>=MAX_FILENAME_LENGTH) throw CError ("logfile path is too long");
	strcpy (m_sOutputDir+m_nOutputDirLength,sFilename);
	if (m_oNameRes) strncat (m_sOutputDir,".ncsa.dns",9);
	else strncat (m_sOutputDir,".ncsa",5);

	FILE * fOutput;
	if (m_bOverwrite) {
		fOutput = fopen (m_sOutputDir,"wt");
		printf ("Writing");
	} else {
		fOutput = fopen (m_sOutputDir,"at");
		printf ("Appending");
	}
	printf (" file %s\n",m_sOutputDir);
	if (!fOutput) throw CError ("unable to open output file");
	
	char sDateNcsa[11];
	int nLineLength;
	char * sIp;
	char * s;
	char * sMethod;
	int nCsBytes;
	int nBytes;

	while (nLineLength=ReadLine()) if (nLineLength>=m_nFieldCount*2) 
	if (ConvertDate (Field(F_DATE),sDateNcsa)) {
		sMethod = Field(F_CS_METHOD);
		nBytes = atoi(Field(F_SC_BYTES));
		nCsBytes = atoi(Field(F_CS_BYTES));
		
		sIp = Field(F_C_IP);
		if (m_oNameRes) {
			s = m_oNameRes->Resolve (sIp);
			if (s) sIp = s;
		}
		fprintf (fOutput,"%s ",sIp);
		fprintf (fOutput,"- %s ", Field(F_CS_USERNAME));
		fprintf (fOutput,"[%s:%s +0000] ",sDateNcsa, Field(F_TIME));
		fprintf (fOutput,"\"");
		fprintf (fOutput,"%s ", sMethod);
		fprintf (fOutput,"%s ", Field(F_CS_URI_STEM));
//		fprintf (fOutput,"%s ", Field(F_CS_URI_QUERY));
		if (Field(F_CS_VERSION)) fprintf (fOutput,"%s", Field(F_CS_VERSION));
		else fprintf (fOutput,"HTTP/1.0");
		fprintf (fOutput,"\" ");
		fprintf (fOutput,"%s ", Field(F_SC_STATUS));

		switch (m_cLogType) {
		case 1: break;
		case 2: nBytes = nCsBytes;
		case 3: nBytes+= nCsBytes;
		default: if (strcmp(sMethod,"POST")==0) nBytes = nCsBytes; break;
		}

		fprintf (fOutput,"%d", nBytes);

		fprintf (fOutput," \"%s\"", Field(F_CS_REFERER));
		fprintf (fOutput," \"%s\"", Field(F_CS_USER_AGENT));
		fprintf (fOutput,"\n");
		m_nLinesWritten++;
	}

	fclose (fOutput);
	fclose (m_fInput);

	printf ("%s completed, %d lines processed.\n",sFilename,m_nLines);
	printf ("%d Web lines written\n",m_nLinesWritten);
	printf ("%d non-www lines discarded\n",m_nLines-m_nLinesWritten);

	m_nLinesWrittenTotal += m_nLinesWritten;
	m_nLinesTotal += m_nLines;
}


void CLogReader::Run ()
{
	WIN32_FIND_DATA wfData;
	HANDLE hFind;
	int nFiles=0;

	hFind = FindFirstFile (m_sFilename,&wfData);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			if ((wfData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
				Convert (wfData.cFileName);
				nFiles++;
			}
		} while (FindNextFile (hFind,&wfData));
		FindClose(hFind);
	} else throw CError ("file not found");
}


int main(int argc, char * argv[])
{
	try {
		CLogReader oLogReader (argc,argv);
		oLogReader.Run();
	} catch (CError oError) {
		oError.Display();
		return 0;
	}

	return 1;
}




