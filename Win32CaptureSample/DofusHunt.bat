@echo off
setlocal

REM Check if --install argument is provided
set INSTALL=0
for %%i in (%*) do (
    if "%%i"=="--install" (
        set INSTALL=1
    )
)

REM Change to the python-server subfolder
pushd python-server

REM Create virtual environment and install requirements if --install is provided
if %INSTALL%==1 (
    echo Creating virtual environment...
    python -m venv venv
    echo Activating virtual environment...
    call venv\Scripts\activate
    echo Installing requirements...
    pip install -r requirements.txt
    echo Requirements installed.
) else (
    REM Activate virtual environment and launch server.py
    echo Activating virtual environment...
    call venv\Scripts\activate
    echo Launching server.py...
    python server.py %*
)

REM Return to the original directory
popd


REM Run DofusHunt.exe
REM TODO
endlocal
