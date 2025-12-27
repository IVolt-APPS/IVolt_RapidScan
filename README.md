# IVolt RapidScan User Manual

**Version:** 1.0  
**Release Time:** 1700 CST — 04-19-2025  
**Platform:** Windows x64  
**Interface:** Command Line (CLI)

---

## Overview

**IVolt RapidScan** is a high-performance disk and directory scanning utility designed to enumerate files and folders at extremely high speed. It recursively scans one or more paths and exports detailed metadata to CSV files.

Typical use cases include:
- Fast inventory of disks or directories
- Auditing file structures
- Large-scale file analysis and indexing
- Performance benchmarking of storage devices

---

## Key Features

- Recursive file and directory scanning
- Multiple input paths per run
- UTF-8 encoded CSV output
- Extremely high throughput (tested near 82,000 entries/sec)
- Optional high-resolution execution timing
- Efficient Windows filesystem API usage

---

## System Requirements

- Windows 7 or later
- NTFS or compatible filesystem
- Command Prompt, PowerShell, or terminal environment
- Write access to the output directory

---

## Command Line Syntax

```text
IVolt_RapidScan.exe [-timeit] -paths <path1,path2,...> -out <output_directory>

![image](https://github.com/user-attachments/assets/15cf03fa-e171-4c3d-aace-e529186d5c78)
