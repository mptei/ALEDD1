# Reads the XML and generates header file with C definitions
import sys
import xml.etree.ElementTree as ET

def determineSizeInBit(parmType, ns):
    node = parmType.find('knx:TypeNumber',ns)
    if node is not None:
        return node.get('SizeInBit')

    node = parmType.find('knx:TypeRestriction',ns)
    if node is not None:
        return node.get('SizeInBit')

    raise ValueError(f'Unsupport parameter type: {parmType[0]}')

def generate_knx(file_name, out=sys.stdout):
    root = ET.parse(file_name).getroot()
    ns = { 'knx': "http://knx.org/xml/project/20" }

    parmTypes = {parmType.get('Id'): determineSizeInBit(parmType, ns) for parmType in root.findall('knx:ManufacturerData/knx:Manufacturer/knx:ApplicationPrograms/knx:ApplicationProgram/knx:Static/knx:ParameterTypes/knx:ParameterType',ns)}

    functionMap = {
        '8': 'paramByte',
        '32': 'paramInt'
    }

    print (f'#pragma once', file=out)
    print(file=out)

    print(f'// Parameters', file=out)
    for parm in root.findall('knx:ManufacturerData/knx:Manufacturer/knx:ApplicationPrograms/knx:ApplicationProgram/knx:Static/knx:Parameters/knx:Parameter',ns):
        sizeInBits = parmTypes.get(parm.get('ParameterType'))
        funcName = functionMap.get(sizeInBits)
        if not funcName:
            raise KeyError(f'Unsupported size: {sizeInBits}')
        print(f'#define PARM_{parm.get("Name")} {parm.find("knx:Memory",ns).get("Offset")}', file=out)
        print(f'#define PARMVAL_{parm.get("Name")}() knx.{funcName}(PARM_{parm.get("Name")})', file=out)
    print (file=out)

    print(f'// Group objects', file=out)
    for com_object in root.findall('knx:ManufacturerData/knx:Manufacturer/knx:ApplicationPrograms/knx:ApplicationProgram/knx:Static/knx:ComObjectTable/knx:ComObject',ns):
        name=f'{com_object.get("Name")}_{com_object.get("FunctionText")}'.replace(" ","_").replace("/","_")
        print(f'#define NUM_{name} {com_object.get("Number")}', file=out)
        print(f'#define go_{name} knx.getGroupObject(NUM_{name})', file=out)

def generate_knx_to_file(file_name, target_name):
    with open(target_name, 'w') as f:
        generate_knx(file_name, f)

if __name__ == "__main__":

    # Check python version
    if sys.version_info.major < 3 or sys.version_info.minor < 9:
        print ("Need at least python3.9")
        exit(-1)
    
    if 1 == len(sys.argv):
        raise Exception("Missing file name")

    generate_knx(sys.argv[1])