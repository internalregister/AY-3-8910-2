# YM converter to SNG file format for the AY-3-8910 programmable sound generator and compatible chips
# version 2.0

# SNG file format:
# 5 bytes: 'S', 'N', 'G', 1, 0
# Followed by sequence of 2 bytes: register, value

# when register < 16, it's AY-3-8910 register for the first PSG
# when (register & 0x7F) < 16, it's AY-3-8910 register for the second PSG ((register & 0x7F) is the number of the register to use)
# when register = 16, the following value hold how many frames to skip
# when register = 255, means end of file

# Sérgio Vieira 2022

import sys
import struct

def readAllFrames(inputFile, frameCount):
    totalBytes = 16 * frameCount

    tempArray = inputFile.read(totalBytes)
    finalArray = [0] * totalBytes

    for currentRegister in range(16):
        offset = frameCount * currentRegister
        for i in range(frameCount):
            finalArray[16 * i + currentRegister] = tempArray[i + offset]
    
    return finalArray

def readYMFile(inputFilename):
    try:

        frameCount = 0
        frames = []

        print()
        print(f'Reading file "{inputFilename}"...')

        with open(inputFilename, "rb") as inputFile:
            # Read Header
            header = inputFile.read(4)

            if not header in ["YM3!".encode("ascii"), "YM4!".encode("ascii"), "YM5!".encode("ascii"), "YM6!".encode("ascii")]:
                print("ERROR: Invalid header in input file")
                print("Make sure it's an uncompressed YM file, if you think this is a valid YM file, try to uncompress it and use the uncompressed file")
                sys.exit(1)
            
            print("\tHeader:", header.decode("ascii"))

            checkString = inputFile.read(8)
            
            if checkString != "LeOnArD!".encode("ascii"):
                print("ERROR: Check string doesn't check out in input file")
                sys.exit(1)

            frameCount = struct.unpack(">i", inputFile.read(4))[0]
            print("\tFrame count:", frameCount)

            temp = struct.unpack(">i", inputFile.read(4))[0]
            print("\tSong attributes:", temp)

            if (temp & 1) != 1:
                print("Error: Only interlaced YM files are supported.")
                sys.exit(1)

            temp = struct.unpack(">h", inputFile.read(2))[0]
            print("\tDigidrums samples:", temp)

            temp = struct.unpack(">i", inputFile.read(4))[0]
            print("\tYM frequency:", temp, "Hz")

            frameRate = struct.unpack(">h", inputFile.read(2))[0]
            print("\tFrame rate:", frameRate, "Hz")

            loopFrame = struct.unpack(">i", inputFile.read(4))[0]
            print("\tLoop frame number:", loopFrame)

            # Read unused bytes
            inputFile.read(2)

            title = ''.join(iter(lambda: inputFile.read(1).decode('ascii'), '\x00'))
            print("\tTitle:", title);
            artist = ''.join(iter(lambda: inputFile.read(1).decode('ascii'), '\x00'))
            print("\tArtist:", artist);
            comments = ''.join(iter(lambda: inputFile.read(1).decode('ascii'), '\x00'))
            print("\tComments:", comments);

            # Return frames
            return readAllFrames(inputFile, frameCount)

    except FileNotFoundError as fnfe:
        print("ERROR: Can't open input file:", fnfe)
    except BaseException as be:
        print("ERROR: Error reading input file:", be)
        sys.exit(1)

def writeOutputFile(outputFilename, frames, textOutput):
    print()
    print(f"Writing to file {outputFilename}...")
    try:
        fileMode = "wb"
        if textOutput:
            fileMode = "w"
        with open(outputFilename, fileMode) as outputFile:
            # Write header
            if textOutput:
                outputFile.write("SNG\n")
                outputFile.write("1, 0\n")
            else:
                outputFile.write("SNG".encode("ascii"))
                outputFile.write(bytes([1, 0]))

            registerBytes = [0] * 32
            lastRegisterBytes = [0] * 32

            totalSize = 0
            lastChange = 0

            framesCount = [len(frames[0]) // 16, len(frames[1]) // 16]

            for i in range(max(framesCount[0], framesCount[1])):

                anyChange = False
                for reg in list(range(14))+list(range(16, 30)):
                    if reg < 16:
                        if i >= framesCount[0]:
                            continue
                        else:
                            registerBytes[reg] = frames[0][i * 16 + reg]
                    else:
                        if i >= framesCount[1]:
                            continue
                        else:
                            registerBytes[reg] = frames[1][i * 16 + (reg - 16)]

                    # Always write values of register 13 because whenever it's written to
                    # it resets the envelope wave
                    if i == 0 or lastRegisterBytes[reg] != registerBytes[reg] or reg == 13 or reg == 16+13:
                        if not anyChange and i != 0:
                            # Write wait instruction
                            if textOutput:
                                outputFile.write(f"16, {lastChange}\n")
                            else:
                                outputFile.write(bytes([16, lastChange]))
                            totalSize = totalSize + 2
                            lastChange = 0
                        
                        # The value 255 for register 13 in a YM file is to be ignored
                        if (reg != 13 and reg != 16+13) or registerBytes[reg] != 255:
                            # Write register value
                            outputReg = reg
                            if reg >= 16:
                                outputReg = (reg - 16) | 0x80
                            if textOutput:
                                outputFile.write(f"{outputReg}, {registerBytes[reg]}\n")
                            else:
                                outputFile.write(bytes([outputReg, registerBytes[reg]]))
                            lastRegisterBytes[reg] = registerBytes[reg]
                            totalSize = totalSize + 2
                            anyChange = True
                
                if not anyChange:
                    lastChange += 1
            
            print(f"Output binary size is {totalSize} bytes.")            

            # Write end of file
            if textOutput:
                outputFile.write("255, 255")
            else:
                outputFile.write(bytes([255, 255]))

            print()
            print(f"Process complete.")


    except BaseException as be:
        print("ERROR: Error writing to output file", be)
        sys.exit(1)

def printUsage():
    print()
    print("Usage for one PSG: python3 ymConverter2.py ymFile [-o outputFile] [-t]")
    print("Usage for two PSGs: python3 ymConverter2.py ymFile ymFile2 [-o outputFile] [-t]")
    print()
    print('  -o outputFile    Define output file name (default is "output.sng" or "output.txt" if in text output mode)')
    print("  -t               Text output mode")
    print()
    print("Note: the input files must be an uncompressed YM file in interlaced mode")

if __name__ == "__main__":    
    print("YM Converter to SNG file format for AY-3-8910 PSGs 2.0")

    inputFilenames = []
    outputFilename = ""
    textOutput = False

    if len(sys.argv) < 2:
        printUsage()
        sys.exit(1)

    skipNext = False
    for i in range(1, len(sys.argv)):
        if skipNext:
            skipNext = False
            continue
        if sys.argv[i] == '-o':
            outputFilename = argv[i+1]
            skipNext = True
        elif sys.argv[i] == '-t':
            textOutput = True
        else:
            if len(inputFilenames) == 2:
                print("ERROR: Only up to two input files.")
                sys.exit(1)
            inputFilenames.append(sys.argv[i])
        
    if outputFilename == "":
        if textOutput:
            outputFilename = "output.txt"
        else:
            outputFilename = "output.sng"

    frames = [[], []]

    for i in range(len(inputFilenames)):
        frames[i] = readYMFile(inputFilenames[i])

    if len(inputFilenames) == 2 and len(frames[0]) != len(frames[1]):
        print("WARNING: YM files don't have the same number of frames")

    writeOutputFile(outputFilename, frames, textOutput)

    

