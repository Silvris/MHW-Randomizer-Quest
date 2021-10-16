from pathlib import Path
import sys
import os
import struct

def ReadUInt(file):
    return struct.unpack("I",file.read(4))[0]

def GetUIntFromBytes(buffer,start,end):
    return struct.unpack("I",buffer[start:end])[0]

def GetBytesFromUInt(value):
    return struct.pack("I",value)

chunkPath = r"D:\MHW\chunk"
outputPath = r"C:\Users\silve\Documents\GitHub\MHW-Randomizer-Quest"

monsterIds = [0,1,7,9,10,11,12,13,14,16,17,18,19,21,22,24,27,28,29,30,31,32,33,34,35,37,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,87,88,89,90,91,92,93,94,95,99,100]

#special arena, challenge arena, and Seliana Supply Cache all work without alnk edits as they are created as "arena" maps
#I don't use Xeno's alnk data here, as he has 2 paths going from 0 to 1, and I don't want to make monsters leave players (without at least adding in warps between the two levels)
confluence = bytearray([0x95,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x95,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3B,0x45,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00])
origin1 = bytearray([0x9C,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x9C,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x43,0x47,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00])
origin2 = bytearray([0x9D,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x9D,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3B,0x45,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00])
secluded = bytearray([0xA0,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0xA0,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00])
schrade = bytearray([0xA1,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0xA1,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0xBC,0xBE,0x4C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00])

def readWriteALNK(inFile,outFile):
    mainHeader = inFile.read(40) #leave out the last 4
    mapCount = ReadUInt(inFile)
    maps = []
    mapIds = []
    for i in range(mapCount):
        mapData = bytearray()
        mapID = ReadUInt(inFile)
        mapIds.append(mapID)
        mapData.extend(GetBytesFromUInt(mapID))
        pathCount = ReadUInt(inFile)
        mapData.extend(GetBytesFromUInt(pathCount))
        for j in range(pathCount):
            mapData.extend(inFile.read(4)) #map id again
            routePackCount = ReadUInt(inFile)
            mapData.extend(GetBytesFromUInt(routePackCount))
            for k in range(routePackCount):
                mapData.extend(inFile.read(14))
                routeCount = ReadUInt(inFile)
                mapData.extend(GetBytesFromUInt(routeCount))
                for l in range(routeCount):
                    mapData.extend(inFile.read(4))
                    destCount = ReadUInt(inFile)
                    mapData.extend(GetBytesFromUInt(destCount))
                    for m in range(destCount):
                        mapData.extend(inFile.read(8))
                mapData.extend(inFile.read(3))
            pathCloser = ReadUInt(inFile)
            mapData.extend(GetBytesFromUInt(pathCloser))
            if pathCloser != 0:
                mapData.extend(inFile.read(16))#extra info
            mapData.extend(inFile.read(4))#was originally update versions, seems to be zero'd out now
        maps.append(mapData)
    if(405 not in mapIds):
        #confluence not present
        maps.append(confluence)
    if(412 not in mapIds):
        maps.append(origin1)
    if(413 not in mapIds):
        maps.append(origin2)
    if(416 not in mapIds):
        maps.append(secluded)
    if(417 not in mapIds):
        maps.append(schrade)
    outFile.write(mainHeader)
    outFile.write(GetBytesFromUInt(len(maps)))
    for i in range(len(maps)):
        outFile.write(maps[i])
    inFile.close()
    outFile.close()

def getAllFromChunk():
    for path in Path(chunkPath).rglob("*.dtt_alnk"):
        print(path)
        filePath = str(path).replace('\\','/').split(r"chunk/")[1]
        dirPath = outputPath + "/nativePC/" + filePath
        osPath = os.path.split(dirPath)[0]
        if(osPath != ''):
                os.makedirs(osPath,exist_ok=True)
        readWriteALNK(open(path,'rb'),open(dirPath,'wb'))

def getFromFile(path):
    filePath = path.replace('\\','/').split(r"nativePC/")[1]
    dirPath = outputPath + "/nativePC/" + filePath
    osPath = os.path.split(dirPath)[0]
    if(osPath != ''):
        os.makedirs(osPath,exist_ok=True)
    readWriteALNK(open(path,'rb'),open(dirPath,'wb'))


if __name__ == "__main__":

    if len(sys.argv) > 1:
        for i, arg in enumerate(sys.argv):
            if i > 0:
                getFromFile(arg)
    else:
        run = input("Run on chunk files? (Y/N):")
        if run.upper() == 'Y':
            getAllFromChunk()