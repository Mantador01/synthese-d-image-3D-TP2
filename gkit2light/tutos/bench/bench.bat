@echo off 

set prefix= bench
:: set prefix= vega64
:: set prefix= geforce1070
:: set prefix= geforce1080ti
:: set prefix= geforce2080
:: set prefix= geforce3080
:: set prefix= ryzen5500U
md %prefix%

:: triangles
set triangles= 65536 262144 1048576 4194304

(for %%n in (%triangles%) do (
echo bench triangles %%n
echo %prefix%/triangles-%%n.txt

bin\benchv3 --frames 1000 --rotation true --triangles %%n -o %prefix%/triangles-%%n.txt

))

:: fragments
set lights= 1 8 16 32

(for %%n in (%lights%) do (
echo bench lights %%n
echo %prefix%/lights-%%n.txt

bin\benchv3 --frames 1000 --rotation true --lights %%n -o %prefix%/lights-%%n.txt

))

:: rasterization
set sizes= 4 8 16 32 64 128 256 512 1024

(for %%n in (%sizes%) do (
echo bench grid %%n
echo %prefix%/grid-%%n.txt

bin\benchv3 --frames 2000 --rotation true --size %%n -o %prefix%/grid-%%n.txt

))

:: overdraw
set slices= 1 4 8 16 32 64 128 256

(for %%n in (%slices%) do (
echo bench overdraw %%n
echo %prefix%/overdraw-%%n.txt

bin\benchv3 --frames 1000 --rotation true --overdraw  %%n -o %prefix%/overdraw-%%n.txt

))

:: cpu
bin\bench_trace --frames 1000 -o %prefix%/cpu-1.txt --trace bench-data/trace.txt
bin\bench_trace --frames 1000 -o %prefix%/cpu2-1.txt --trace bench-data/trace2.txt
bin\bench_trace --frames 1000 -o %prefix%/cpu3-1.txt --trace bench-data/trace3.txt

