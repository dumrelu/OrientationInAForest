import sys
from PIL import Image

openColor = (125, 68, 29)
roadColor = (105, 105, 105)
treeColor = (30, 147, 45)
cliffColor = (0, 0, 0)

if len(sys.argv) < 3:
    print("The script requires 2 inputs, the name of the image and the name of the output map.")
    sys.exit(1)

imageName = sys.argv[1]
mapName = sys.argv[2]

print("Input image name: " + imageName)
print("Output map file name: " + mapName)
print("OPEN zone color: " + str(openColor))
print("ROAD zone color: " + str(roadColor))
print("TREE zone color: " + str(treeColor))
print("CLIFF zone color: " + str(cliffColor))

image = Image.open(imageName)
width, height = image.size
print("Image height: " + str(height))
print("Image width: " + str(width))

with open(mapName, "wt") as file:
    file.write(str(height) + " " + str(width) + "\n")
    for y in range(0, height):
        for x in range(0, width):
            pixel = image.getpixel((x, y))
            if pixel == openColor:
                file.write('O')
            elif pixel == roadColor:
                file.write('R')
            elif pixel == treeColor:
                file.write('T')
            else:
                file.write('C')
        file.write('\n')
