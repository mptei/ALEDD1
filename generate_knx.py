# Script to create knx definitions

Import("env")
import subprocess, re
from generator import generate_knx_to_file

print(env)

generate_knx_to_file("ALEDD1.xml", "include/generated/knx_defines.h")
