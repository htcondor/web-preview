@echo off & setlocal
if x%1% == x (
  echo Usage: "sleep <seconds>"
  goto :EOF
) 
set /a COUNT=%1+1
echo Sleeping for %1 second(s)
ping -n %COUNT% 127.0.0.1 >NUL 2>&1
endlocal
