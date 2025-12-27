
#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

// Function prototypes
void listDirectoryContents(const wchar_t* path, FILE* outFile);
void fileTimeToLocalTimeString(const FILETIME* ft, wchar_t* buffer, size_t bufSize);
static const wchar_t* RAPIDSCAN_HELP_TEXT =
L"IVolt RapidScan v1.0\n"
L"High-performance disk and directory scanner\n"
L"\n"
L"Usage:\n"
L"  IVolt_RapidScan.exe [-timeit] -paths <path1,path2,...> -out <output_directory>\n"
L"\n"
L"Required arguments:\n"
L"  -paths   Comma-separated list of files or directories to scan\n"
L"  -out     Output directory for generated CSV files\n"
L"\n"
L"Optional arguments:\n"
L"  -timeit  Display total execution time using high-resolution timer\n"
L"  -h, --help, /?  Display this help text\n"
L"\n"
L"Examples:\n"
L"  IVolt_RapidScan.exe -paths C: -out C:\\ScanResults\n"
L"  IVolt_RapidScan.exe -paths C:\\Windows,D:\\Data -out D:\\Reports\n"
L"  IVolt_RapidScan.exe -timeit -paths C:\\Users -out C:\\ScanResults\n"
L"\n"
L"Notes:\n"
L"  - One CSV file is generated per input path\n"
L"  - Reparse points are not recursively followed\n"
L"  - Access-denied directories are skipped with warnings\n";

