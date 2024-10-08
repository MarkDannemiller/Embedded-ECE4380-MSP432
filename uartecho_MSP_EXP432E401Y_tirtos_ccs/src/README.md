## General TODO List
- [ ] Console Command history with arrow key traversing

## P500 TODO List
- [x] Create tickers.c and tickers.h: We'll create a new source file and header file for the ticker functionality.
- [x] Define Ticker Data Structures: Create a struct to represent each ticker, including its index, delay, period, count, and payload.
- [x] Initialize Tickers: Set up an array to hold the 16 tickers and initialize them.
- [x] Implement the Ticker Timer: Use a second hardware timer to manage ticker scheduling with 10 ms ticks.
- [x] Modify execute_payload(): Ensure that scheduled commands can be executed using the existing execute_payload() function.
- [x] Implement CMD_ticker() Function: Parse the -ticker command and configure the specified ticker.
- [x] Update the -help Command: Add help information for the -ticker command.
- [x] Integrate Tickers into Main Loop: Ensure that tickers are checked and updated periodically.
- [ ] Test the Implementation: Verify that tickers work as expected with various commands.
- [ ] Add general printout of -ticker (display all tickers and payload info)
- [ ] Add general printout of -callback (display all callbacks and payload info)
- [ ] Add general printout of -timer (display configured settings)


# Changelog

## P400
- [x] Implement executing commands from queue
- [x] Test by adding example command in start of main() function
- [x] Implement uart read in task with command queuing
- [x] Implement command queueing by callbacks with payload