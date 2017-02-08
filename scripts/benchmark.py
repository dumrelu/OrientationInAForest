import os
import sys
import subprocess
import datetime
import shutil

minNumOfProcs = 0
maxNumOfProcs = 0
if len(sys.argv) < 3:
    print("The script requires the minimum and maximum number of processes as input.")
    sys.exit(1)
minNumOfProcs = int(sys.argv[1])
maxNumOfProcs = int(sys.argv[2])

increment = 1
if len(sys.argv) >= 4:
    increment = int(sys.argv[3])

tests = [
    {
        'map_name': 'medium_map.png', 
        'options': '-p -s -x 494 -y 959 -d 1'
    }
]

benchmarkDir = 'benchmark_' + datetime.datetime.today().strftime('%Y_%m_%d_%H_%M_%S')
os.mkdir(benchmarkDir)
print('Benchmark results folder: ' + benchmarkDir)

# shutil.copytree(benchmarkDir, 'test/test')

def get_latest_directory():
    all_subdirs = [d for d in os.listdir('.') if os.path.isdir(d)]
    return max(all_subdirs, key=os.path.getmtime)

def get_test_name(test, procs):
    return 'output_' + test['map_name']  + '_' + str(procs) +  '_'.join(test['options'].split(' '))

print('Running ' + str(len(tests)) + ' tests...')
for procs in range(minNumOfProcs, maxNumOfProcs + 1, increment):
    print('Running tests for ' + str(procs) + ' processes')
    for test in tests:
        mapName = test['map_name']
        options = test['options']

        print('Running test: ' + str(test))

        subprocess.call('python run_program.py ' + str(procs) 
            + ' ' + mapName
            + ' ' + options)
            #, stdout=subprocess.PIPE)

        outputDir = get_latest_directory()
        testDirName = get_test_name(test, procs)
        shutil.move(outputDir, benchmarkDir + '/' + testDirName)