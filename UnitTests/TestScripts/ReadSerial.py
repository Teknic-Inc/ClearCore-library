# Serial Communications Library
import serial
# Regular expression operations library.
import re
# Date time for logging purposes
from datetime import datetime
# Argument Parsing
import argparse
# Timing used for timeouts
import time

LOG_FILE_NAME = "ClearCore_TestLog_%Y%m%d-%H%M%S.txt"
INPUT_FILE_NAME = "Dummy CC Test Output.txt"

TEST_TIMEOUT_SEC = 600

uploadPort = 'COM29'

# Batch Command to retrive the com port
#for /f "usebackq" %%B in (`wmic path Win32_SerialPort Where "PNPDeviceID LIKE 'USB\\VID_2890&PID_8022%%'" Get DeviceID 2^> nul ^| FINDSTR "COM"`) do echo %%B

def readData(comPort):
    didError = False
    
    print('Attempting to connect to Serial Port ' + comPort)
    # TODO: Check for port availability and attempt re-tries
    ser = serial.Serial(
        port=comPort,\
        baudrate=115200,\
        parity=serial.PARITY_NONE,\
        stopbits=serial.STOPBITS_ONE,\
        bytesize=serial.EIGHTBITS,\
            timeout=0)

    print("Serial Sniffer Connected to Port " + ser.portstr)
    count=1

    testReg = re.compile('(?i)(TEST)\(([A-Za-z0-9]+), ([A-Za-z0-9]+)')
    passTestReg =  re.compile('(?i)(TEST)\(([A-Za-z0-9]+), ([A-Za-z0-9]+)(\) - )([0-9]+)( ms)')
    errorReg = re.compile('(?i)error')
    failReg = re.compile('(?i)fail')
    finishReg = re.compile('(?i)Unit tests Complete')

    testDetailsPrev = False
    testDetails = False
    
    testDetailsTxt = ""

    testRunning = True
    timeOfLastReading = time.time();
    while testRunning:
        #this will store the line
        line = ""
        while True:
            c = ser.read(1)
            if c.decode("utf-8") == '\n':
                break
            if c == b'':
                #print("Empty Char")
                continue
            line = line + c.decode("utf-8")
            #print(line)
        #line = str(line)
        print('ClearCore:' + line)
    
        if passTestReg.match(line):
            #print('{:3d}'.format(count) + " Passes")
            testDetails = False
        elif testReg.match(line):
            #print('{:3d}'.format(count) + " Pending")
            testDetails = True
        elif errorReg.search(line) or failReg.search(line):
            #print('{:3d}'.format(count) + " Failed")
            didError = True
        elif finishReg.search(line):
            testRunning = false
        else:
            testDetails = True
            #print('{:3d}'.format(count) + " Unknown")

        if testDetails:
            #print('Saving line')
            testDetailsTxt = testDetailsTxt + '\n' + line

        if testDetailsPrev and (not testDetails):
            #print('{:3d}'.format(count) + " Message")
            #print(testDetailsTxt)
            testDetailsText = ""
        testDetailsPrev = testDetails
        count = count+1

    if didError:
        return 7
    else:
        return 0

def main():
    parser = argparse.ArgumentParser(description='Read Test Data from ClearCore')
    
    parser.add_argument('--port', '-p', nargs='?', type=str, default=uploadPort,
                        help='Specify port that ClearCore is on')

    args = parser.parse_args()
    

    ret = readData(uploadPort)
    if ret != 0:
        print("Tests failed!!!!!!")
    exit(ret)

if __name__ == "__main__":
    main()
