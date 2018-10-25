:: Copyright 2018, IBM.
::
:: This source code is licensed under the Apache License, Version 2.0 found in
:: the LICENSE.txt file in the root directory of this source tree.

@ECHO OFF
@SETLOCAL enabledelayedexpansion

pushd %~dp0

SET target=%~n1

IF "%target%"=="lint" GOTO :lint
IF "%target%"=="test" GOTO :test
IF "%target%"=="profile" GOTO :profile
IF "%target%"=="dist" GOTO :dist

:usage
ECHO.
ECHO.Usage:
ECHO.    .\make lint    Runs Pyhton source code analysis tool
ECHO.    .\make test    Runs tests
ECHO.    .\make profile Runs profiling tests
ECHO.
GOTO :end

:lint
pylint -rn qiskit\backends\jku test_jku_simulator
IF errorlevel 9009 GOTO :error
GOTO :next

:test
python -m unittest discover -s test_jku -v
IF errorlevel 9009 GOTO :error
GOTO :next

:profile
python -m unittest discover -p "profile*.py" -v
IF errorlevel 9009 GOTO :error
GOTO :next

:dist
FOR %%A IN (py34_64 py35_64 py36_64 py37_64 py34_32 py35_32 py36_32 py37_32) DO (
  activate %%A
  python setup.py bdist_wheel
)
IF errorlevel 9009 GOTO :error
GOTO :next

:error
ECHO.
ECHO.Somehting is missing in your Python installation.
ECHO.Please make sure you have properly installed Anaconda3
ECHO.(https://www.continuum.io/downloads), and that you are
ECHO.running the "Anaconda Shell Command".
ECHO.
exit /b 1

:next

:end
popd
ENDLOCAL