@cl rconvlog.cpp -c -O2 -GF -GX -MD -Gy -W3 -nologo
@cl hostname.cpp -c -O2 -GF -GX -MD -Gy -W3 -nologo
@cl misc.cpp -c -O2 -GF -GX -MD -Gy -W3 -nologo
@link rconvlog.obj hostname.obj misc.obj /OUT:"rconvlog.exe" /NOLOGO /SUBSYSTEM:CONSOLE ws2_32.lib
