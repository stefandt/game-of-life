#!/bin/bash
# macOS / Linux build helper — mirrors cmake_with_vsdev.cmd but without
# the Windows VsDevCmd activation step (not needed on Unix).
cmake "$@"
