#ifndef _RCONVLOG_H_
#define _RCONVLOG_H_

#include "misc.h"

typedef char * pchar;

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

class CLogReader
{
private:
	FILE * m_fInput;                           // input file
	char * m_sLine;                            // last line read from input file (parsed into fields)
	pchar * m_psFields;                        // pointers to fields in m_sLine
	int m_iFields[F_MAX];                      // IDs of fields in m_psFields
	int m_nMaxLineLength;                      // length of m_sLine string
	int m_nMaxFieldCount;                      // length of p_msFields and m_iFields
	int m_nFieldCount;                         // actual count of fields in current line
	char m_sOutputDir[MAX_FILENAME_LENGTH+16]; // output directory + current filename
	int m_nOutputDirLength;                    // length of directory part of m_sOutputDir
	char * m_sEmpty;                           // pointer to "-" string
	CNameResolution * m_oNameRes;              // pointer to name resolution object or NULL
	pchar * m_psFiles;                         // list of filenames given in command line (may include wildcards on WIN32)
	int m_nFilesCount;                         // number of filenames given in command line
	char m_sLastDate[16];                      // last date read from #Date line

	int m_nLines;                              // no. of lines read while processing current file
	int m_nLinesTotal;                         // total no. of lines read
	int m_nLinesWrittenTotal;                  // total no. of lines written

	bool m_bOverwrite;                         // append/rewrite flag - open file using "wt" if true and "at" if false
	char m_cInputType;                         // currently unused
	char * m_sGmtOffset;                       // currently unused
	bool m_bSaveNonWwwEntries;                 // currently unused
	int m_cLogType;                            // currently unused
	int m_cDateFormat;                         // currently unused

	int m_nReportInterval;                     // time interval between progress reports (in seconds)
	clock_t m_ctStartTime;                     // starting time
	time_t m_ttIgnoreTime;                     // ignore threshold - ignore entry if older

	int ReadLine();                            // read a log line and fill m_sLine, m_psFields and m_iFields
	char * Field (FIELDS field);               // return pointer to field content
	void Convert (char * sFilename);           // convert single file from W3C log format to NCSA combined log format
	void DisplayHelp();                        // display help
	void ProgressReport();

public:
	CLogReader (int argc, char * argv[], int nMaxLineLength=1024, int nMaxFieldCount=64, int nReportInterval=10);
	~CLogReader();
	void Run();                                // do all the processing
};


class CError
{
private:
	char * m_sMessage;
	char * m_sParam;
public:
	CError (char * sMessage) {m_sMessage = sMessage;}
	CError (char * sMessage, char * sParam) {m_sMessage = sMessage; m_sParam = sParam;}
	CError () {m_sMessage = NULL; m_sParam = NULL;}

	void Display()
	{
		if (m_sMessage)
		{
			printf ("error: ");
			if (m_sParam==NULL)
				printf ("%s",m_sMessage);
			else
				printf (m_sMessage, m_sParam);
			printf ("\n");
		}
	}
};

#endif
