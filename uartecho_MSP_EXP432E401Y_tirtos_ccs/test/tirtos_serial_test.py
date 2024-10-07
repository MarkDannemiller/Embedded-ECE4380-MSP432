import serial
import time
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description='Test device commands via serial port.')
parser.add_argument('--port',           default="COM11",   help='Serial port name (e.g., COM3, COM11, or /dev/ttyUSB0)')
parser.add_argument('--baud', type=int, default=115200,    help='Baud rate (default: 115200)')
args = parser.parse_args()

SERIAL_PORT = args.port
BAUD_RATE = args.baud
TIMEOUT = 1

# Define test cases as a list of dictionaries
# expected = test result (or string contained in result)
test_cases = [
    # -about command test cases
    {
        'command': '-about',
        'description': 'Normal usage of -about command',
        'expected': 'Student Name',  # Adjust expected substring
    },
    {
        'command': '-about extra_arg',
        'description': 'Extra arguments with -about command',
        'expected': 'Student Name',  # Or an error message
    },
    {
        'command': 'about',
        'description': 'Missing hyphen in -about command',
        'expected': 'Error: Unknown command',  # Expected error message
    },
    # -callback command test cases
    {
        'command': '-callback 1 2 -gpio 3 t',
        'description': 'Set up callback on SW1 to toggle GPIO 3 twice',
        'expected': 'Callback 1 set with count 2 and payload: -gpio 3 t',
    },
    {
        'command': '-callback 5 1 -print Invalid Index',
        'description': 'Invalid callback index',
        'expected': 'Error: Invalid callback index',
    },
    {
        'command': '-callback',
        'description': 'Missing index parameter',
        'expected': 'Error: Invalid callback index',
    },
    {
        'command': '-callback 1',
        'description': 'Missing count and payload',
        'expected': 'Error: Missing count parameter',
    },
    {
        'command': '-callback 1 2',
        'description': 'Missing payload',
        'expected': 'Error: Missing payload',
    },
    # -error command test cases
    {
        'command': '-error',
        'description': 'Display error counts',
        'expected': 'Error Count:',
    },
    {
        'command': '-error extra_arg',
        'description': 'Extra arguments with -error command',
        'expected': 'Error Count:',  # Or an error message
    },
    # -gpio command test cases
    {
        'command': '-gpio 3 w 1',
        'description': 'Write HIGH to GPIO pin 3',
        'expected': 'Wrote => Pin: 3  State: 1',
    },
    {
        'command': '-gpio 3 w 0',
        'description': 'Write LOW to GPIO pin 3',
        'expected': 'Wrote => Pin: 3  State: 0',
    },
    {
        'command': '-gpio 6 r',
        'description': 'Read GPIO pin 6',
        'expected': 'Read => Pin: 6  State:',
    },
    {
        'command': '-gpio 2 t',
        'description': 'Toggle GPIO pin 2',
        'expected': 'Toggled => Pin: 2 State:',
    },
    {
        'command': '-gpio -1 w 1',
        'description': 'Invalid GPIO pin number (negative)',
        'expected': 'Error: GPIO out of range',
    },
    {
        'command': '-gpio 8 w 1',
        'description': 'Invalid GPIO pin number (out of range)',
        'expected': 'Error: GPIO out of range',
    },
    {
        'command': '-gpio 3 x 1',
        'description': 'Invalid GPIO function',
        'expected': 'Read => Pin: 3  State:',  # Defaults to read operation
    },
    {
        'command': '-gpio 3 w',
        'description': 'Missing value for write function',
        'expected': 'Wrote => Pin: 3  State: 0',
    },
    # -help command test cases
    {
        'command': '-help',
        'description': 'Display general help',
        'expected': 'Supported Command',
    },
    {
        'command': '-help gpio',
        'description': 'Display help for -gpio command',
        'expected': 'Command: -gpio [pin][function][val]',
    },
    {
        'command': '-help unknown_command',
        'description': 'Help for an unknown command',
        'expected': 'Supported Command [arg1][arg2]...',  # Just respond with default help
    },
    # -memr command test cases
    {
        'command': '-memr 0x1000',
        'description': 'Read memory at address 0x1000',
        'expected': 'MEMR',
    },
    {
        'command': '-memr 0xFFFFFFFF',
        'description': 'Memory address out of range',
        'expected': 'Error: Address out of range',
    },
    {
        'command': '-memr',
        'description': 'Missing address parameter',
        'expected': 'MEMR',  # According to code, defaults to address 0
    },
    # -print command test cases
    {
        'command': '-print Hello World',
        'description': 'Print a simple message',
        'expected': 'Hello World',
    },
    {
        'command': '-print',
        'description': 'Missing message to print',
        'expected': '',  # No output or error, depending on implementation
    },
    # -timer command test cases
    {
        'command': '-timer 100000',
        'description': 'Set Timer0 period to 100,000 us',
        'expected': 'Set Timer0 period to 100000 us',
    },
    {
        'command': '-timer 50',
        'description': 'Set Timer0 period below minimum (invalid)',
        'expected': 'Error: Timer period out of range',
    },
    {
        'command': '-timer',
        'description': 'No period value provided',
        'expected': 'Warning: No period value detected',
    },
    # Invalid command test case
    {
        'command': '-unknown',
        'description': 'Unknown command',
        'expected': 'Error: Unknown command',
    },
    # Edge cases for buffer overflow
    {
        'command': '-print ' + 'A' * 1024,
        'description': 'Print message exceeding buffer size',
        'expected': 'Error: Buffer Overflow',
    },
    {
        'command': '-callback 1 2 ' + 'A' * 1024,
        'description': 'Callback payload exceeding buffer size',
        'expected': 'Error: Buffer Overflow',
    },
    # Case sensitivity test case
    {
        'command': '-PRINT Hello',
        'description': 'Command in uppercase',
        'expected': 'Hello',  # Commands not case-sensitive
    },
    # Command with extra whitespace
    {
        'command': '-gpio   3   w   1',
        'description': 'Command with extra spaces',
        'expected': 'Wrote => Pin: 3  State: 1',
    },
    # Command injection attempt
    {
        'command': '-print Test; -gpio 3 t',
        'description': 'Attempt to inject multiple commands',
        'expected': 'Test; -gpio 3 t',  # Should treat entire input as message to print
    },
    # Commands without hyphen
    {
        'command': 'gpio 3 w 1',
        'description': 'Missing hyphen in command',
        'expected': 'Error: Unknown command',
    },
    # Non-integer inputs
    {
        'command': '-gpio three w 1',
        'description': 'Non-integer pin number',
        'expected': 'Wrote => Pin: 0  State: 1',  # Default to 0 if not parsable
    },
    {
        'command': '-timer abc',
        'description': 'Non-integer timer period',
        'expected': 'Error:',  # May produce an error due to invalid input
    },
    # Command with negative values
    {
        'command': '-timer -5000',
        'description': 'Negative timer period',
        'expected': 'Error: Timer period out of range',
    },
    # Reading uninitialized memory
    {
        'command': '-memr 0x20000000',
        'description': 'Read from start of SRAM',
        'expected': 'MEMR',
    },
    # Help command with extra arguments
    {
        'command': '-help gpio extra_arg',
        'description': 'Help command with extra argument',
        'expected': 'Command: -gpio [pin][function][val]',
    },
    # Empty input
    {
        'command': '',
        'description': 'Empty input',
        'expected': '',  # Depending on implementation, may produce no output
    },
    # Whitespace input
    {
        'command': '   ',
        'description': 'Input with only spaces',
        'expected': '',  # Depending on implementation, may produce no output
    },
    # Callback with zero count
    {
        'command': '-callback 1 0 -print Should Not Execute',
        'description': 'Callback with zero count (should not execute)',
        'expected': 'Callback 1 set with count 0 and payload: -print Should Not Execute',
    },
    # Read GPIO with extra value
    {
        'command': '-gpio 3 r 1',
        'description': 'Read GPIO with extra value',
        'expected': 'Read => Pin: 3  State:',
    },
    # Toggle GPIO with extra value
    {
        'command': '-gpio 3 t 1',
        'description': 'Toggle GPIO with extra value',
        'expected': 'Toggled => Pin: 3 State:',
    },
    # Long payload in callback
    {
        'command': '-callback 1 1 ' + '-print ' + 'A' * 500,
        'description': 'Callback with long payload',
        'expected': 'Error: Buffer Overflow',  # Payload may be truncated
    },
]


def send_command(ser, command):
    """Send a command to the device and read the response."""
    ser.write((command + '\n').encode())  # Send command
    time.sleep(0.1)  # Wait for the device to process
    response = ''
    start_time = time.time()
    while True:
        line = ser.readline().decode('utf-8', errors='ignore')
        if not line:
            if time.time() - start_time > TIMEOUT:
                break
            continue
        response += line
    return response.strip()

def run_test_cases():
    """Run all test cases and report the results."""
    # Open serial port
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
    except serial.SerialException as e:
        print(f"Error opening serial port {SERIAL_PORT}: {e}")
        return

    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")

    passed = 0
    failed = 0

    for idx, test in enumerate(test_cases, start=1):
        print(f"\nTest Case {idx}: {test['description']}")
        print(f"Command: {test['command']}")

        response = send_command(ser, test['command'])
        print("Response:")
        print(response)

        # Determine if the test passed or failed
        if test['expected'] in response:
            print("Result: PASS")
            passed += 1
        else:
            print("Result: FAIL")
            failed += 1

    print("\nTest Summary:")
    print(f"Total Tests: {len(test_cases)}")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")

    ser.close()

if __name__ == '__main__':
    run_test_cases()
