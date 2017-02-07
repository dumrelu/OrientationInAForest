import os
import sys
import subprocess
import shutil

# Parse arguments
numOfProcs = 0
if len(sys.argv) < 2:
    print("The script requires the number of processes as input.")
    sys.exit(1)
numOfProcs = int(sys.argv[1])

optionsList = [sys.argv[i] for i in range(2, len(sys.argv))]
options = ' '.join(optionsList)


#Run the program
print("Running orientation.exe with " + str(numOfProcs) + " processes.")
print("orientation.exe options: " + options)

subprocess.call("mpiexec -n " + str(numOfProcs) + " orientation.exe " + options)

#Move the results to the output folder
currentDir = os.getcwd()
outputDir = "output"

if(os.path.isdir(outputDir)):
    i = 2
    while True:
        nextOutputDir = outputDir + str(i)
        i = i + 1
        if not os.path.isdir(nextOutputDir):
            outputDir = nextOutputDir
            break


# if(os.path.isdir(outputDir)):
#     shutil.rmtree(outputDir, ignore_errors=True)
os.mkdir(outputDir)

def move_file(filename):
    if(os.path.isfile(filename)):
        os.rename(filename, outputDir + "/" + filename)

locationFilename = "location_finding.path"
pathFindingFilename = "path_finding.path"
statisticsFilename = "statistics.txt"
logFilenames = ["process_" + str(i) + "_log.txt" for i in range(0, numOfProcs)]

print("Copying output files to the " + outputDir + " directory...")
move_file(locationFilename)
move_file(pathFindingFilename)
move_file(statisticsFilename)
for logFile in logFilenames:
    move_file(logFile)