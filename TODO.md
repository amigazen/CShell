# CShell TODO - Code Quality and SAS/C Library Improvements

## New Features
- Implement modern csh/tcsh and bash style script execution
- Permanently remove deprecated (archive) files
- Update FileSystem recognition to cover latest versions of FastFileSystem and other bundled FileSystems
- Update file and path lengths based on known filesystem types
- Add 64bit volume / file size support to info command and other commands where relevant

## Memory Management Improvements

### Replace Basic Memory Allocation with SAS/C Enhanced Functions
- Replace `salloc()` custom allocator with SAS/C's enhanced memory functions where appropriate
- Consider using `calloc()` instead of `malloc()` + `memset()` patterns for zero-initialized memory
- Implement proper error checking for all memory allocations
- Add memory pool management using SAS/C library features for frequently allocated/freed structures

### Memory Leak Prevention
- Review all `salloc()` calls and ensure corresponding `free()` calls exist
- Implement reference counting for shared data structures (HIST nodes, variable nodes)
- Add memory debugging macros that can be enabled in debug builds
- Create memory allocation wrappers that track allocations and detect leaks

## Code Structure and Architecture

### Modularization Improvements
- Split large source files (comm1.c, comm3.c, execom.c) into one file per command
- Implement proper encapsulation for global variables
- Add consistent error handling patterns across all modules

### Function Parameter Validation
- Add NULL pointer checks to all public functions
- Implement parameter range validation for numeric inputs
- Add pre-condition and post-condition assertions in debug builds
- Create input validation helper functions

## Error Handling and Robustness

### Enhanced Error Reporting
- Expand the PERROR table with more detailed error descriptions
- Implement stack trace functionality for debugging
- Add error context information (file, line, function)
- Create error recovery mechanisms for non-fatal errors

### Resource Management
- Implement RAII-style resource management patterns (within C89 constraints)
- Add automatic cleanup for temporary files and locks
- Implement timeout mechanisms for long-running operations
- Add graceful shutdown procedures for all subsystems

## Performance Optimizations

### Memory Access Patterns
- Optimize data structure layouts for better cache performance
- Implement object pools for frequently allocated structures
- Add memory prefetching hints where beneficial
- Reduce memory fragmentation through better allocation strategies

## Platform and Compatibility

### AmigaOS Integration
- **Priority: Medium**
- Improve integration with AmigaOS 3.x features
- Localize 
- Add Amiga specific commands e.g. for commodities control
- AmigaOS 4 native build
