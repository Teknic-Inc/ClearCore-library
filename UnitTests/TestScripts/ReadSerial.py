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


uploadPort = 'COM29'
baudRate = 115200


# Batch Command to retrive the com port
#for /f "usebackq" %%B in (`wmic path Win32_SerialPort Where "PNPDeviceID LIKE 'USB\\VID_2890&PID_8022%%'" Get DeviceID 2^> nul ^| FINDSTR "COM"`) do echo %%B

def readSerialData(comPort):
    didError = False
    
    print('Attempting to connect to Serial Port ' + comPort)
    # TODO: Check for port availability and attempt re-tries
    ser = serial.Serial()
    ser.baudrate = baudRate
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.bytesize = bytesize=serial.EIGHTBITS
    ser.timeout = 1000
    ser.port=comPort
    
    connectionAttempts = 1
    while not ser.is_open:
        try:
            ser.open()
        except:
            print("Attempt " + str(connectionAttempts) + " failed to connect to " + comPort)
            connectionAttempts = connectionAttempts + 1
            if connectionAttemps > 10:
                print("Could not open " + comPort)
                return 2
            time.sleep(1)
            
    

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
            testRunning = False
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
    
    ser.close()
    if didError:
        return 7
    else:
        return 0

def main():
    parser = argparse.ArgumentParser(description='Read Test Data from ClearCore')
    
    parser.add_argument('--port', '-p', nargs='?', type=str, default=uploadPort,
                        help='Specify port that ClearCore is on')
    parser.add_argument('--baud', nargs='?', type=int, default=115200,
                        help='Specify baud rate')

    args = parser.parse_args()
    
    baudRate = args.baud
    ret = readSerialData(args.port)
    if ret != 0:
        print("Tests failed!")
    exit(ret)

if __name__ == "__main__":
    main()
