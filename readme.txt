 
                         =========================
                            Rebex CONVLOG v1.10
                         =========================


IIS is shipped with the program called convlog.exe, which is able to convert W3C
Extended log files (created by IIS) into NCSA Common Log format. It can be useful
when some log analyzer is used, such as the excellent webalizer (http://www.webalizer.org/),
which does not support native IIS format or W3C Extended format.

However, using Microsoft convlog causes some problems:
- referers and user agents (browsers) are lost during conversion
- upload is ignored (which is bad if converting FTP log files)

RCONVLOG solves those problems - it is nearly identical to genuine convlog,
but it does work as expected :-)

Advantages of RCONVLOG:
- browsers and referers are fully supported
- upload, download or both can be choosed for conversion
- RCONVLOG uses cache mechanism for resolving IP addresses to DNS names,
  which greatly improves preformance

Note: Currently, RCONVLOG only understands W3C Extended format. If you need
to process MS Internet Standard Log File Format or NCSA Common Log format,
mail us - we will add it upon first request.


Availability:

FREEWARE. Can be downloaded from http://www.rebex.cz/


Usage:

rconvlog [options] LogFile

LogFile
    File(s) is W3C format to be converted - may contain wildcards such
    as ? and *.

-o <output directory>
    Directory where logfiles in NCSA Combined format will be created.
    Default is current directory. Created files will have filename
    of the original ones + extension ".ncsa" (or "ncsa.dns" for -d option).

-c <hostname cache file>
    Use this file as hostname cache. If filename without path is specified,
    path of rconvlog.exe (use .\filename to specify
    file in current directory). Cache saves time when executing RCONVLOG
    multiple - resolved hostnames can be reused.

-t <ncsa[:GMTOffset]>
    Specifies GTM Offset to use in NCSA logfile. Default is ncsa:+0000.

-d  Convert IP addresses to DNS hostnames if possible.

-w  Overwrite existing NCSA log files (default is append).
    
-b<0|1|2|3>
    How to log bytes sent:
    0 = be compatible with convlog (default)
    1 = log bytes sent from server to client only (download)
    2 = log bytes sent from client to server only (upload)
    3 = log sum of bytes sent both ways (upload and download)

-n YYYY-MM-DDTHH:NN:SS
    Ignore records older than specified datetime (eg. -n 2001-12-20T00:00:00)

-h H
    Ignore records older than H hours (eg. -h 164)

(c) Rebex, Lukas Pokorny 2001. For latest version mail to lukas.pokorny@rebex.cz
or dowload it from http://wwww.rebex.cz

