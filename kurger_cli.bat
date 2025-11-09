:TOP
@CLS
@ECHO OFF
TITLE kurger_cli
:BEGIN
if not exist C:\MingW\bin\kurger_cli.exe (
  COPY kurger_cli.exe C:\MingW\bin\kurger_cli.exe
)
CLS
:loop
echo from 1 to 10 resolution 100 file book.pdf  # without quotes for book.pdf
SET /P NAME=
C:\MingW\bin\kurger_cli.exe %name%
PAUSE
GOTO loop