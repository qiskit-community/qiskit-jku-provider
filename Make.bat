:: Copyright 2018 IBM RESEARCH. All Rights Reserved.
:: 
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.
:: =============================================================================

@ECHO OFF
@SETLOCAL enabledelayedexpansion

pushd %~dp0

SET target=%~n1

IF "%target%"=="lint" GOTO :lint
IF "%target%"=="test" GOTO :test
IF "%target%"=="profile" GOTO :profile

:usage
ECHO.
ECHO.Usage:
ECHO.    .\make lint    Runs Pyhton source code analysis tool
ECHO.    .\make test    Runs tests
ECHO.    .\make profile Runs profiling tests
ECHO.
GOTO :end

:lint
pylint -rn qiskit_addon_jku test_jku_simulator
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