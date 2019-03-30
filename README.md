# faustmd
Static metadata generator for Faust/C++

## Description

This program builds the metadata for a Faust DSP ahead of time, rather than dynamically.

The result is a block of C++ code which can be appended to the code generation.

## Typical usage

```
faust -uim MyProcessor.dsp -a MyArchitecture.cpp > MyProcessor.cpp
faustmd MyProcessor.dsp >> MyProcessor.cpp
```

It will probably require the flag `-uim` to make the instance variables accessible.

## Features

- general information
- description of controls and signals
- control ranges, units and scales
- getters and setters of control values by method or index
