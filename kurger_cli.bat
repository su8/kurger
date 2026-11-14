@ECHO OFF
SETLOCAL
TITLE kurger_cli

IF EXIST "%~dp0kurger_cli.exe" (
  SET "KURGER_CLI=%~dp0kurger_cli.exe"
) ELSE (
  SET "KURGER_CLI=kurger_cli.exe"
)

:loop
CLS
ECHO Usage example:
ECHO   from 1 to 10 resolution 100 file "book.pdf"
ECHO.
SET "ARGS="
SET /P "ARGS=kurger_cli> "
IF NOT DEFINED ARGS GOTO :eof

"%KURGER_CLI%" %ARGS%
ECHO.
PAUSE
GOTO loop
