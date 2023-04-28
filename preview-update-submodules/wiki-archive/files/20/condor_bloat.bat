@echo off
for /L %%i in (0,1,%2) do condor_submit %1
