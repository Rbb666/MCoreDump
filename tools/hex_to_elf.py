import binascii
from datetime import datetime

def hex_to_elf(hex_string, output_file):
    # Convert hex string to binary
    try:
        binary_data = binascii.unhexlify(hex_string.replace(" ", ""))
    except binascii.Error:
        raise ValueError("Invalid hex string")

    # Write binary data to ELF file
    with open(output_file, 'wb') as f:
        f.write(binary_data)

def main():
    # Prompt for hex string
    print("Enter the hex string containing complete ELF data (spaces allowed):")
    hex_string = input().strip()
    
    # Prompt for output file name
    print("Enter the output file name (leave blank for default format corefile_timestamp.elf):")
    output_file = input().strip()
    
    # Generate default filename with timestamp if not provided
    if not output_file:
        timestamp = datetime.now().strftime("%y.%m.%d.%H.%M")
        output_file = f"corefile_{timestamp}.elf"
    
    try:
        hex_to_elf(hex_string, output_file)
        print(f"ELF file generated: {output_file}")
    except Exception as e:
        print(f"Error: {str(e)}")

if __name__ == "__main__":
    main()