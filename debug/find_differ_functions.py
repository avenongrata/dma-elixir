"""
    This script will find difference between functions 
    that were called in MEM_TO_MEM (memcpy)
    and MEM_TO_DEV (cyclic)
"""
import sys

path_to_file = r"/home/nongrata/projects/own_drivers/dma/"       # set path to file
memcpy_file_name = 'memcpy'
cyclic_file_name = 'cyclic_rx_16' 

if path_to_file:
    memcpy_file_name = path_to_file + memcpy_file_name
    cyclic_file_name = path_to_file + cyclic_file_name
    
# Try to open memcpy file
try:
    memcpy_file = open(memcpy_file_name, 'r')
except IOError:
    print("{0} isn't exist".format(memcpy_file_name))
    sys.exit(0)
else:
    print("{0} file opened".format(memcpy_file_name))

# Try to open cyclic file
try:
    cyclic_file = open(cyclic_file_name, 'r')
except IOError:
    print("{0} isn't exist".format(cyclic_file_name))
    sys.exit(0)
else:
    print("{0} file opened".format(cyclic_file_name))

# read all functions in file with 'called' string inside 
memcpy_functions = {}
for line in memcpy_file:
    if "called" in line:
        memcpy_functions[line] = '0'
        
# read all functions in file with 'called' string inside 
cyclic_functions = {}
for line in cyclic_file:
    if "called" in line:
        cyclic_functions[line] = '0'
        
memcpy_diff = []    # this functions are not in cyclic file
cyclic_diff = []    # this functions are not in memcpy file

# find finctions that are not in cyclic file
for key in memcpy_functions:
    if key not in cyclic_functions:
        memcpy_diff.append(key)
        
# find finctions that are not in memcpy file
for key in cyclic_functions:
    if key not in memcpy_functions:
        cyclic_diff.append(key)

# make output
print("\nThis functions are not in {0} file:\n".format(cyclic_file_name))
for x in memcpy_diff:
    print(x, end='')

print("\nThis functions are not in {0} file:\n".format(memcpy_file_name))
for x in cyclic_diff:
    print(x, end='')
