@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM Set paths and files
set SOURCE_FILE=hello_world.c
set PROGRAM=hello_world.exe
set INPUT_FILE=test_case.txt
set EXPECT_FILE=expect.txt
set OUTPUT_FILE=dump.txt
set TEMP_INPUT=temp_input.txt

REM Counters
set /a test_count=0
set /a passed=0
set /a failed=0

echo Checking if compilation is needed...
echo.

REM Check if source file exists
if not exist %SOURCE_FILE% (
    echo Error: Source file %SOURCE_FILE% not found
    echo Current directory: %CD%
    dir *.c
    exit /b 1
)

REM Check if recompilation is needed
if not exist %PROGRAM% (
    echo Executable not found, starting compilation...
    goto compile
) else (
    REM Check if source file is newer than executable
    for %%I in (%SOURCE_FILE%) do set source_time=%%~tI
    for %%I in (%PROGRAM%) do set program_time=%%~tI
    
    if "!source_time!" gtr "!program_time!" (
        echo Source file updated, recompiling...
        goto compile
    ) else (
        echo Using existing executable.
        goto run_tests
    )
)

:compile
REM Try to compile C source file
echo Compiling %SOURCE_FILE%...
gcc -o %PROGRAM% %SOURCE_FILE% 2> compile_errors.txt

REM Check if compilation succeeded
if errorlevel 1 (
    echo Compilation failed!
    type compile_errors.txt
    del compile_errors.txt 2>nul
    exit /b 1
) else (
    echo Compilation successful!
    del compile_errors.txt 2>nul
)

:run_tests
REM Check if files exist
if not exist %PROGRAM% (
    echo Error: Program %PROGRAM% not found
    exit /b 1
)

if not exist %INPUT_FILE% (
    echo Error: Test input file %INPUT_FILE% not found
    exit /b 1
)

if not exist %EXPECT_FILE% (
    echo Error: Expected output file %EXPECT_FILE% not found
    exit /b 1
)

echo.
echo Starting tests...
echo.

REM Read test inputs and expected outputs into arrays
set /a i=0
for /f "usebackq delims=" %%a in ("%INPUT_FILE%") do (
    set /a i+=1
    set "input[!i!]=%%a"
)

set /a input_count=%i%

set /a i=0
for /f "usebackq delims=" %%a in ("%EXPECT_FILE%") do (
    set /a i+=1
    set "expect[!i!]=%%a"
)

set /a expect_count=%i%

REM Check if number of test cases match
if not %input_count% equ %expect_count% (
    echo Error: Number of test inputs and expected outputs do not match
    echo Test input count: %input_count%
    echo Expected output count: %expect_count%
    exit /b 1
)

REM Run each test case
for /l %%i in (1,1,%input_count%) do (
    set /a test_count+=1
    
    echo Testing sample %%i...
    
    REM Create temporary input file
    echo !input[%%i]! > %TEMP_INPUT%
    
    REM Run program and capture output
    < %TEMP_INPUT% %PROGRAM% > %OUTPUT_FILE%
    
    REM Read program output
    set /p output=< %OUTPUT_FILE%
    
    REM Compare output with expected result
    if "!output!" == "!expect[%%i]!" (
        echo  Sample %%i: PASSED
        set /a passed+=1
    ) else (
        echo  Sample %%i: FAILED
        set /a failed+=1
        echo   Input: !input[%%i]!
        echo   Expected: !expect[%%i]!
        echo   Actual: !output!
        echo.
    )
)

REM Clean up temporary files
if exist %TEMP_INPUT% del %TEMP_INPUT%
if exist %OUTPUT_FILE% del %OUTPUT_FILE%

REM Output test summary
echo.
echo ====== TEST SUMMARY ======
echo Total test cases: %test_count%
echo Passed: %passed%
echo Failed: %failed%

if %failed% equ 0 (
    echo All tests passed!
    exit /b 0
) else (
    echo Some tests failed!
    exit /b 1
)