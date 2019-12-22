@ECHO OFF
cl.exe /EHsc /std:c++17 User32.lib vulkan-1.lib 001_Present.cpp
001_Present.exe
