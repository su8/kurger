:TOP
@CLS
@ECHO OFF
echo from 1 to 10 resolution 100 file 'book.pdf'
SET /P NAME=
:BEGIN
COPY kurger_cli.exe C:\MingW\bin\kurger_cli.exe
CLS
:loop
C:\MingW\bin\kurger_cli.exe %name%
PAUSE
GOTO loop