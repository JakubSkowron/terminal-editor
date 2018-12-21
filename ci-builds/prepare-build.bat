@echo off

@rem this script assumes, that:
@rem   - GENERATOR variable is set to proper CMake generator

echo "Running CMake with GENERATOR=%GENERATOR%"

pushd %cd%

mkdir build-zz
cd build-zz

"C:\Program Files\CMake\bin\cmake.exe" -G "%GENERATOR%" ..
if %errorlevel% neq 0 exit /b 1

popd
