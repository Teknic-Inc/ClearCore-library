:: Name:     flash_clearcore.cmd
:: Purpose:  Flash a connected ClearCore device with a compiled binary file.
:: Author:   Zachary Lebold
:: Revision: January 2020 - initial version

@echo off

set versionNumber=1.0.11

rem ---------------------------------------------------------------------------
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
rem ------------------------------------------------------------------------
set "waitOneSecond=ping 192.0.2.2 -n 1 -w 1000 > nul"
set "waitTwoSeconds=ping 192.0.2.2 -n 1 -w 2000 > nul"
set "waitFiveSeconds=ping 192.0.2.2 -n 1 -w 5000 > nul"

set isComPort="PNPDeviceID LIKE 'USB\\VID_2890&PID_8022%%'"
set isBootPort="PNPDeviceID LIKE 'USB\\VID_2890&PID_0022%%'"

set inBootloader=0

set maxComPortFindAttempts=10
set comPortFindAttempt=0

:find_binary
rem ---------------------------------------------------------
rem Try to grab the binary file from a command line argument.
rem ---------------------------------------------------------

if "%~1"=="" (
    echo Error: You must supply the location of a binary file as the argument to this script.
    %waitFiveSeconds%
    exit /b 1
)
if not exist "%~f1" (
    echo Error: The supplied filepath "%~f1" does not exist! Please provide a valid path to a binary file.
    %waitFiveSeconds%
    exit /b 2
)
if not "%~x1"==".bin" (
    echo Error: You must supply a binary file ^(.bin^)! Supplied file is of extension: %~x1.
    %waitFiveSeconds%
    exit /b 3
)
    
echo Using binary file at: %~f1
echo.


:find_bossac
rem ------------
rem Find bossac.
rem ------------

set "bossacPath=%~dp0\bossac\bossac.exe"

if not exist "%bossacPath%" (
    echo Error: Could not find bossac uploader!
    %waitFiveSeconds%
    exit /b 4
)

echo Found bossac at: %bossacPath%
echo.

:find_boot_port
rem ----------------------------
rem Find the ClearCore COM port.
rem ----------------------------

if %comPortFindAttempt% gtr 0 (
    echo Finding ClearCore bootloader COM port... ^(Attempt %comPortFindAttempt%/%maxComPortFindAttempts%^)
) else (
    echo Finding ClearCore bootloader COM port... 
)
echo.

set comPort=

rem ----------------------------------------------------------------------------
rem Enumerate the COM ports and grab the one with the VID/PID for a ClearCore bootloader.
rem ----------------------------------------------------------------------------
for /f "usebackq" %%B in (`wmic path Win32_SerialPort Where %isBootPort% Get DeviceID 2^> nul ^| FINDSTR "COM"`) do set comPort=%%B

if not defined comPort (
    if %comPortFindAttempt% equ 0 (
        echo Couldn't find the bootloader port. Searching for a regular ClearCore port...
        goto find_com_port
    ) else if %comPortFindAttempt% lss %maxComPortFindAttempts% (
        set /a comPortFindAttempt=%comPortFindAttempt%+1
        echo Couldn't find the port. Trying again...
        %waitTwoSeconds%
        goto find_boot_port
    ) else (
        echo Got error code: %errorlevel%
        echo Error: Couldn't find a ClearCore on one of the COM ports!
        %waitOneSecond%
        exit /b 5
    )
) else (
    echo Found ClearCore bootloader device on %comPort%.
    echo.
    goto chip_flash
)

:find_com_port
rem ----------------------------
rem Find the ClearCore COM port.
rem ----------------------------

echo Finding ClearCore COM port...
echo.

set comPort=

rem ----------------------------------------------------------------------------
rem Enumerate the COM ports and grab the one with the VID/PID for a ClearCore device.
rem ----------------------------------------------------------------------------
for /f "usebackq" %%B in (`wmic path Win32_SerialPort Where %isComPort% Get DeviceID 2^> nul ^| FINDSTR "COM"`) do set comPort=%%B

if not defined comPort (
    echo Got error code: %errorlevel%
    echo Error: Couldn't find a ClearCore on one of the COM ports!
    %waitOneSecond%
    exit /b 6
) else (
    set inBootloader=1
)

echo Found ClearCore device on %comPort%.
echo.

rem ---------------
rem Enter bootloader mode.
rem ---------------

echo Enter bootloader mode...
echo.

"%bossacPath%" --info --debug --arduino-erase --port=%comPort%

rem ----------------------------------------------------------------------------
rem In case we get a "Failed to open port at 1200bps" error as the ClearCore 
rem switches from the running application to bootloader mode, this will give  
rem us enough time for the port to become available again.
rem Otherwise, if we don't wait here, we won't find the port and the subsequent
rem flash will fail.
rem ----------------------------------------------------------------------------
if %errorlevel% neq 0 (
    %waitOneSecond%
)

set inBootloader=1
set comPortFindAttempt=1
set portRemovalAttempt=0
goto find_boot_port

:chip_flash
rem ----------------------
rem Flash the binary file.
rem ----------------------

"%bossacPath%" --info --debug --port=%comPort% --usb-port --write --erase --verify --offset=0x4000 --reset "%~f1"

if %errorlevel% equ 0 (
    echo Success!
    %waitOneSecond%
    goto boot_port_removal
) else (
    echo Got error code: %errorlevel%
    echo Error: Failed to flash!
    exit /b 7
)

:boot_port_removal
set testPort=
for /f "usebackq" %%B in (`wmic path Win32_SerialPort Where "DeviceID LIKE '%comPort%'" Get DeviceID 2^> nul ^| FINDSTR "COM"`) do set testPort=%%B

if defined testPort (
    set /a portRemovalAttempt = %portRemovalAttempt%+1
    if %portRemovalAttempt% gtr 15 (
        echo Timed out waiting for bootloader port removal
        exit /b 8
    )
    %waitOneSecond%
    goto boot_port_removal
) else (
    exit /b 0
)