#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <TlHelp32.h>

DWORD64 Base;
DWORD pid;
HANDLE pHandle;

struct module {
	DWORD64 dwBase, dwSize;
};

module TargetModule;
HANDLE TargetProcess;
DWORD64 TargetId;

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

HANDLE GetProcess(const wchar_t* processName) {
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W entry = { sizeof(entry) }; 
	if (Process32FirstW(handle, &entry)) {
		do {
			if (!_wcsicmp(entry.szExeFile, processName)) {
				TargetId = entry.th32ProcessID;
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
	printf("[+] Dumper baslatildi, oyun araniyor...\n");
	Sleep(1000);

	HWND hWnd = FindWindowA("grcWindow", nullptr);
	if (hWnd) {
		GetWindowThreadProcessId(hWnd, &pid);
		printf("[+] GTA V Penceresi bulundu. PID: %d\n", pid);
	}

	bool processFound = false;
	if (GetProcess(L"FiveM_GTAProcess.exe")) {
		printf("[+] FiveM_GTAProcess.exe sureci yakalandi!\n");
		processFound = true;
	} else if (GetProcess(L"gta5.exe")) {
		printf("[+] gta5.exe sureci yakalandi!\n");
		processFound = true;
	}

	if (!processFound) {
		printf("[-] HATA: Oyun sureci bulunamadi! Lutfen sunucuya girip tekrar deneyin.\n");
		system("pause");
		return 0;
	}

	module mod = GetModule(L"FiveM_GTAProcess.exe");
	if (!mod.dwBase) mod = GetModule(L"gta5.exe");

	if (mod.dwBase) {
		printf("[+] Modul taban adresi alindi: 0x%I64X\n", mod.dwBase);
		printf("[+] Pattern taramasi yapiliyor, lutfen bekleyin...\n");

		// Guncel FiveM surumleri icin alternatif hafifletilmis pattern taraması
		DWORD64 TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8b\x05\x00\x00\x00\x00\x45\x00\x00\x00\x00\x48\x8b\x48\x08", "xxx????x????xxxx");
		
		if (!TempWorldPTR) {
			// Eski pattern'i de yedek olarak aratıyoruz
			TempWorldPTR = FindSignature(mod.dwBase, mod.dwSize, "\x48\x8b\x05\x00\x00\x00\x00\x45\x00\x00\x00\x00\x48\x8b\x48\x08\x48\x85\xc9\x74\x07", "xxx????x????xxxxxxxxx");
		}

		if (TempWorldPTR) {
			DWORD64 world = TempWorldPTR + *(int*)(TempWorldPTR + 3) + 7;
			printf("\n[====== SONUC ======]\n");
			printf("world offset : 0x%I64X\n", world - mod.dwBase);
			printf("world pointer: 0x%I64X\n", world);
			printf("[===================]\n\nDerleme Basarili!\n");
		} else {
			printf("[-] HATA: Oyunun bu surumu icin guncel Pattern (Imza) bulunamadi!\n");
		}
	} else {
		printf("[-] HATA: Oyun modulu hafizadan okunamadi!\n");
	}

	system("pause");
	return 0;
}
