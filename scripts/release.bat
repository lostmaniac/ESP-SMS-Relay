@echo off
setlocal enabledelayedexpansion

REM ESP-SMS-Relay Release Script (Windows Version)
REM Create new version tags and trigger automatic release

echo ========================================
echo ESP-SMS-Relay Version Release Tool
echo ========================================
echo.

REM Check if in git repository
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Current directory is not a git repository
    pause
    exit /b 1
)

REM Check if working directory is clean
git diff-index --quiet HEAD -- >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Working directory has uncommitted changes, please commit or stash first
    echo.
    git status --porcelain
    pause
    exit /b 1
)

REM Get current branch
for /f "tokens=*" %%i in ('git branch --show-current') do set current_branch=%%i
echo [INFO] Current branch: !current_branch!

REM Check if on main branch
if not "!current_branch!"=="main" (
    echo [WARNING] Not on main branch, recommend switching to main branch for release
    set /p continue="Continue anyway? (y/N): "
    if /i not "!continue!"=="y" (
        exit /b 1
    )
)

REM Get latest tag
for /f "tokens=*" %%i in ('git describe --tags --abbrev=0 2^>nul') do set last_tag=%%i
if "!last_tag!"=="" set last_tag=v0.0.0
echo [INFO] Previous version: !last_tag!
echo.

REM Show commits since last release
echo [INFO] Commits since last release:
git log !last_tag!..HEAD --oneline --no-merges
echo.

REM Prompt for new version number
echo [INFO] Please enter new version number (format: v1.2.3):
set /p new_version="Version: "

REM Simple version number format validation
echo !new_version! | findstr /r "^v[0-9]*\.[0-9]*\.[0-9]*" >nul
if errorlevel 1 (
    echo [ERROR] Invalid version format, should be v1.2.3 format
    pause
    exit /b 1
)

REM Check if tag already exists
git rev-parse !new_version! >nul 2>&1
if not errorlevel 1 (
    echo [ERROR] Tag !new_version! already exists
    pause
    exit /b 1
)

REM Confirm release
echo.
echo [WARNING] About to create tag !new_version! and push to remote repository
echo [WARNING] This will trigger automatic build and release process
set /p confirm="Confirm release? (y/N): "
if /i not "!confirm!"=="y" (
    echo [INFO] Release cancelled
    pause
    exit /b 0
)

REM Create tag
echo.
echo [INFO] Creating tag !new_version!...
git tag -a "!new_version!" -m "Release !new_version!"
if errorlevel 1 (
    echo [ERROR] Failed to create tag
    pause
    exit /b 1
)

REM Push tag
echo [INFO] Pushing tag to remote repository...
git push origin "!new_version!"
if errorlevel 1 (
    echo [ERROR] Failed to push tag
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Tag !new_version! created and pushed successfully
echo [INFO] GitHub Actions will automatically start build and release process
echo [INFO] Please visit GitHub repository to check build status and release progress

REM Get repository info
for /f "tokens=*" %%i in ('git config --get remote.origin.url') do set remote_url=%%i
echo !remote_url! | findstr "github.com" >nul
if not errorlevel 1 (
    echo.
    echo [INFO] Related links:
    echo [INFO] - Actions page: Open the Actions tab in your browser
    echo [INFO] - Releases page: Open the Releases tab in your browser
)

echo.
echo ========================================
echo Release process started!
echo ========================================
pause