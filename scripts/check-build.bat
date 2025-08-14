@echo off
setlocal enabledelayedexpansion

REM ESP-SMS-Relay Build Status Check Script
REM Check local build environment and remote CI status

echo ========================================
echo ESP-SMS-Relay Build Status Check
echo ========================================
echo.

REM Check if in git repository
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Current directory is not a git repository
    pause
    exit /b 1
)

REM Check PlatformIO installation
echo [CHECK] PlatformIO installation status...
pio --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] PlatformIO not installed or not in PATH
    echo [SUGGEST] Please install PlatformIO: pip install platformio
    pause
    exit /b 1
) else (
    echo [SUCCESS] PlatformIO is installed
    for /f "tokens=*" %%i in ('pio --version') do echo          Version: %%i
)
echo.

REM Check project configuration
echo [CHECK] Project configuration files...
if exist "platformio.ini" (
    echo [SUCCESS] platformio.ini exists
) else (
    echo [ERROR] platformio.ini not found
    pause
    exit /b 1
)

if exist ".github\workflows\build-and-release.yml" (
    echo [SUCCESS] GitHub Actions build workflow configured
) else (
    echo [WARNING] GitHub Actions build workflow not found
)

if exist ".github\workflows\ci.yml" (
    echo [SUCCESS] GitHub Actions CI workflow configured
) else (
    echo [WARNING] GitHub Actions CI workflow not found
)
echo.

REM Check dependencies
echo [CHECK] Project dependencies...
pio pkg list >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Cannot get dependency list, may need to install dependencies
    echo [SUGGEST] Run: pio pkg install
) else (
    echo [SUCCESS] Project dependencies check completed
)
echo.

REM Try compilation check
echo [CHECK] Build environment test...
echo [INFO] Running syntax check compilation...
pio run --target checkprogsize >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Compilation check failed, may have syntax errors
    echo [SUGGEST] Run detailed compilation: pio run --verbose
) else (
    echo [SUCCESS] Compilation check passed
    REM Get compilation info
    for /f "tokens=*" %%i in ('pio run --target checkprogsize 2^>^&1 ^| findstr "RAM:"') do echo          %%i
    for /f "tokens=*" %%i in ('pio run --target checkprogsize 2^>^&1 ^| findstr "Flash:"') do echo          %%i
)
echo.

REM Get Git status
echo [CHECK] Git repository status...
for /f "tokens=*" %%i in ('git branch --show-current') do set current_branch=%%i
echo [INFO] Current branch: !current_branch!

for /f "tokens=*" %%i in ('git describe --tags --abbrev=0 2^>nul') do set last_tag=%%i
if "!last_tag!"=="" (
    echo [INFO] Latest tag: None
) else (
    echo [INFO] Latest tag: !last_tag!
)

for /f "tokens=*" %%i in ('git rev-parse --short HEAD') do set commit_hash=%%i
echo [INFO] Current commit: !commit_hash!

git diff-index --quiet HEAD -- >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Working directory has uncommitted changes
) else (
    echo [SUCCESS] Working directory is clean
)
echo.

REM Check remote repository
echo [CHECK] Remote repository info...
for /f "tokens=*" %%i in ('git config --get remote.origin.url') do set remote_url=%%i
echo [INFO] Remote repository: !remote_url!

echo !remote_url! | findstr "github.com" >nul
if not errorlevel 1 (
    echo [INFO] GitHub repository detected
    echo [SUGGEST] You can visit the following pages to check CI status:
    echo           - Actions: Open the Actions tab in your browser
    echo           - Releases: Open the Releases tab in your browser
)
echo.

REM Show build suggestions
echo ========================================
echo Build Suggestions
echo ========================================
echo.
echo [Local Build]
echo   Full compile: pio run
echo   Verbose compile: pio run --verbose
echo   Clean build: pio run --target clean
echo   Upload firmware: pio run --target upload
echo.
echo [Version Release]
echo   Create release: scripts\release.bat
echo   Manual tag: git tag v1.x.x ^&^& git push origin v1.x.x
echo.
echo [CI/CD Status]
echo   Pushing code to main/develop branch triggers auto build
echo   Creating tags triggers auto release to GitHub Releases
echo.
echo ========================================
echo Check Complete
echo ========================================
pause