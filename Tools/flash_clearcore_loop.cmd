:: Name:     flash_clearcore_loop.cmd
:: Purpose:  Flash a connected ClearCore device with a compiled binary file.
::           Do this in a loop until an upload error is discovered.
:: Author:   Zachary Lebold
:: Revision: February 2020 - initial version

@echo off

set versionNumber=1.0.11

rem ----------------------------------------------------------------------------
rem An awful but necessary hack because timeout causes redirection issues, even
rem with the /nobreak argument that prevents user input from interrupting
rem the wait time.
rem Problem: The timeout command causes the input to get redirected in order to
rem wait for user input to optionally interrupt the wait time. When a batch
rem script with timeout commands runs in the background, the input cannot be
rem redirected to allow for user input and we fail with the following error:
rem "ERROR: Input redirection is not supported, exiting the process immediately"
rem
rem The sleep command would be great, but it is not available on all systems.
rem
rem Solution: We're using ping simply as a timeout because it seems to be the
rem only way...
rem IP addresses of the form 192.0.2.x are reserved under RFC 3330. Therefore
rem pinging this kind of IP will never succeed. We ping once, so we will
rem just end up waiting for the specified timeout (in milliseconds).
rem ----------------------------------------------------------------------------
set "waitOneSecond=ping 192.0.2.2 -n 1 -w 1000 > nul"
set "waitTwoSeconds=ping 192.0.2.2 -n 1 -w 2000 > nul"
set "waitFiveSeconds=ping 192.0.2.2 -n 1 -w 5000 > nul"

set inBootloader=0

set flashCount=1

if not exist "%~dp0\flash_clearcore.cmd" (
    echo Error: Could not find flash_clearcore.cmd script! 
    echo.
    %waitFiveSeconds%
    exit /b 404
)

:flash
echo Starting flash attempt %flashCount%...

call "%~dp0\flash_clearcore.cmd" %1

echo %errorlevel%
if %errorlevel% equ 0 (
    set /a flashCount=%flashCount%+1
    goto flash
) else (
    echo Exiting for error %errorlevel% on flash attempt %flashCount%
    exit /b %errorlevel%
)
