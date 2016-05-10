VertexShaders = ["StaticDefault", "SkinnedDefault", "PointTextureEmitter", "StaticPainting"]
FragmentShaders = ["Default", "DiffuseDefault", "NormalDefault", "DiffuseNormalDefault", "PointTextureEmitter", "DiffuseNormalPainting"]
GeometryShaders = ["PointTextureEmitter"]
ComputeKernels = ["AutomataManagement"]

import sys
import struct

SourceDirectory = sys.argv[1]

def GenerateShaderHeader(outFilename, shaderFilename, shaderName):
    with open(outFilename, "w") as outFile:
        with open(SourceDirectory + "/" + shaderFilename, "r") as shaderFile:
            outFile.write("const static char ")
            outFile.write(shaderName)
            outFile.write("[] = {")
            byte = shaderFile.read(1)
            while byte != "":
                (b,) = struct.unpack('B', byte)
                outFile.write(str(b))
                outFile.write(", ")
                byte = shaderFile.read(1)
            outFile.write("0};\n")

def GenerateShaderHeaders(names, extension):
    for name in names:
        GenerateShaderHeader(name + "_" + extension + ".h", name + "." + extension, name + "_" + extension)

GenerateShaderHeaders(VertexShaders, "vs")
GenerateShaderHeaders(FragmentShaders, "fs")
GenerateShaderHeaders(GeometryShaders, "gs")
GenerateShaderHeaders(ComputeKernels, "cl")
