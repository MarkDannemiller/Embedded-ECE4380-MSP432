## General TODO List
- [ ] Console Command history with arrow key traversing
- [ ] Make `-reg rem` by 0 return remainder of numerator
- [ ] Debug print statement system enabled via shadow registers
- [ ] Create `-shadow` to access shadow register contents (for system setup)

## P700-P800
- [x] Implement `-script` with 64 lines.
- [x] Implement script pointer for running script payload amidst other task payloads.
- [ ] Implement `-if` conditional mechanism to allow a script to apply conditional decisions based on register contents.
- [ ] Support immediate constants # and registers R.
- [ ] Implement emergency stop function to break program out of runaway timer, ticker, callback, or script.
   - Ensure graceful shutdown of tasks through a shutdown flag, and restart with buffers, payloads, timers, callbacks, and tickers cleared.
- [x] Implement application printout above user input line.
- [x] Update `-print` to display with a newline character always.


---

# Changelog

## P600

- [x] **Create `register.h` and `register.c`**: Implemented new source and header files to encapsulate register operations. This modularization improves code maintainability and readability.
- [x] **Modify `execute_payload()`**: Updated the command parser to recognize and handle the `-reg` command, enabling users to perform register operations via the command interface.
- [x] **Implement `CMD_reg()` Function**: Developed the `CMD_reg()` function to parse and execute register operations. The function supports various operations like `mov`, `add`, `sub`, `inc`, `dec`, and displays all registers when called without arguments.
- [x] **Add General Printout for `-reg` Command**: Enhanced the `-reg` command to display the current values of all registers when no additional arguments are provided. This aids in debugging and monitoring register states.
- [x] **Add General Printout for `-ticker` Command**: Modified `CMD_ticker()` to display all ticker configurations and their payloads when the `-ticker` command is called without arguments. This provides users with a comprehensive view of active tickers.
- [x] **Add General Printout for `-callback` Command**: Updated `CMD_callback()` to display all callback configurations and their payloads when the `-callback` command is called without arguments. This helps users verify callback setups.
- [x] **Add General Printout for `-timer` Command**: Adjusted `CMD_timer()` to display the current Timer0 settings when the `-timer` command is called without arguments. Users can now easily check the timer configuration.
- [x] **Update the `-help` Command**: Enhanced help messages to include detailed information about the new `-reg` command. Updated existing help descriptions for `-ticker`, `-callback`, and `-timer` to reflect the added functionalities, providing clearer guidance to users.
- [x] **Test the Implementation**: Conducted extensive testing of the new `-reg` command and its operations, ensuring correctness and robustness. Verified that all commands display appropriate information when called without arguments.
- [x] **Update Python Test Script**: Added new test cases for the `-reg` command to the Python test script, covering various register operations and error conditions. Expanded test cases for the `-ticker` command to include scenarios with and without arguments, enhancing automated testing coverage.

## P500
- [x] Create tickers.c and tickers.h: We'll create a new source file and header file for the ticker functionality.
- [x] Define Ticker Data Structures: Create a struct to represent each ticker, including its index, delay, period, count, and payload.
- [x] Initialize Tickers: Set up an array to hold the 16 tickers and initialize them.
- [x] Implement the Ticker Timer: Use a second hardware timer to manage ticker scheduling with 10 ms ticks.
- [x] Modify execute_payload(): Ensure that scheduled commands can be executed using the existing execute_payload() function.
- [x] Implement CMD_ticker() Function: Parse the -ticker command and configure the specified ticker.
- [x] Update the -help Command: Add help information for the -ticker command.
- [x] Integrate Tickers into Main Loop: Ensure that tickers are checked and updated periodically.
- [x] Test the Implementation: Verify that tickers work as expected with various commands.

## P400
- [x] Implement executing commands from queue
- [x] Test by adding example command in start of main() function
- [x] Implement uart read in task with command queuing
- [x] Implement command queueing by callbacks with payload