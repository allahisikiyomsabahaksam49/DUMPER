#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <TlHelp32.h>
#include <string>

DWORD64 Base;
DWORD pid;
HANDLE pHandle;

struct module {
	DWORD64 dwBase, dwSize;
};

module TargetModule;
HANDLE TargetProcess;
DWORD64 TargetId;
std::wstring DetectedProcessName = L"";

bool MemoryCompare(const BYTE* bData, const BYTE* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++bData, ++bMask) {
		if (*szMask == 'x' && *bData != *bMask) return false;
	}
	return *szMask == 0;
}

DWORD64 FindSignature(DWORD64 start, DWORD64 size, const char* sig, const char* mask) {
	BYTE* data = new BYTE[size];
	SIZE_T bytesRead;
	ReadProcessMemory(TargetProcess, (LPVOID)start, data, size, &bytesRead);

	for (DWORD64 i = 0; i < size; i++) {
		if (MemoryCompare(data + i, (const BYTE*)sig, mask)) {
			delete[] data;
			return start + i;
		}
	}
	delete[] data;
	return 0;
}

HANDLE GetProcessSmart() {
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W entry = { sizeof(entry) }; 
	if (Process32FirstW(handle, &entry)) {
		do {
			std::wstring exeName(entry.szExeFile);
			if (exeName.find(L"GTAProcess") != std::wstring::npos || _wcsicmp(entry.szExeFile, L"gta5.exe") == 0) {
				TargetId = entry.th32ProcessID;
				DetectedProcessName = entry.szExeFile;
				CloseHandle(handle);
				TargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, TargetId);
				return TargetProcess;
			}
		} while (Process32NextW(handle, &entry));
	}
	CloseHandle(handle);
	return NULL;
}

module GetModule(const wchar_t* moduleName) {
	HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, TargetId);
	MODULEENTRY32W mEntry = { sizeof(mEntry) };
	if (Module32FirstW(hmodule, &mEntry)) {
		do {
			if (!_wcsicmp(mEntry.szModule, moduleName)) {
				CloseHandle(hmodule);
				return { (DWORD64)mEntry.hModule, mEntry.modBaseSize };
			}
		} while (Module32NextW(hmodule, &mEntry));
	}
	CloseHandle(hmodule);
	return { 0, 0 };
}

int main() {
	printf("[+] Dumper baslatildi, b3258 icin tarama yapiliyor...\n");
	Sleep(1000);

	HWND hWnd = FindWindowA("grcWindow", nullptr);
	if (hWnd) {
		GetWindowThreadProcessId(hWnd, &pid);
		printf("[+] GTA V Penceresi bulundu. PID: %d\n", pid);
	}

	if (GetProcessSmart()) {
		std::wcout << L"[+] Oyun sureci yakalandi: " << DetectedProcessName << L" (PID: " << TargetId << L")\n";
	} else {
		printf("[-] HATA: Oyun sureci bulunamadi!\n");
		system("pause");
		return 0;
	}

	module mod = GetModule(DetectedProcessName.c_str());

	if (mod.dwBase) {
		printf("[+] Modul taban adresi alindi: 0x%I64X\n", mod.dwBase);
		printf("[+] b3258 / Guncel surum patternleri taraniyor...\n");

		DWORD64 TempWorldPTR = 0;

		// 1. Deneme: En Guncel b3258 / b3095 Sürümleri İçin Özel Pattern
		TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8B\x05\x00\x00\x00\x00\x45\x33\xC9\x48\x8B\x48\x08\x48\x85\xC9\x74\x07", "xxx????xxxxxxxxxxxx");

		// 2. Deneme: Alternatif b3258 / Havuz Taraması
		if (!TempWorldPTR) {
			TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8B\x05\x00\x00\x00\x00\x45\x33\xC9\x48\x8B\x48", "xxx????xxxxxxx");
		}

		// 3. Deneme: Yedek Genel Pattern
		if (!TempWorldPTR) {
			TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8b\x05\x00\x00\x00\x00\x45\x00\x00\x00\x00\x48\x8b\x48\x08", "xxx????x????xxxx");
		}

		if (TempWorldPTR) {
			// Offset hesaplama (RIP-relative adres çözme)
			DWORD64 world = TempWorldPTR + *(int*)(TempWorldPTR + 3) + 7;
			printf("\n[====== BASARILI ======]\n");
			printf("World Offset : 0x%I64X\n", world - mod.dwBase);
			printf("World Pointer: 0x%I64X\n", world);
			printf("[======================]\n\nİslem tamamlandi!\n");
		} else {
			printf("[-] HATA: b3258 surumune uygun imza bulunamadi! Kod icindeki imzalari kontrol edin.\n");
		}
	} else {
		printf("[-] HATA: Oyun modulu okunamadi!\n");
	}

	system("pause");
	return 0;
}
