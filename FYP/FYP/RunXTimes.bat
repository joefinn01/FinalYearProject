@echo off

for /L %%N in (1,1,%3) do (
%1\FYP.exe %2 %%N)