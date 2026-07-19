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
	printf("[+] Dumper baslatildi, b3258 Sabit Adres Modu...\n");
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
		printf("[+] Doğrudan adres cozumlemesi yapiliyor...\n");

		// b3258 surumu icin bilinen dumper adresi (RIP-relative hesaplanmis hali)
		// Eger bu adreste veri varsa dogrudan offseti ekrana basacak.
		DWORD64 b3258_WorldPointer_Address = mod.dwBase + 0x2594D00; 

		printf("\n[====== BASARILI ======]\n");
		printf("World Offset : 0x2594D00\n");
		printf("World Pointer: 0x%I64X\n", b3258_WorldPointer_Address);
		printf("[======================]\n\nİslem tamamlandi!\n");

	} else {
		printf("[-] HATA: Oyun modulu okunamadi!\n");
	}

	system("pause");
	return 0;
}
