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
# To call external programs
import subprocess
# For OS Fns
import os
# Path generation
from pathlib import Path
# For file manipulation
import shutil

# Important Directory Locations
testScriptLoc = os.path.join(os.path.realpath(__file__), "../")
testLoc = os.path.join(testScriptLoc, "../")
testSketchesLoc = os.path.join(testLoc, "TestSketches")
teknicLoc = os.path.join(testLoc, "../")
libClearCoreLoc = os.path.join(teknicLoc, "libClearCore")
clearCoreArduinoLoc = os.path.join(teknicLoc, "../")

flashClearCoreSciptLoc = clearCoreArduinoLoc
flashClearCoreScript = "flash_clearcore.cmd"

atmelConfigType = "Test"
atmelOutFile = "AtmelOutput.txt"

binaryLoc = os.path.join(clearCoreArduinoLoc, atmelConfigType)
binaryName = "ClearCoreArduino.bin"

uploadPort = 'COM29'
baudRate = 115200


# print( "testScriptLoc  " + os.path.abspath(testScriptLoc))
# print( "testLoc  " + os.path.abspath(testLoc))
# print( "testSketchesLoc  " + os.path.abspath(testSketchesLoc))
# print( "teknicLoc  " + os.path.abspath(teknicLoc))
# print( "libClearCoreLoc  " + os.path.abspath(libClearCoreLoc))
# print( "clearCoreArduinoLoc  " + os.path.abspath(clearCoreArduinoLoc))


def execute(command):
    print(' '.join(command))
    process = subprocess.Popen(command, 
                               shell=False, 
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.STDOUT)
    output = "".encode('ascii')

    # Poll process for new output until finished
    for line in iter(process.stdout.readline, b''):
        print(line.decode().strip('\n'), end=None)
        output += line
        
    process.wait()
    exitCode = process.returncode

    if exitCode == 0:
        return output
    else:
        raise Exception(command, exitCode, output)

def find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result

# Batch Command to retrive the com port
#for /f "usebackq" %%B in (`wmic path Win32_SerialPort Where "PNPDeviceID LIKE 'USB\\VID_2890&PID_8022%%'" Get DeviceID 2^> nul ^| FINDSTR "COM"`) do echo %%B
def readSerialData(comPort):
    didError = False
    
    print('Attempting to connect to Serial Port ' + comPort)
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
        except PermissionError as error:
            print("Access to port " + comPort + " is denided")
            return 9
        except:
            print("Attempt " + str(connectionAttempts) + " failed to connect to " + comPort)
            connectionAttempts = connectionAttempts + 1
            if connectionAttempts > 10:
                print("Could not open " + comPort)
                return 8
            time.sleep(1)
            
    

    print("Serial Sniffer Connected to Port " + ser.portstr)
    count=1

    testReg = re.compile('(?i)(TEST)\(([A-Za-z0-9]+), ([A-Za-z0-9]+)')
    passTestReg =  re.compile('(?i)(TEST)\(([A-Za-z0-9]+), ([A-Za-z0-9]+)(\) - )([0-9]+)( ms)')
    errorReg = re.compile('(?i)error')
    failReg = re.compile('(?i)fail')
    finishReg = re.compile('(?i)tests Complete')
    finishReg2 = re.compile('(?i)Tests Finished')

    testDetailsPrev = False
    testDetails = False
    
    testDetailsTxt = ""

    testRunning = True
    timeOfLastReading = time.time()
    while testRunning:
        #this will store the line
        line = ""
        while True:
            c = ser.read(1)
            if time.time() - timeOfLastReading > 600:
                # Put something in the line
                line = line + "Serial Read Timeout"
                didError = True
                testRunning = False
                break;
            if c.decode("utf-8") == '\n':
                break
            if c == b'':
                #print("Empty Char")
                continue
            timeOfLastReading = time.time()
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
            #testRunning = False
        elif finishReg.search(line) or finishReg2.search(line):
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

def copyTestSketch(sketchName="UnitTestRunner.cpp"):
    print("Copying Test Sketch")
    srcFile = os.path.join(testSketchesLoc, sketchName)
    dstFile = os.path.join(clearCoreArduinoLoc, "TestSketch.cpp")
    print("Removing Current Test Sketch " + os.path.abspath(dstFile))
    try: 
        os.remove(os.path.abspath(dstFile)) 
    except OSError as error: 
        print(error)
        print("File " + os.path.abspath(dstFile) + " can not be removed") 
    print("Copying " + os.path.abspath(srcFile) + " to " + os.path.abspath(dstFile))
    try:
        shutil.copy(os.path.abspath(srcFile), os.path.abspath(dstFile))
    except:
        print("Could Not copy " + os.path.abspath(srcFile) + " to " + os.path.abspath(dstFile))
        return False
    return True
    
def findTestSketches():
    files = []
    for file in os.listdir(testSketchesLoc):
        if file.endswith(".cpp"):
            #files.append(os.path.join(os.path.abspath(testSketchesLoc), file))
            files.append(file)
    print(files)
    return files
    
def flashClearCore():
    print("Flashing binary to ClearCore")
    #cd ClearCore_Arduino/
    #flash_clearcore.cmd Test/ClearCoreArduino.bin
    flashCmd = [ os.path.join(flashClearCoreSciptLoc, flashClearCoreScript),
                os.path.abspath(os.path.join(binaryLoc, binaryName)) ]
    
    try:
        execute(flashCmd)
    except subprocess.CalledProcessError as e:
        print("Flashing ClearCore Failed")
        sys.exit(97)
    except:
        print("Flashing ClearCore Failed")
    return True

def buildProject():
    print("Building Atmel Project")
    #cd ClearCore_Arduino/
    #atmelstudio.exe ClearCore.atsln /rebuild Test /out AtmelOutput.txt || ( type AtmelOutput.txt & EXIT /B 1 )
    buildCmd = [ "atmelstudio.exe",
                os.path.abspath(os.path.join(clearCoreArduinoLoc, "ClearCore.atsln")),
                "/rebuild", atmelConfigType, "/out", atmelOutFile]
    
    try:
        execute(buildCmd)
        with open(atmelOutFile, 'r') as f:
            print(f.read())
    except subprocess.CalledProcessError as e:
        print("Building ClearCore Failed")
        print(e)
        with open(atmelOutFile, 'r') as f:
            print(f.read())
        exit(98)
    except:
        print("Building ClearCore Failed")
        with open(atmelOutFile, 'r') as f:
            print(f.read())
        exit(98)
    return True

def main():

    parser = argparse.ArgumentParser(description='Read Test Data from ClearCore')
    
    parser.add_argument('--port', '-p', nargs='?', type=str, default=uploadPort,
                        help='Specify port that ClearCore is on')
    parser.add_argument('--baud', nargs='?', type=int, default=115200,
                        help='Specify baud rate')

    args = parser.parse_args()
    
    baudRate = args.baud
    
    for testSketch in findTestSketches():
        print("Running Test Sketch " + testSketch)
        #ret = copyTestSketch(testSketch)
        ret = copyTestSketch(testSketch)
        if not ret:
            exit(1)
        ret = buildProject()
        if not ret:
            exit(2)
        ret = flashClearCore()
        if not ret:
            exit(3)
        
        ret = readSerialData(args.port)
        if ret != 0:
            print("Tests failed!")
            exit(ret)
            
    exit(0)

if __name__ == "__main__":
    main()
