import os
import sys
import binascii

def generate_header_from_cubin(cubin_dir, header_file):
    # Open the header file for writing
    with open(header_file, 'w') as header:
        # Write header guards
        header.write('#pragma once\n\n#include <cstdint>\n\n')

        # Iterate over all .cubin files in the directory
        for file_name in os.listdir(cubin_dir):
            if file_name.endswith('.cubin'):
                # Determine the array name from the file name
                array_name = file_name.replace('.cubin', '').replace('-', '_').replace(' ', '_').upper()
                cubin_path = os.path.join(cubin_dir, file_name)

                # Read the contents of the .cubin file
                with open(cubin_path, 'rb') as cubin_file:
                    cubin_data = cubin_file.read()

                # Convert binary data to hex and format as uint8_t array
                hex_data = binascii.hexlify(cubin_data).decode('ascii')
                hex_data_lines = [hex_data[i:i+64] for i in range(0, len(hex_data), 64)]  # Split into 64-char lines

                # Write array declaration to header file
                header.write(f'const uint8_t {array_name}[] = {{\n')

                for line in hex_data_lines:
                    header.write('    ' + ', '.join(f'0x{line[i:i+2]}' for i in range(0, len(line), 2)) + ',\n')

                header.write('};\n\n')

if __name__ == '__main__':
    generate_header_from_cubin("output", "memcopy_kernels.h")
