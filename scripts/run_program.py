import os
import sys
import subprocess
import shutil

# Parse arguments
numOfProcs = 0
if len(sys.argv) < 2:
    print("The script requires the number of processes and the map image as input.")
    sys.exit(1)
numOfProcs = int(sys.argv[1])
mapImageName = sys.argv[2]

mapFilename = os.path.splitext(mapImageName)[0] + ".map"
if os.path.isfile(mapFilename):
    print("Map output file found")
else:
    print("Generating the map...")
    if subprocess.call("python image_to_map.py " + mapImageName + " " + mapFilename):
        print("Error while generating the map!")
        sys.exit(1)

optionsList = ["-m", mapFilename] + [sys.argv[i] for i in range(3, len(sys.argv))]
options = ' '.join(optionsList)


#Run the program
print("Running orientation.exe with " + str(numOfProcs) + " processes.")
print("orientation.exe options: " + options)

subprocess.call("mpiexec -n " + str(numOfProcs) + " orientation.exe " + options)

#Move the results to the output folder
currentDir = os.getcwd()
outputDir = "output"

if os.path.isdir(outputDir):
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
    if os.path.isfile(filename):
        os.rename(filename, outputDir + "/" + filename)

locationPathFilename = "location_finding.path"
pathFindingPathFilename = "path_finding.path"
statisticsFilename = "statistics.txt"
logFilenames = ["process_" + str(i) + "_log.txt" for i in range(0, numOfProcs)]
pathedMapFilename = "path_on_map" + os.path.splitext(mapImageName)[1]


print("Writing the paths")
subprocess.call("python print_path_on_map.py " + mapImageName + " " + locationPathFilename + " " + pathedMapFilename + " 0 191 255")
subprocess.call("python print_path_on_map.py " + pathedMapFilename + " " + pathFindingPathFilename + " " + pathedMapFilename + " 255 140 0")

print("Copying output files to the " + outputDir + " directory...")
move_file(locationPathFilename)
move_file(pathFindingPathFilename)
move_file(statisticsFilename)
move_file(pathedMapFilename)
for logFile in logFilenames:
    move_file(logFile)
