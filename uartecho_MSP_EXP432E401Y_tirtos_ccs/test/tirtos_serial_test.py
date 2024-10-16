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
        'description': 'Display active callbacks',
        'expected': 'Callback Configurations:',
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
        'description': 'Display current timer period',
        'expected': 'Current Timer0 period is',
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
        
        
        
        
    # -reg command test cases
    {
        'command': '-reg',
        'description': 'Display all registers when no arguments are provided',
        'expected': 'Register Values:',
    },
    {
        'command': '-reg mov r0 #10',
        'description': 'Move immediate value 10 into register R0',
        'expected': 'R0 = 10',
    },
    {
        'command': '-reg inc r0',
        'description': 'Increment register R0',
        'expected': 'R0 = 11',  # Assuming R0 was 10 from previous test
    },
    {
        'command': '-reg add r0 r1',
        'description': 'Add R1 to R0',
        'expected': 'R0 =',  # Depends on R1's value
    },
    {
        'command': '-reg mov r1 #5',
        'description': 'Move immediate value 5 into register R1',
        'expected': 'R1 = 5',
    },
    {
        'command': '-reg add r0 r1',
        'description': 'Add R1 to R0 (R0 += R1)',
        'expected': 'R0 = 16',  # Assuming R0 was 11 and R1 is 5
    },
    {
        'command': '-reg sub r0 #1',
        'description': 'Subtract immediate value 1 from R0',
        'expected': 'R0 = 15',
    },
    {
        'command': '-reg mul r0 #2',
        'description': 'Multiply R0 by 2',
        'expected': 'R0 = 30',
    },
    {
        'command': '-reg div r0 #3',
        'description': 'Divide R0 by 3',
        'expected': 'R0 = 10',
    },
    {
        'command': '-reg rem r0 #3',
        'description': 'Remainder of R0 divided by 3',
        'expected': 'R0 = 1',
    },
    {
        'command': '-reg neg r1',
        'description': 'Negate register R1',
        'expected': 'R1 = -5',
    },
    {
        'command': '-reg not r1',
        'description': 'Bitwise NOT of register R1',
        'expected': 'R1 =',  # Expected value depends on two's complement representation
    },
    {
        'command': '-reg and r0 #0x0F',
        'description': 'Bitwise AND R0 with immediate value 0x0F',
        'expected': 'R0 =',  # Expected value depends on R0's value
    },
    {
        'command': '-reg or r0 r1',
        'description': 'Bitwise OR R0 with R1',
        'expected': 'R0 =',  # Expected value depends on R0 and R1
    },
    {
        'command': '-reg xor r0 #0xFF',
        'description': 'Bitwise XOR R0 with immediate value 0xFF',
        'expected': 'R0 =',  # Expected value depends on R0's value
    },
    {
        'command': '-reg max r0 #20',
        'description': 'Set R0 to max of R0 and 20',
        'expected': 'R0 =',  # Expected value depends on R0's value
    },
    {
        'command': '-reg min r0 #5',
        'description': 'Set R0 to min of R0 and 5',
        'expected': 'R0 = 5',  # If R0 was greater than 5
    },
    {
        'command': '-reg xchg r0 r1',
        'description': 'Exchange values of R0 and R1',
        'expected': 'R0 =',  # Values of R0 and R1 swapped
    },
    {
        'command': '-reg mov r31 #123456',
        'description': 'Move a large immediate value into R31',
        'expected': 'R31 = 123456',
    },
    {
        'command': '-reg mov r32 #1',
        'description': 'Invalid register index (R32 does not exist)',
        'expected': 'Error: Invalid destination register.',
    },
    {
        'command': '-reg mov r0 #xFF',
        'description': 'Move hexadecimal immediate value into R0',
        'expected': 'R0 = 255',
    },
    {
        'command': '-reg mov r0 @0x20000000',
        'description': 'Move value from memory address into R0 (if permitted)',
        'expected': 'R0 =',  # Value at address 0x20000000
    },
    {
        'command': '-reg div r0 #0',
        'description': 'Division by zero error',
        'expected': 'Error: Division by zero.',
    },
    {
        'command': '-reg rem r0 #0',
        'description': 'Remainder by zero error',
        'expected': 'Error: Division by zero.',
    },
    {
        'command': '-reg unknown r0 r1',
        'description': 'Unknown operation',
        'expected': 'Error: Unknown operation.',
    },
    {
        'command': '-reg inc',
        'description': 'Missing operand for inc operation',
        'expected': 'Error: Missing operand.',
    },
    {
        'command': '-reg mov r0',
        'description': 'Missing source operand for mov operation',
        'expected': 'Error: Missing operands.',
    },
    {
        'command': '-reg mov r0 invalid',
        'description': 'Invalid source operand',
        'expected': 'Error: Invalid source operand.',
    },
    {
        'command': '-reg mov invalid r0',
        'description': 'Invalid destination register',
        'expected': 'Error: Invalid destination register.',
    },
    {
        'command': '-reg inc r33',
        'description': 'Increment non-existent register',
        'expected': 'Error: Invalid register.',
    },
    {
        'command': '-reg mov r0 #xZZ',
        'description': 'Invalid hexadecimal immediate value',
        'expected': 'Error: Invalid source operand.',
    },
    {
        'command': '-reg mov r0 #999999999999',
        'description': 'Immediate value out of range',
        'expected': 'R0 =',  # May cause overflow or error
    },
    {
        'command': '-reg mov r0 @0xFFFFFFFF',
        'description': 'Invalid memory address',
        'expected': 'Error: Invalid memory address.',
    },
    {
        'command': '-reg mov r0 #',
        'description': 'Incomplete immediate value',
        'expected': 'Error: Invalid source operand.',
    },
    {
        'command': '-reg xchg r0 r0',
        'description': 'Exchange register with itself',
        'expected': 'R0 =',  # Value should remain the same
    },
    {
        'command': '-reg neg r0',
        'description': 'Negate R0',
        'expected': 'R0 =',  # Negative of current R0
    },
    {
        'command': '-reg not r0',
        'description': 'Bitwise NOT of R0',
        'expected': 'R0 =',  # Bitwise complement of R0
    },
    {
        'command': '-reg add r0 #2147483647',
        'description': 'Add large positive number to R0',
        'expected': 'R0 =',  # May cause integer overflow
    },
    {
        'command': '-reg sub r0 #-2147483648',
        'description': 'Subtract large negative number from R0',
        'expected': 'R0 =',  # May cause integer underflow
    },
    {
        'command': '-reg mov r0 #abc',
        'description': 'Invalid immediate value (non-numeric)',
        'expected': 'Error: Invalid source operand.',
    },
    {
        'command': '-reg mov r0 #x',
        'description': 'Invalid hexadecimal immediate value',
        'expected': 'Error: Invalid source operand.',
    },
    {
        'command': '-reg mul r0 #0',
        'description': 'Multiply R0 by zero',
        'expected': 'R0 = 0',
    },
    {
        'command': '-reg div r0 #1',
        'description': 'Divide R0 by one',
        'expected': 'R0 =',  # Should remain the same
    },
    {
        'command': '-reg mov r0 @invalid',
        'description': 'Invalid memory address format',
        'expected': 'Error: Invalid source operand.',
    },
    {
        'command': '-reg mov r0 @0x20000000',
        'description': 'Read from valid memory address',
        'expected': 'R0 =',  # Value at address 0x20000000
    },
    '''{ TODO - Determine if unaligned memory access is supported
        'command': '-reg mov r0 @0x20000001',
        'description': 'Read from unaligned memory address',
        'expected': 'Error: Unaligned memory access.',
    },'''
    {
        'command': '-reg',
        'description': 'Display all registers after operations',
        'expected': 'Register Values:',
    },




    # -ticker command test cases
    {
        'command': '-ticker',
        'description': 'Display all tickers when no arguments are provided',
        'expected': 'Ticker Configurations:',
    },
    {
        'command': '-ticker 0 100 200 5 -gpio 2 t',
        'description': 'Set ticker 0 with delay 100, period 200, count 5, payload to toggle GPIO 2',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -gpio 2 t',
    },
    {
        'command': '-ticker',
        'description': 'Display tickers after setting ticker 0',
        'expected': 'Ticker 0:',
    },
    {
        'command': '-ticker 1 50 100 -1 -print Hello',
        'description': 'Set ticker 1 with infinite count and payload to print "Hello"',
        'expected': 'Ticker 1 set with delay 50, period 100, count -1, payload: -print Hello',
    },
    {
        'command': '-ticker 0',
        'description': 'Missing parameters for ticker 0 (should raise error)',
        'expected': 'Error: Missing initial delay parameter.',
    },
    {
        'command': '-ticker 16 100 200 5 -gpio 2 t',
        'description': 'Invalid ticker index (out of range)',
        'expected': 'Error: Invalid ticker index.',
    },
    {
        'command': '-ticker 0 -100 200 5 -gpio 2 t',
        'description': 'Negative initial delay (should be accepted or handled)',
        'expected': 'Ticker 0 set with delay -100, period 200, count 5, payload: -gpio 2 t',
    },
    {
        'command': '-ticker 0 100 -200 5 -gpio 2 t',
        'description': 'Negative period (should be accepted or handled)',
        'expected': 'Ticker 0 set with delay 100, period -200, count 5, payload: -gpio 2 t',
    },
    {
        'command': '-ticker 0 100 200 abc -gpio 2 t',
        'description': 'Non-integer count value',
        'expected': 'Error: Missing count parameter.',
    },
    {
        'command': '-ticker 0 100 200 5',
        'description': 'Missing payload (should raise error)',
        'expected': 'Error: Missing payload.',
    },
    {
        'command': '-ticker 0 abc 200 5 -gpio 2 t',
        'description': 'Non-integer initial delay',
        'expected': 'Error: Missing initial delay parameter.',
    },
    {
        'command': '-ticker 0 100 abc 5 -gpio 2 t',
        'description': 'Non-integer period',
        'expected': 'Error: Missing period parameter.',
    },
    {
        'command': '-ticker 0 100 200 5 ' + 'A' * 1024,
        'description': 'Payload exceeding buffer size',
        'expected': 'Error: Buffer Overflow',  # Or appropriate error message
    },
    {
        'command': '-ticker 0 0 0 0 -print Immediate',
        'description': 'Ticker with zero initial delay and period',
        'expected': 'Ticker 0 set with delay 0, period 0, count 0, payload: -print Immediate',
    },
    {
        'command': '-ticker 0 100 200 5 -gpio 2 t',
        'description': 'Resetting ticker 0 with new parameters',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -gpio 2 t',
    },
    {
        'command': '-ticker',
        'description': 'Display tickers after resetting ticker 0',
        'expected': 'Ticker 0:',
    },
    {
        'command': '-ticker -1 100 200 5 -gpio 2 t',
        'description': 'Invalid ticker index (negative)',
        'expected': 'Error: Invalid ticker index.',
    },
    {
        'command': '-ticker 0 100 200 5 -gpio 9 t',
        'description': 'Payload with invalid GPIO pin (should be accepted, error occurs during execution)',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -gpio 9 t',
    },
    {
        'command': '-ticker 0 100 200 5',
        'description': 'Missing payload with correct parameters',
        'expected': 'Error: Missing payload.',
    },
    {
        'command': '-ticker abc 100 200 5 -gpio 2 t',
        'description': 'Non-integer ticker index',
        'expected': 'Error: Invalid ticker index.',
    },
    {
        'command': '-ticker',
        'description': 'Display all tickers after several operations',
        'expected': 'Ticker Configurations:',
    },
    {
        'command': '-ticker 0 100 200 -1 -print TickerTest',
        'description': 'Set ticker 0 with infinite count and print payload',
        'expected': 'Ticker 0 set with delay 100, period 200, count -1, payload: -print TickerTest',
    },
    {
        'command': '-ticker 0 100 200 5 -print TickerTest',
        'description': 'Set ticker 0 with finite count and print payload',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -print TickerTest',
    },
    {
        'command': '-ticker',
        'description': 'Display tickers to confirm ticker 0 settings',
        'expected': 'Ticker 0:',
    },
    {
        'command': '-ticker 15 100 200 5 -print EdgeCase',
        'description': 'Set ticker at maximum valid index',
        'expected': 'Ticker 15 set with delay 100, period 200, count 5, payload: -print EdgeCase',
    },
    {
        'command': '-ticker',
        'description': 'Display all tickers to confirm ticker 15 settings',
        'expected': 'Ticker 15:',
    },
    # Testing the display of inactive tickers
    {
        'command': '-ticker',
        'description': 'Display all tickers including inactive ones',
        'expected': 'Ticker Configurations:',
    },
    # Command with extra whitespace
    {
        'command': '-ticker   0   100   200   5   -gpio   2   t',
        'description': 'Ticker command with extra spaces',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -gpio   2   t',
    },
    # Edge case with large values
    {
        'command': '-ticker 0 4294967295 4294967295 2147483647 -print LargeValues',
        'description': 'Set ticker with maximum unsigned and signed integer values',
        'expected': 'Ticker 0 set with delay 4294967295, period 4294967295, count 2147483647, payload: -print LargeValues',
    },
    # Negative count
    {
        'command': '-ticker 0 100 200 -5 -print NegativeCount',
        'description': 'Set ticker with negative count (other than -1)',
        'expected': 'Ticker 0 set with delay 100, period 200, count -5, payload: -print NegativeCount',
    },
    # Ticker with zero count
    {
        'command': '-ticker 0 100 200 0 -print ZeroCount',
        'description': 'Set ticker with zero count (should not execute)',
        'expected': 'Ticker 0 set with delay 100, period 200, count 0, payload: -print ZeroCount',
    },
    # Ticker with zero period
    {
        'command': '-ticker 0 100 0 5 -print ZeroPeriod',
        'description': 'Set ticker with zero period',
        'expected': 'Ticker 0 set with delay 100, period 0, count 5, payload: -print ZeroPeriod',
    },
    # Ticker with non-existent command in payload
    {
        'command': '-ticker 0 100 200 5 -nonexistentcommand',
        'description': 'Set ticker with invalid payload command',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -nonexistentcommand',
    },
    # Attempt to inject multiple commands
    {
        'command': '-ticker 0 100 200 5 -print FirstCommand; -gpio 2 t',
        'description': 'Payload containing multiple commands (should be treated as a single payload)',
        'expected': 'Ticker 0 set with delay 100, period 200, count 5, payload: -print FirstCommand; -gpio 2 t',
    },
    # Empty payload
    {
        'command': '-ticker 0 100 200 5 ',
        'description': 'Ticker command with empty payload',
        'expected': 'Error: Missing payload.',
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
