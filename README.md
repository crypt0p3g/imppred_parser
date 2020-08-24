# imppred_parser (JpnIHDS.dat)

This repository contains source code for the article:
English:　https://ierae.co.jp/blog/jpnihds_en/ 
日本語:　https://ierae.co.jp/blog/jpnihds_jp/

> **JpnIHDS.dat** contains last Japanese input from the user (Japanese IME keyboard).
> Use this source code to parse it.

### Beacon Object File (Cobalt Strike 4.1) 

**File:** read_jpn_pred.cpp

##### how to compile:

Visual Studio:

```
cl.exe /c /GS- /TP read_jpn_pred.cpp /Foread_jpn_pred.o
```

MinGW: 

```
x86_64-w64-mingw32-gcc -c read_jpn_pred.cpp -o read_jpn_pred.o
```

##### how to use:

```
inline-execute read_jpn_pred.o
```

### C++ WinAPI Implementation

**File:** imppred_parser.cpp

##### how to compile:

Create new console application in Visual Studio, import **imppred_parser.cpp**.

##### how to use:


Read file from current user's folder:

```
imppred_parser.exe
```

Read specified file:

```
imppred_parser.exe C:\Temp\JpnIHDS.dat
```

### Python Script

**File:** userdict_jpn.py

##### how to use:

Verbose output with slack extraction:

```
./userdict_jpn.py advanced JpnIHDS.dat
```

Simple "timestamp: text" output:

```
./userdict_jpn.py JpnIHDS.dat
```