int wmain(int argc, wchar_t* argv[]) {
	// <-- ADD: timing variables
	int doTimeIt = 0;
	LARGE_INTEGER freq, startCount, endCount;

	// Updated usage: both -paths and -out are required.   MA
	if (argc < 2) {
		fwprintf(stderr, L"Invalid usage.\n\n");
		fwprintf(stderr, L"%s", RAPIDSCAN_HELP_TEXT);
		return 10;
	}


	const wchar_t* pathsArg = NULL;
	const wchar_t* outDir = NULL;

	// Parse command-line arguments for "-paths" and "-out"
	for (int i = 1; i < argc; i++) {

		if (_wcsicmp(argv[i], L"--help") == 0 ||
			_wcsicmp(argv[i], L"-h") == 0 ||
			wcscmp(argv[i], L"/?") == 0) {
			wprintf(L"%s", RAPIDSCAN_HELP_TEXT);
			return 0;
		}


		if (_wcsicmp(argv[i], L"-timeit") == 0) {
			doTimeIt = 1;                        // Added For Timing Option   -- MA
		}
		else if (_wcsicmp(argv[i], L"-paths") == 0) {
			if (i + 1 < argc)
				pathsArg = argv[++i];
			else {
				fwprintf(stderr, L"Error: Missing argument for -paths.\n");
				return 20;
			}
		}
		else if (_wcsicmp(argv[i], L"-out") == 0) {
			if (i + 1 < argc)
				outDir = argv[++i];
			else {
				fwprintf(stderr, L"Error: Missing argument for -out.\n");
				return 30;
			}
		}
	}

	if (!pathsArg) {
		fwprintf(stderr, L"Error: -paths argument is required.\n");
		return 40;
	}
	if (!outDir) {
		fwprintf(stderr, L"Error: -out argument is required.\n");
		return 50;
	}

	// Adding Timing Option Code  -- MA
	if (doTimeIt) {
		if (!QueryPerformanceFrequency(&freq)) {
			fwprintf(stderr, L"Error: High-resolution timer not supported.\n");
			doTimeIt = 0;
		}
		else {
			QueryPerformanceCounter(&startCount);
		}
	}


	// Ensure the output directory exists; if not, create it.
	if (GetFileAttributesW(outDir) == INVALID_FILE_ATTRIBUTES) {
		if (!CreateDirectoryW(outDir, NULL)) {
			fwprintf(stderr, L"Error: Unable to create output directory \"%s\".\n", outDir);
			return 60;
		}
	}

	// Duplicate the paths argument
	wchar_t* pathsList = _wcsdup(pathsArg);
	if (!pathsList) {
		fwprintf(stderr, L"Memory allocation failure.\n");
		return 70;
	}
	wchar_t* context = NULL;
	wchar_t* pathToken = wcstok(pathsList, L",", &context);
	if (!pathToken) {
		fwprintf(stderr, L"Error: No paths provided after -paths.\nUsage: %s -paths <path1,path2,...> -out <output directory>\n", argv[0]);
		free(pathsList);
		return 80;
	}

	// Process each path token
	while (pathToken) {
		// Trim leading whitespace
		while (*pathToken == L' ' || *pathToken == L'\t' || *pathToken == L'\n' || *pathToken == L'\r') {
			pathToken++;
		}
		// Remove trailing whitespace
		size_t tokenLen = wcslen(pathToken);
		while (tokenLen > 0 && (pathToken[tokenLen - 1] == L' ' || pathToken[tokenLen - 1] == L'\t' ||
			pathToken[tokenLen - 1] == L'\n' || pathToken[tokenLen - 1] == L'\r')) {
			pathToken[tokenLen - 1] = L'\0';
			tokenLen--;
		}
		if (tokenLen == 0) { // skip empty token
			pathToken = wcstok(NULL, L",", &context);
			continue;
		}

		// Normalize the path
		wchar_t normalizedPath[MAX_PATH * 4];
		wcsncpy(normalizedPath, pathToken, (sizeof(normalizedPath) / sizeof(normalizedPath[0])) - 1);
		normalizedPath[(sizeof(normalizedPath) / sizeof(normalizedPath[0])) - 1] = L'\0';
		tokenLen = wcslen(normalizedPath);
		if (tokenLen == 2 && normalizedPath[1] == L':') {
			wcscat(normalizedPath, L"\\");
			tokenLen = wcslen(normalizedPath);
		}
		if (tokenLen > 3 && normalizedPath[tokenLen - 1] == L'\\') {
			normalizedPath[tokenLen - 1] = L'\0';
			tokenLen--;
		}

		// Determine output CSV file name based on normalizedPath
		wchar_t fileName[MAX_PATH];
		if (tokenLen >= 2 && normalizedPath[1] == L':' &&
			(tokenLen == 2 || (tokenLen == 3 && normalizedPath[2] == L'\\'))) {
			wchar_t driveLetter = towupper(normalizedPath[0]);
			swprintf(fileName, MAX_PATH, L"%c.csv", driveLetter);
		}
		else {
			const wchar_t* lastSlash = wcsrchr(normalizedPath, L'\\');
			if (!lastSlash)
				lastSlash = normalizedPath;
			else
				lastSlash++; // move past the slash
			if (*lastSlash == L'\0') {
				wchar_t* prevSlash = wcsrchr(normalizedPath, L'\\');
				if (prevSlash && *(prevSlash + 1) == L'\0') {
					*prevSlash = L'\0';
					lastSlash = wcsrchr(normalizedPath, L'\\');
					lastSlash = lastSlash ? lastSlash + 1 : normalizedPath;
				}
			}
			swprintf(fileName, MAX_PATH, L"%s.csv", lastSlash);
		}

		// Prepend the output directory to the file name.
		wchar_t outPath[MAX_PATH];
		swprintf(outPath, MAX_PATH, L"%s\\%s", outDir, fileName);

		// Open CSV file for writing (UTF-8 encoding)
		FILE* outFile = _wfopen(outPath, L"w, ccs=UTF-8");
		if (!outFile) {
			fwprintf(stderr, L"Error: Cannot open output file '%s' for writing.\n", outPath);
			pathToken = wcstok(NULL, L",", &context);
			continue;
		}

		// Write CSV header and flush
		if (fwprintf(outFile, L"\"FullPath\",\"Type\",\"Size\",\"CreationTime\",\"LastAccessTime\",\"LastWriteTime\"\n") < 0) {
			fwprintf(stderr, L"Error writing header to output file '%s'.\n", outPath);
			fclose(outFile);
			pathToken = wcstok(NULL, L",", &context);
			continue;
		}
		fflush(outFile);

		// Check if normalizedPath exists (file or directory)
		DWORD attrib = GetFileAttributesW(normalizedPath);
		if (attrib == INVALID_FILE_ATTRIBUTES) {
			fwprintf(stderr, L"Warning: Path \"%s\" not found or inaccessible.\n", normalizedPath);
		}
		else {
			if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
				WIN32_FIND_DATAW ffd;
				HANDLE hFind = FindFirstFileW(normalizedPath, &ffd);
				if (hFind != INVALID_HANDLE_VALUE) {
					wchar_t creation[20], access[20], write[20];
					fileTimeToLocalTimeString(&ffd.ftCreationTime, creation, 20);
					fileTimeToLocalTimeString(&ffd.ftLastAccessTime, access, 20);
					fileTimeToLocalTimeString(&ffd.ftLastWriteTime, write, 20);
					if (fwprintf(outFile, L"\"%s\",\"Directory\",\"0\",\"%s\",\"%s\",\"%s\"\n",
						normalizedPath, creation, access, write) < 0) {
						fwprintf(stderr, L"Error writing directory info to '%s'.\n", outPath);
					}
					FindClose(hFind);
				}
				else {
					if (fwprintf(outFile, L"\"%s\",\"Directory\",\"0\",,,\n", normalizedPath) < 0) {
						fwprintf(stderr, L"Error writing directory info to '%s'.\n", outPath);
					}
				}
				listDirectoryContents(normalizedPath, outFile);
			}
			else {
				WIN32_FIND_DATAW ffd;
				HANDLE hFind = FindFirstFileW(normalizedPath, &ffd);
				if (hFind != INVALID_HANDLE_VALUE) {
					wchar_t creation[20], access[20], write[20];
					fileTimeToLocalTimeString(&ffd.ftCreationTime, creation, 20);
					fileTimeToLocalTimeString(&ffd.ftLastAccessTime, access, 20);
					fileTimeToLocalTimeString(&ffd.ftLastWriteTime, write, 20);
					ULONGLONG fileSize = ((ULONGLONG)ffd.nFileSizeHigh << 32) | ffd.nFileSizeLow;
					if (fwprintf(outFile, L"\"%s\",\"File\",\"%llu\",\"%s\",\"%s\",\"%s\"\n",
						normalizedPath, fileSize, creation, access, write) < 0) {
						fwprintf(stderr, L"Error writing file info to '%s'.\n", outPath);
					}
					FindClose(hFind);
				}
				else {
					fwprintf(stderr, L"Error: Unable to retrieve information for file \"%s\".\n", normalizedPath);
					fwprintf(outFile, L"\"%s\",\"File\",,,,\n", normalizedPath);
				}
			}
		}

		if (fflush(outFile) != 0) {
			fwprintf(stderr, L"Error flushing output file '%s'.\n", outPath);
		}
		fclose(outFile);
		wprintf(L"Output written to %s\n", outPath);

		pathToken = wcstok(NULL, L",", &context);
	}

	free(pathsList);

	//  Timer Output Added By    MA
	if (doTimeIt) {
		QueryPerformanceCounter(&endCount);
		double elapsed = (double)(endCount.QuadPart - startCount.QuadPart) / (double)freq.QuadPart;
		wprintf(L"Total execution time: %.6f seconds\n", elapsed);
	}


	return 0;
}

