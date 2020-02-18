#pragma once

#include <Windows.h>

/*
 * ���̲�����
 * ����ͬ������صĲ���
 * ������
 *      1. ��д�ڴ�
 *      2. ��ȡ�����߳������Ļ���
 *      3. ��ѯ�޸��ڴ�����
 *      4. ����ַ�Ƿ���Ч
 */

class ProcessOperating
{
public:
    ProcessOperating();
    virtual ~ProcessOperating();

    /*
     * ��������
     *      strApp��������
     *      dwCreateFlags��������־
     *      si��������Ϣ
     *      pi��������Ϣ
     */
    static bool CreateProcess(char *strApp, DWORD dwCreateFlags, STARTUPINFOA &si, PROCESS_INFORMATION &pi)
    {
        return ::CreateProcess(NULL, strApp, NULL, NULL, FALSE, dwCreateFlags, NULL, NULL, &si, &pi) != 0;
    }

    /*
     * ���ڴ�
     *      dwProcessPid: ����id
     *      lpBaseAddress������ַ
     *      lpBuffer: ������
     *      dwSize����ȡ�ֽ���
     */
    static bool ReadProcessMemory(DWORD dwProcessPid, PVOID lpBaseAddress, PVOID lpBuffer, DWORD dwSize)
    {
        bool ret = false;
        // �򿪽��̻�ȡ���̾��
        HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessPid);

        if(hProcess && ::ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL)) {  // �����Ч�Ҷ�ȡ�ɹ�
            ::CloseHandle(hProcess);
            ret = true;
        } else {
            hProcess == NULL ? 1 : ::CloseHandle(hProcess); // �������Ч�Ҳ���ʧ�ܣ��رվ��
        }

        return ret;
    }

    /*
     * д�ڴ�
     *      dwProcessPid: ����id
     *      lpBaseAddress������ַ
     *      lpBuffer: ������
     *      dwSize����ȡ�ֽ���
     */
    static bool WriteProcessMemory(DWORD dwProcessPid, PVOID lpBaseAddress, PVOID lpBuffer, DWORD dwSize)
    {
        bool ret = false;
        // �򿪽��̻�ȡ���̾��
        HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessPid);

        if (hProcess && ::WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL)) {  // �����Ч��д��ɹ�
            ::CloseHandle(hProcess);
            ret = true;
        } else {
            hProcess == NULL ? 1 : ::CloseHandle(hProcess); // �������Ч�Ҳ���ʧ�ܣ��رվ��
        }

        return ret;
    }

    /*
     * ��ȡ�߳������Ļ���
     *      dwProcessTid���߳�id
     *      lpContext���߳������Ļ���
     */
    static bool GetThreadContext(DWORD dwProcessTid, CONTEXT &lpContext)
    {
        bool ret = false;
        // ���̻߳�ȡ�߳̾��
        HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, dwProcessTid);

        if(hThread) {   // �����Ч����ȡ�߳������Ļ���
            lpContext.ContextFlags = CONTEXT_ALL;
            if(::GetThreadContext(hThread, &lpContext)) {
                ret = true;
            }
            ::CloseHandle(hThread);
        }
        return ret;
    }

    /*
     * �����߳������Ļ���
     *      dwProcessTid���߳�id
     *      lpContext���߳������Ļ���
     */
    static bool SetThreadContext(DWORD dwProcessTid, CONTEXT &lpContext)
    {
        bool ret = false;
        // ���̻߳�ȡ�߳̾��
        HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, dwProcessTid);

        if (hThread) {   // �����Ч�������߳������Ļ���
            lpContext.ContextFlags = CONTEXT_ALL;
            if (::SetThreadContext(hThread, &lpContext)) {
                ret = true;
            }
            ::CloseHandle(hThread);
        }
        return ret;
    }

    /*
     * ��ѯ�ڴ�����
     *      dwProcessPid������id
     *      lpBaseAddress������ַ
     *      mbi���ڴ���Ϣ
     */
    static bool VirtualQueryEx(DWORD dwProcessPid, PVOID lpBaseAddress, MEMORY_BASIC_INFORMATION &mbi)
    {
        bool ret = false;
        // �򿪽��̻�ȡ���̾��
        HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessPid);

        if (hProcess && ::VirtualQueryEx(hProcess, lpBaseAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION))) {  // �����Ч�Ҳ�ѯ�ɹ�
            ::CloseHandle(hProcess);
            ret = true;
        } else {
            hProcess == NULL ? 1 : ::CloseHandle(hProcess); // �������Ч�Ҳ���ʧ�ܣ��رվ��
        }

        return ret;
    }

    /*
     * �޸��ڴ�����
     *      dwProcessPid������id
     *      lpBaseAddress������ַ
     *      mbi���ڴ���Ϣ
     */
    static bool VirtualProtectEx(DWORD dwProcessPid, PVOID pAddress, DWORD dwSize, DWORD flNewProtect)
    {
        bool ret = false;
        // �򿪽��̻�ȡ���̾��
        HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessPid);
        DWORD flOldProtect; // ԭ������

        if (hProcess && ::VirtualProtectEx(hProcess, pAddress, dwSize, flNewProtect, &flOldProtect)) {  // �����Ч���޸ĳɹ�
            ::CloseHandle(hProcess);
            ret = true;
        } else {
            hProcess == NULL ? 1 : ::CloseHandle(hProcess); // �������Ч�Ҳ���ʧ�ܣ��رվ��
        }

        return ret;
    }

    /*
     * ����ڴ��ַ�Ƿ���Ч
     *      dwProcessPid������id
     *      lpBaseAddress������ַ
     */
    static bool IsAddressValid(DWORD dwProcessPid, PVOID lpBaseAddress)
    {
        MEMORY_BASIC_INFORMATION mbi;
        // ��ѯ�ڴ�����
        VirtualQueryEx(dwProcessPid, lpBaseAddress, mbi);
        return mbi.Protect != PAGE_NOACCESS;
    }
};

