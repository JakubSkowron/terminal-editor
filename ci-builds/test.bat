@echo off

pushd %cd%

echo "Testing..."

cd .\tests
..\build-zz\tests\Debug\editor-tests.exe
if %errorlevel% neq 0 exit /b 1

echo "Testing done."

popd