/**
 * Recursively list contents of a directory.
 * @param path The directory path (normalized, without trailing "\*").
 * @param outFile File handle to the open CSV file.
 */
void listDirectoryContents(const wchar_t* path, FILE* outFile) {
	WIN32_FIND_DATAW ffd;
	memset(&ffd, 0, sizeof(WIN32_FIND_DATAW));
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t searchPath[MAX_PATH * 4];

	size_t pathLen = wcslen(path);
	wcsncpy(searchPath, path, (sizeof(searchPath) / sizeof(searchPath[0])) - 5);
	searchPath[(sizeof(searchPath) / sizeof(searchPath[0])) - 5] = L'\0';
	if (pathLen == 0)
		return;
	if (searchPath[pathLen - 1] != L'\\') {
		searchPath[pathLen] = L'\\';
		searchPath[pathLen + 1] = L'\0';
		pathLen++;
	}
	wcscat(searchPath, L"*");

	hFind = FindFirstFileExW(searchPath, FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, 0);
	if (hFind == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err == ERROR_ACCESS_DENIED)
			fwprintf(stderr, L"Warning: Access denied to directory \"%s\", skipping.\n", path);
		else if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND)
			fwprintf(stderr, L"Warning: Unable to open directory \"%s\" (Error %lu), skipping.\n", path, err);
		return;
	}

	do {
		const wchar_t* name = ffd.cFileName;
		if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
			continue;
		wchar_t fullPath[MAX_PATH * 4];
		swprintf(fullPath, (sizeof(fullPath) / sizeof(fullPath[0])), L"%s\\%s", path, name);

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			wchar_t creation[20], access[20], write[20];
			fileTimeToLocalTimeString(&ffd.ftCreationTime, creation, 20);
			fileTimeToLocalTimeString(&ffd.ftLastAccessTime, access, 20);
			fileTimeToLocalTimeString(&ffd.ftLastWriteTime, write, 20);
			if (fwprintf(outFile, L"\"%s\",\"Directory\",\"0\",\"%s\",\"%s\",\"%s\"\n", fullPath, creation, access, write) < 0)
				fwprintf(stderr, L"Error writing directory info for \"%s\".\n", fullPath);
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
				listDirectoryContents(fullPath, outFile);
		}
		else {
			ULONGLONG fileSize = ((ULONGLONG)ffd.nFileSizeHigh << 32) | ffd.nFileSizeLow;
			wchar_t creation[20], access[20], write[20];
			fileTimeToLocalTimeString(&ffd.ftCreationTime, creation, 20);
			fileTimeToLocalTimeString(&ffd.ftLastAccessTime, access, 20);
			fileTimeToLocalTimeString(&ffd.ftLastWriteTime, write, 20);
			if (fwprintf(outFile, L"\"%s\",\"File\",\"%llu\",\"%s\",\"%s\",\"%s\"\n",
				fullPath, fileSize, creation, access, write) < 0)
				fwprintf(stderr, L"Error writing file info for \"%s\".\n", fullPath);
		}
	} while (FindNextFileW(hFind, &ffd) != 0);

	DWORD error = GetLastError();
	if (error != ERROR_NO_MORE_FILES)
		fwprintf(stderr, L"Error: FindNextFile failed in \"%s\" (Error %lu)\n", path, error);

	FindClose(hFind);
}

/**       TODO Refine Comments
 * Convert a FILETIME (UTC) to a local time formatted string "YYYY-MM-DD HH:MM:SS".
 * @param ft Pointer to FILETIME.
 * @param buffer Wide-char buffer for the formatted timestamp.
 * @param bufSize Size of the buffer (number of wchar_t elements).
 */
void fileTimeToLocalTimeString(const FILETIME* ft, wchar_t* buffer, size_t bufSize) {
	if (ft->dwLowDateTime == 0 && ft->dwHighDateTime == 0) {
		wcsncpy(buffer, L"", bufSize);
		return;
	}
	FILETIME localFt;
	SYSTEMTIME st;
	if (!FileTimeToLocalFileTime(ft, &localFt))
		localFt = *ft;
	if (FileTimeToSystemTime(&localFt, &st)) {
		swprintf(buffer, bufSize, L"%04d-%02d-%02d %02d:%02d:%02d",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond);
	}
	else {
		wcsncpy(buffer, L"", bufSize);
	}
}
