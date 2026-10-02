# Progress Log  Time Travel Debugger

## 2nd OCT 2026

### Core Setup:
- Installed WSL and set up VS code with WSL.
- Installed `g++`, `gdb` and `git`. Verified by running a test c++ program.
- Created the Github repo and pushed the `server.cpp` file

### Stack Class:
- Implemented the `stack` class as a linked list.
- Tested it in `test_stack.cpp`
- Encountered and fixed a segmentation error in `pop()`, I was deleting the `top` instead of the old node(`temp`).

### Timeline Class:
- Implemented the `timeline` class as a doubly linked list.
- Tested it in the `test_timeline.cpp` by recording 3 snapshots and traversing the list backward and forward. Output was as expected.

### Pass(0x0):
- Implemented `readSourceLine()`, `firstWord()`, `secondWord()` and `validateProgram()`.
- Tested with 1 valid and 3 invalid text files. All results matched.
- Encountered and fixed two bugs: wrong whitespace checking conditions in `firstWord()` and `secondWord()`, and the boolean flag 
not being reset on `func_end`.
