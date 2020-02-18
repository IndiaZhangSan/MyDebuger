#include "MyDebuger.h"
#include "ErrorReport.h"
#include <stdio.h>
#include "ProcessOperating.h"
#include <algorithm>
#include <fstream>

MyDebuger::MyDebuger(std::string strApp)
    : m_strApp(strApp)
{
    m_si.cb = sizeof(STARTUPINFO);

    // ��������
    char buf[MAX_PATH] { 0 };
    strncpy(buf, m_strApp.c_str(), m_strApp.length());
    if (ProcessOperating::CreateProcess(buf, /*DEBUG_PROCESS*/ DEBUG_ONLY_THIS_PROCESS, m_si, m_pi)) {
        // ���̴����ɹ�
        m_IsStart = true;

        // ��ʼ�����������
        ZydisDecoderInit(&m_Decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
    } else {
        // ���̴���ʧ��
        ErrorReport::Report("CreateProcess");
        m_IsStart = false;
    }
}

MyDebuger::~MyDebuger()
{
}

void MyDebuger::Work()
{
    while (m_IsStart) {
        m_dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
        // �ȴ������¼�
        WaitForDebugEvent(&m_DebugEv, INFINITE);

        // �жϵ����¼����룬������
        switch (m_DebugEv.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT: // �쳣
            m_dwContinueStatus = OnDebugExceptionEvent();
            break;

        case CREATE_THREAD_DEBUG_EVENT: // �����߳�
            m_dwContinueStatus = OnCreateThreadEvent();
            break;

        case CREATE_PROCESS_DEBUG_EVENT:    // ��������
            m_dwContinueStatus = OnCreateProcessEvent();
            break;

        case EXIT_THREAD_DEBUG_EVENT:   // �˳��߳�
            m_dwContinueStatus = OnExitThreadEvent();
            break;

        case EXIT_PROCESS_DEBUG_EVENT:  // �˳�����
            m_dwContinueStatus = OnExitProcessEvent();
            break;

        case LOAD_DLL_DEBUG_EVENT:  // ����DLL
            m_dwContinueStatus = OnLoadDllEvent();
            break;

        case UNLOAD_DLL_DEBUG_EVENT:    // ж��DLL
            m_dwContinueStatus = OnUnloadDLLEvent();
            break;

        case OUTPUT_DEBUG_STRING_EVENT: // OutputString���
            // TODO: ������
            break;

        case RIP_EVENT: // RIP
            // TODO: ������
            break;

        default:
            break;
        }

        // ����ִ��
        ContinueDebugEvent(m_DebugEv.dwProcessId, m_DebugEv.dwThreadId, m_dwContinueStatus);
    }
}

DWORD MyDebuger::OnDebugExceptionEvent()
{
    DWORD dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED/*DBG_CONTINUE*/;

    // �ж��쳣���벢����
    switch (m_DebugEv.u.Exception.ExceptionRecord.ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:    // �ڴ�����쳣
        dwContinueStatus = OnExceptionAccessViolation();
        break;

    case EXCEPTION_BREAKPOINT:  // �ϵ��쳣
        dwContinueStatus = OnExceptionBreakPoint();
        break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
        // TODO: ������
        break;

    case EXCEPTION_SINGLE_STEP: // �����쳣
        dwContinueStatus = OnExceptionSingleStep();
        break;

    case DBG_CONTROL_C:
        // TODO: ������
        break;

    default:
        break;
    }

    return dwContinueStatus;
}

DWORD MyDebuger::OnCreateThreadEvent()
{
    DWORD dwContinueStatus = DBG_CONTINUE;

    // ���뵽������
    m_ThreadList.push_back(THREAD_ITEM({ m_DebugEv.u.CreateThread.hThread, m_DebugEv.u.CreateThread.lpThreadLocalBase,
                            m_DebugEv.u.CreateThread.lpStartAddress }));

    return dwContinueStatus;
}

DWORD MyDebuger::OnCreateProcessEvent()
{
    DWORD dwContinueStatus = DBG_CONTINUE;

    // �Ƿ��ǵ�ǰ�����ԵĽ���
    if (m_DebugEv.dwProcessId == m_pi.dwProcessId) {
        printf("\tWelcome to MyDebuger!\n       ���� < ? > �鿴����ҳ��\n\n");

        AppendModule(m_DebugEv.u.CreateProcessInfo.hFile);

        // ������ڵ�Ϊһ���Զϵ�
        AddBP(m_DebugEv.u.CreateProcessInfo.lpStartAddress, true);

        // ��ӡ��Ϣ
        printf("���Գ���%s\n", m_ModuleList.front().ModulePath.c_str());
        printf("��ڵ�ַ: %p  ģ���ַ: %p\n\n", m_DebugEv.u.CreateProcessInfo.lpStartAddress, m_DebugEv.u.CreateProcessInfo.lpBaseOfImage);
    }

    return dwContinueStatus;
}

DWORD MyDebuger::OnExitThreadEvent()
{
    DWORD dwContinueStatus = DBG_CONTINUE;

    // �����߳�����
    auto iter = m_ThreadList.begin();
    while (iter != m_ThreadList.end()) {
        // ����ǰ�˳����̴߳��������Ƴ�
        if (::GetThreadId(iter->hThread) == m_DebugEv.dwThreadId) {
            m_ThreadList.erase(iter);
            break;
        }
        ++iter;
    }

    return dwContinueStatus;
}

DWORD MyDebuger::OnExitProcessEvent()
{
    DWORD dwContinueStatus = DBG_CONTINUE;

    // �Ƿ��ǵ�ǰ�����ԵĽ���
    if (m_DebugEv.dwProcessId == m_pi.dwProcessId) {
        m_IsStart = false;
        ::CloseHandle(m_pi.hProcess);
        ::CloseHandle(m_pi.hThread);
    }
    return dwContinueStatus;
}

DWORD MyDebuger::OnLoadDllEvent()
{
    DWORD dwContinueStatus = DBG_CONTINUE;

    // ��ӵ�������
    AppendModule(m_DebugEv.u.LoadDll.hFile, true);

    return dwContinueStatus;
}

DWORD MyDebuger::OnUnloadDLLEvent()
{
    DWORD dwContinueStatus = DBG_CONTINUE;

    // ����ģ������
    auto iter = m_ModuleList.begin();
    while (iter != m_ModuleList.end()) {
        // ����ǰж�ص�DLL���������Ƴ�
        if (iter->hModule == (HANDLE)m_DebugEv.u.UnloadDll.lpBaseOfDll) {
            m_ModuleList.erase(iter);
            break;
        }
        ++iter;
    }

    return dwContinueStatus;
}

DWORD MyDebuger::OnExceptionBreakPoint()
{
    // ��ȡ��ǰ�쳣�ĵ�ַ
    m_CurrentAddr = (ZyanU64)m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress;

    // �ж��Ƿ���ϵͳ�ϵ�
    if (m_IsSysBP) {
        m_IsSysBP = false;
        // �ж��쳣��ַ�Ƿ�������ڵ�
        if (m_CurrentAddr != (DWORD)m_ModuleList.front().hModule) {
            return DBG_CONTINUE;
        }
    }

    // �˻ص��쳣���ʹ�
    ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context);
    m_Context.Eip = (DWORD)m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress;
    ProcessOperating::SetThreadContext(m_DebugEv.dwThreadId, m_Context);

    // ��ʾ�Ĵ���
    ShowRegister();

    // ��ȡ�õ�ַ���Ĵ���ָ������
    ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData));

    // �������ʾһ��
    ShowDisassembly(m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData), 1);

    // �����������ҵ�ǰ�Ķϵ�ڵ�
    auto iter = m_BPList.begin();
    while (iter != m_BPList.end()) {
        if (iter->BPAddr == m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress) {
            // ��ԭָ��
            ProcessOperating::WriteProcessMemory(m_DebugEv.dwProcessId, iter->BPAddr, &iter->OldCode, sizeof(iter->OldCode));

            // �ж��Ƿ���һ���Զϵ�
            if (iter->IsOnce) {
                // ɾ���ڵ�
                iter = m_BPList.erase(iter);
                continue;
            } else {
                // ���뵽��ԭ������
                m_BPReductionList.push_back(*iter);
            }
        }
        ++iter;
    }

    // ����Զ�׷���Ƿ���
    if (m_IsAutoTrack) {
        // �������Ƿ�δ����
        if (!m_IsTrack) {
            // ����쳣��ַ�Ƿ��Ǹ��ٵ���ʼ��ַ
            if (m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress == m_AutoTrackParam.StartAddr) {
                // ��ʼ����
                m_IsTrack = true;
                m_IsRun = false;    // ����g����
                SetSingleStep();    // ���õ���

                //��ȡ�õ�ַ�Ļ������
                ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData));

                //��ʾ��һ�д����쳣�Ļ��
                ShowDisassembly(m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData), 1);

                return DBG_CONTINUE;
            }
        } else {
            // ֹͣ����
            m_IsTrack = false;
            m_IsAutoTrack = false;

            printf("�����ϵ㣬����ֹͣ��Dump�ļ���%p-%p.txt\n", m_AutoTrackParam.StartAddr, m_AutoTrackParam.EndAddr);
            char buf[MAX_PATH] { 0 };
            sprintf(buf, "%p-%p.txt", m_AutoTrackParam.StartAddr, m_AutoTrackParam.EndAddr);
            // dump��Ϣ
            DumpTrackInfo(buf);
        }
    }

    return GetCmdLine();
}

DWORD MyDebuger::OnExceptionAccessViolation()
{
    MEMORY_BASIC_INFORMATION mbi;
    // ��ѯ�ڴ�����
    ProcessOperating::VirtualQueryEx(m_DebugEv.dwProcessId, (PVOID)m_DebugEv.u.Exception.ExceptionRecord.ExceptionInformation[1], mbi);

    unsigned int PageIndex;//��ҳ����

    // ����Ƿ���ڷ�ҳ(�Ƿ����Լ��ϵ�ķ�ҳ)
    if (IsExistsMemPage(mbi.BaseAddress, &PageIndex)) {
        unsigned int BPIndex;//�ϵ�����

        // ����Ƿ���ڶϵ�(�Ƿ����Լ��µĶϵ�)
        if (IsExistMemBP((PVOID)m_DebugEv.u.Exception.ExceptionRecord.ExceptionInformation[1], &BPIndex)) {
            // ��鴥���ϵ������Ƿ�һ��
            if (m_MemBPList[BPIndex].MemBPType == m_DebugEv.u.Exception.ExceptionRecord.ExceptionInformation[0]) {
               

                switch (m_MemBPList[BPIndex].MemBPType) {
                case TYPE_MEMBP::MEMBP_READ:    // ��
                    printf("�ڴ���ʣ�%p\n", (PVOID)m_DebugEv.u.Exception.ExceptionRecord.ExceptionInformation[1]);
                    break;
                case TYPE_MEMBP::MEMBP_WRITE:   // д
                    printf("�ڴ�д�룺%p\n", (PVOID)m_DebugEv.u.Exception.ExceptionRecord.ExceptionInformation[1]);
                    break;
                }

                // ��ʾ�Ĵ���
                ShowRegister();

                // ��ȡ�õ�ַ�Ļ������
                ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId,
                    m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress, m_MemoryData, sizeof(m_MemoryData));

                // ��ʾ��һ�д����쳣�Ļ��
                ShowDisassembly((ZyanU64)m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress, m_MemoryData, sizeof(m_MemoryData), 1);

                // ��ԭԭʼ�ڴ����Ա���
                if (ProcessOperating::VirtualProtectEx(m_DebugEv.dwProcessId, m_MemBPPageList[PageIndex].StartAddr,
                    m_MemBPPageList[PageIndex].dwSize, m_MemBPPageList[PageIndex].dwOldProtect)) {
                    //��ӻ�ԭ�ڴ��ҳ����
                    m_MemBPPageReductionList.push_back(m_MemBPPageList[PageIndex]);
                } else {
                    printf("�ڴ�ϵ㣬��ԭԭʼ�ڴ����Ա���ʧ��\n");
                }

                return GetCmdLine();
            }
        }

        // ��ԭԭʼ�ڴ����Ա���
        if (ProcessOperating::VirtualProtectEx(m_DebugEv.dwProcessId, m_MemBPPageList[PageIndex].StartAddr,
            m_MemBPPageList[PageIndex].dwSize, m_MemBPPageList[PageIndex].dwOldProtect)) {
            // ��ӻ�ԭ�ڴ��ҳ����
            m_MemBPPageReductionList.push_back(m_MemBPPageList[PageIndex]);

            // ���õ���(���ڻ�ԭ�ڴ�ϵ�)
            SetSingleStep();
            m_IsRun = true;//����g��������
            return DBG_CONTINUE;
        } else {
            printf("�ڴ�ϵ㣬��ԭԭʼ�ڴ����Ա���ʧ��\n");
            return GetCmdLine();
        }
    }
    printf("δ֪�쳣\n");

    // ��ȡ�õ�ַ�Ļ������
    ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress,
        m_MemoryData, sizeof(m_MemoryData));

    // ��ʾ��һ�д����쳣�Ļ��
    ShowDisassembly((ZyanU64)m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress, m_MemoryData, sizeof(m_MemoryData), 1);
    return GetCmdLine();
}

DWORD MyDebuger::OnExceptionSingleStep()
{
    // ��ȡ�쳣��ַ
    m_CurrentAddr = (ZyanU64)m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress;

    // ��ԭ�ϵ㡢Ӳ���ϵ㡢�ڴ�ϵ�
    if (!m_BPReductionList.empty() || !m_HBPReductionList.empty() || !m_MemBPPageReductionList.empty()) {
        // ��ԭ�ϵ�
        for (auto &item : m_BPReductionList) {
            ProcessOperating::WriteProcessMemory(m_DebugEv.dwProcessId, item.BPAddr, &item.NEwCode, sizeof(item.NEwCode));
        }
        m_BPReductionList.clear();

        // ��ԭӲ���ϵ�
        for (auto &item : m_HBPReductionList) {
            // ��ȡ�̻߳���������
            if (ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
                DR7 Dr7;
                Dr7.Dr7 = m_Context.Dr7;
                // �������öϵ� ���ݼĴ���ID
                switch (item.DrId) {
                case 0:                                     // Dr0
                    m_Context.Dr0 = (DWORD)item.HBPAddr;    // ���õ�ַ
                    Dr7.L0 = 1;                             // ����ȫ��
                    Dr7.RW0 = item.HBPType;                 // ��������
                    Dr7.LEN0 = item.HBPLen;                 // ���ó���
                    break;
                case 1:                                     // Dr1                                   
                    m_Context.Dr1 = (DWORD)item.HBPAddr;    // ���õ�ַ
                    Dr7.L1 = 1;                             // ����ȫ��
                    Dr7.RW1 = item.HBPType;                 // ��������
                    Dr7.LEN1 = item.HBPLen;                 // ���ó���
                    break;
                case 2:                                     // Dr2                                   
                    m_Context.Dr2 = (DWORD)item.HBPAddr;    // ���õ�ַ
                    Dr7.L2 = 1;                             // ����ȫ��
                    Dr7.RW2 = item.HBPType;                 // ��������
                    Dr7.LEN2 = item.HBPLen;                 // ���ó���
                    break;
                case 3:                                     // Dr3                                   
                    m_Context.Dr3 = (DWORD)item.HBPAddr;    // ���õ�ַ
                    Dr7.L3 = 1;                             // ����ȫ��
                    Dr7.RW3 = item.HBPType;                 // ��������
                    Dr7.LEN3 = item.HBPLen;                 // ���ó���
                    break;
                }

                m_Context.Dr7 = Dr7.Dr7;

                // �����߳������Ļ���
                ProcessOperating::SetThreadContext(m_DebugEv.dwThreadId, m_Context);
            }
        }
        m_HBPReductionList.clear();

        // ��ԭ�ڴ�ϵ�
        for (auto &item : m_MemBPPageReductionList) {
            ProcessOperating::VirtualProtectEx(m_DebugEv.dwProcessId, item.StartAddr, item.dwSize, item.dwNewProtect);
        }
        m_MemBPPageReductionList.clear();
    }

    if (OnExceptionHardwareBreakPoint()) {
        m_IsRun = false;    // ����g����
    }

    if (!m_IsRun) {
        //��ʾ�Ĵ���
        ShowRegister();

        //��ȡ�õ�ַ�Ļ������
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData));

        //��ʾ��һ�д����쳣�Ļ��
        ShowDisassembly(m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData), 1);
    }


    // ����Զ������Ƿ���
    if (m_IsAutoTrack) {
        // �����Ƿ���
        if (m_IsTrack) {
            // ����쳣��ַ���Ǹ��ٵĽ�����ַ����������
            if (m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress != m_AutoTrackParam.EndAddr) {
                SetSingleStep();
                return DBG_CONTINUE;
            } else {
                // ֹͣ����
                m_IsTrack = false;
                m_IsAutoTrack = false;
                printf("������ɣ�Dump�ļ���%p-%p.txt\n", m_AutoTrackParam.StartAddr, m_AutoTrackParam.EndAddr);
                char buf[MAX_PATH] { 0 };
                sprintf(buf, "%p-%p.txt", m_AutoTrackParam.StartAddr, m_AutoTrackParam.EndAddr);
                // dump��Ϣ
                DumpTrackInfo(buf);
            }
        }
    }


    // ����Ƿ�g����
    if (m_IsRun) {
        m_IsRun = false;
        return DBG_CONTINUE;
    }

    return GetCmdLine();
}

DWORD MyDebuger::OnExceptionHardwareBreakPoint()
{
    // Ӳ���ϵ㲻Ϊ��
    if (!m_HBPList.empty()) {
        for (auto &item : m_HBPList) {
            // ����Ƿ���Ӳ���ϵ�
            if (item.HBPAddr == m_DebugEv.u.Exception.ExceptionRecord.ExceptionAddress) {
                printf("Ӳ���ϵ㣺%p\n", item.HBPAddr);
                // ��ȡ�߳������Ļ���
                if (ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
                    DR7 Dr7;
                    Dr7.Dr7 = m_Context.Dr7;

                    // ��ԭӲ���ϵ�
                    switch (item.DrId) {
                    case 0:                 // DR0
                        m_Context.Dr0 = 0;  // ��յ�ַ
                        Dr7.L0 = 0;         // ���ȫ�ֱ�־
                        Dr7.RW0 = 0;        // �������
                        Dr7.LEN0 = 0;       // ��ճ���
                        break;
                    case 1:                 // DR1
                        m_Context.Dr1 = 0;  // ��յ�ַ
                        Dr7.L1 = 0;         // ���ȫ�ֱ�־
                        Dr7.RW1 = 0;        // �������
                        Dr7.LEN1 = 0;       // ��ճ���
                        break;
                    case 2:                 // DR2
                        m_Context.Dr2 = 0;  // ��յ�ַ
                        Dr7.L2 = 0;         // ���ȫ�ֱ�־
                        Dr7.RW2 = 0;        // �������
                        Dr7.LEN2 = 0;       // ��ճ���
                        break;
                    case 3:                 // DR3
                        m_Context.Dr3 = 0;  // ��յ�ַ
                        Dr7.L3 = 0;         // ���ȫ�ֱ�־
                        Dr7.RW3 = 0;        // �������
                        Dr7.LEN3 = 0;       // ��ճ���
                        break;
                    }

                    m_Context.Dr7 = Dr7.Dr7;

                    // �����߳������Ļ���
                    if (ProcessOperating::SetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
                        m_HBPReductionList.push_back(item);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

DWORD MyDebuger::GetCmdLine()
{
    DWORD dwContinueStatus = DBG_CONTINUE;
    char buf[CHAR_MAX] { 0 };

    while (true) {
        std::string cmd;
        if (m_CmdLineQueue.empty()) {
            // �������Ϊ�գ���ȡ�û�����
            printf("MyDebuger$ ");
            fgets(buf, sizeof(buf), stdin);
            // ȫ��ת��ΪСд
            _strlwr(buf);
            cmd = buf;
        } else {
            // �����������������
            cmd = m_CmdLineQueue.front();
            m_CmdLineQueue.pop();
            strncpy(buf, cmd.c_str(), cmd.length());
        }

        // ��ʼ��������
        if (strncmp(buf, "?", 1) == 0) {
            // �鿴����
            ShowHelp();
        } else if (strncmp(buf, "u", 1) == 0) {
            // �鿴����� u [address]
            OnCmdU(buf);
        } else if (strncmp(buf, "dd", 2) == 0) {
            // �鿴�ڴ� dd [address]
            OnCmdDD(buf);
        } else if (strncmp(buf, "r", 1) == 0) {
            // ��ʾ�Ĵ���
            ShowRegister();
        } else if (strncmp(buf, "ml", 2) == 0) {
            // ��ʾģ��
            ShowModule();
        } else if (strncmp(buf, "q", 1) == 0) {
            // �˳�
            QuitDebug();
        } else if (strncmp(buf, "ls", 2) == 0) {
            // ����ű�
            if (OnCmdLS(buf)) {
                continue;
            }
        } else if (strncmp(buf, "es", 2) == 0) {
            // �����ű�
            ExportScript();
        } else if (strncmp(buf, "e", 1) == 0) {
            // �޸��ڴ�����
            if (OnCmdE(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "bpl", 3) == 0) {
            // ��ʾ�ϵ��б�
            ShowBPList();
        } else if (strncmp(buf, "bpc", 3) == 0) {
            // ɾ���ϵ�
            if (OnCmdBPC(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "bp", 2) == 0) {
            // �¶ϵ�
            if (OnCmdBP(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "bml", 3) == 0) {
            // ��ʾ�ڴ�ϵ��б�
            ShowMemBPList();
        } else if (strncmp(buf, "bmpl", 4) == 0) {
            // ��ʾ��ҳ�ڴ�ϵ��б�
            ShowMemBPList(true);
        } else if (strncmp(buf, "bmc", 3) == 0) {
            // ɾ���ڴ�ϵ�
            if (OnCmdBMC(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "bm", 2) == 0) {
            // ���ڴ�ϵ�
            if (OnCmdBM(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "bhl", 3) == 0) {
            // ��ʾӲ���ϵ��б�
            ShowHBPList();
        } else if (strncmp(buf, "bhc", 3) == 0) {
            // ɾ��Ӳ���ϵ�
            if (OnCmdBHC(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "bh", 2) == 0) {
            // ��Ӳ���ϵ�
            if (OnCmdBH(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
            }
        } else if (strncmp(buf, "g", 1) == 0) {
            // g����
            if (OnCmdG(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
                break;
            }
        } else if (strncmp(buf, "trace", 5) == 0) {
            // ����
            OnCmdTRACE(buf);
        } else if (strncmp(buf, "t", 1) == 0) {
            // ��������
            if (OnCmdT(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
                break;
            }
        } else if (strncmp(buf, "p", 1) == 0) {
            // ��������
            if (OnCmdP(buf)) {
                // ��¼����
                m_CmdRecordList.push_back(cmd);
                break;
            }
        } else if(strncmp(buf, "dump", 4) == 0) {
            // dump�ڴ�
            if(!OnCmdDUMP(buf)) {
                printf("Dumpʧ��\n");
            }
        } else {  // û������ƥ��
            printf("�������������< ? >�鿴����\n");
        }
    }

    return dwContinueStatus;
}

bool MyDebuger::AddBP(void *BPAddr, bool IsOnecBP)
{
    int ret = false;
    // �ϵ��Ƿ����
    if (IsExistsBP(BPAddr)) {    
        printf("��ַ: %p, �ϵ��Ѿ�����\n", BPAddr);
        return ret;
    }

    if (ProcessOperating::IsAddressValid(m_DebugEv.dwProcessId, BPAddr)) {   // ����ַ�Ƿ���Ч
        // ��������ϵ�
        if(IsExistsOtherBreakPoint(BPAddr)) {
            printf("��ַ��%p�����������ϵ�\n", BPAddr);
            return ret;
        }

        // ��ԭʼ�Ĵ���
        BYTE OldCode;   // ԭʼ����
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, BPAddr, &OldCode, sizeof(BYTE));

        // д��ϵ�
        BYTE NewCode = 0xcc;   // �´���
        if (ProcessOperating::WriteProcessMemory(m_DebugEv.dwProcessId, BPAddr, &NewCode, sizeof(BYTE))) {
            // ����������
            m_BPList.push_back(BP_ITEM({ BPAddr, OldCode, NewCode, IsOnecBP }));
            ret = true;
        } else {
            printf("��ַ: %p, �ϵ�ʧ��\n", BPAddr);
        }
    } else {
        printf("��ַ: %p, �ϵ�ʧ�ܣ�������Ч��ַ\n", BPAddr);
    }
    return ret;
}

bool MyDebuger::DelBP(unsigned number)
{
    if (m_BPList.empty() || number >= m_BPList.size()) {
        return false;   // ����Ϊ�ջ��߱�Ŵ�������ĳ���
    }
    // ��ԭ����
    ProcessOperating::WriteProcessMemory(m_DebugEv.dwProcessId, m_BPList[number].BPAddr,
        &m_BPList[number].OldCode, sizeof(m_BPList[number].OldCode));
    // ɾ���ڵ�
    m_BPList.erase(m_BPList.begin() + number);
    return true;
}

bool MyDebuger::AddMemBP(void *BPAddr, unsigned BPSize, TYPE_MEMBP BPType)
{
    // ��������ϵ�
    if (IsExistsOtherBreakPoint(BPAddr)) {
        printf("��ַ��%p�����������ϵ�\n", BPAddr);
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi;
    // ��ѯ�ڴ�
    if (!ProcessOperating::VirtualQueryEx(m_DebugEv.dwProcessId, BPAddr, mbi)) {
        printf("��ַ��%p������ڴ�ϵ�ʧ��\n", BPAddr);
        return false;
    }

    // ����Ƿ��Խ��ҳ
    SYSTEM_INFO SysInfo;
    ::GetSystemInfo(&SysInfo);
    if ((DWORD64)mbi.BaseAddress + SysInfo.dwPageSize < (DWORD64)BPAddr + BPSize) {
        printf("�ݲ�֧�ֿ��ҳ�ϵ�\n");
        return false;
    }

    if (!IsExistsMemPage(mbi.BaseAddress)) {
        // �����ҳ�ϵ㲻����
        if (!ProcessOperating::VirtualProtectEx(m_DebugEv.dwProcessId, mbi.BaseAddress, SysInfo.dwPageSize, PAGE_NOACCESS)) {
            printf("��ַ��%p������ڴ�ϵ�ʧ��\n", BPAddr);
            return false;
        }

        // ��������
        m_MemBPPageList.push_back(MEM_PAGE_ITEM({ mbi.BaseAddress, SysInfo.dwPageSize, mbi.Protect, PAGE_NOACCESS }));
    }

    // ������öϵ��Ƿ����
    if (IsExistMemBP(BPAddr)) {
        printf("��ַ��%p���Ѿ����ڴ��ڴ�ϵ�", BPAddr);
        return false;
    }

    // ��������
    m_MemBPList.push_back(MEMBP_ITEM({ BPAddr, BPSize, BPType }));
    return true;
}

bool MyDebuger::IsExistsMemPage(void *PageAddr, unsigned *Index)
{
    // ������ҳ���Ƿ���������ַ
    for (size_t i = 0; i < m_MemBPPageList.size(); i++) {
        if (m_MemBPPageList[i].StartAddr == PageAddr) {
            //��鷵������IDָ���Ƿ���Ч
            if (Index != nullptr) {
                *Index = i; // ��������ID
            }
            return true;    // ���ڷ�ҳ��ַ
        }
    }
    return false;   // �����ڷ�ҳ��ַ
}

bool MyDebuger::IsExistMemBP(void *PageAddr, unsigned *Index)
{
    // ��������Χ���Ƿ��¹��ϵ㣬���ڷ�Χ�ڵļ��ʧ��

    // �����ڴ�ϵ��
    for (size_t i = 0; i < m_MemBPList.size(); i++) {
        //�õ���ʱ������ַ
        DWORD64 dwEndAddress = (DWORD64)m_MemBPList[i].MemBPAddr + m_MemBPList[i].dwSize;

        if ((DWORD64)PageAddr < dwEndAddress && (DWORD64)PageAddr >= (DWORD64)m_MemBPList[i].MemBPAddr) {
            //��鷵������ָ���Ƿ���Ч
            if (Index != nullptr) {
                *Index = i;
            }
            return true; //�����Χ���Ѿ����������ϵ�
        }
    }
    return false;
}

bool MyDebuger::DelMemBP(unsigned number)
{
    if (m_MemBPList.empty() || number >= m_MemBPList.size()) {
        return false;   // ����Ϊ�ջ��߱�Ŵ�������ĳ���
    }
    if (m_MemBPPageList.empty() || number >= m_MemBPPageList.size()) {
        return false;
    }
    // ɾ���ڵ�
    m_MemBPList.erase(m_MemBPList.begin() + number);

    // ����ҳ�������û�ȥ
    ProcessOperating::VirtualProtectEx(m_DebugEv.dwProcessId, m_MemBPPageList[number].StartAddr,
        m_MemBPPageList[number].dwSize, m_MemBPPageList[number].dwOldProtect);
    m_MemBPPageList.erase(m_MemBPPageList.begin() + number);
    return true;
}

bool MyDebuger::AddHBP(void *BPAddr, TYPE_HBP HBPType, LEN_HBP HBPSize)
{
    DR7 Dr7 { 0 };
    if (m_HBPList.size() >= 4) {
        // �ϵ���
        printf("Ӳ���ϵ㲻�ܳ���4��\n");
        return false;
    }

    // ��������ϵ�
    if (IsExistsOtherBreakPoint(BPAddr)) {
        printf("��ַ��%p�����������ϵ�\n", BPAddr);
        return false;
    }

    // ��ȡ�߳�������
    if (!ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
        return false;
    }

    Dr7.Dr7 = m_Context.Dr7;
    DWORD DrId = 0;//Ӳ���ϵ�ID

    //���öϵ��ַ
    if (m_Context.Dr0 == 0) {
        m_Context.Dr0 = (DWORD)BPAddr;  //���õ�ַ
        DrId = 0;                       //���üĴ���ID
        Dr7.L0 = 1;                     //����ȫ��
        Dr7.RW0 = HBPType;              //��������
        Dr7.LEN0 = HBPSize;             //���ó���
    } else if (m_Context.Dr1 == 0) {
        m_Context.Dr1 = (DWORD)BPAddr;  //���õ�ַ
        DrId = 1;                       //���üĴ���ID
        Dr7.L1 = 1;                     //����ȫ��
        Dr7.RW1 = HBPType;              //��������
        Dr7.LEN1 = HBPSize;             //���ó���
    } else if (m_Context.Dr2 == 0) {
        m_Context.Dr2 = (DWORD)BPAddr;  //���õ�ַ
        DrId = 2;                       //���üĴ���ID
        Dr7.L2 = 1;                     //����ȫ��
        Dr7.RW2 = HBPType;              //��������
        Dr7.LEN2 = HBPSize;             //���ó���
    } else if (m_Context.Dr3 == 0) {
        m_Context.Dr3 = (DWORD)BPAddr;  //���õ�ַ
        Dr7.L3 = 1;                     //����ȫ��
        DrId = 3;                       //���üĴ���ID
        Dr7.RW3 = HBPType;              //��������
        Dr7.LEN3 = HBPSize;             //���ó���
    } else {
        printf("Ӳ���ϵ�ʧ�ܣ��Ĵ�������\n");
        return false;
    }

    // ����Dr7�Ĵ���
    m_Context.Dr7 = Dr7.Dr7;
    if (!ProcessOperating::SetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
        printf("Ӳ���ϵ�ʧ��\n");
        return false;
    }
    m_HBPList.push_back(HBP_ITEM({ DrId, BPAddr, HBPType, HBPSize }));
    return true;
}

bool MyDebuger::DelHBP(unsigned number)
{
    if (m_HBPList.empty() || number >= m_HBPList.size()) {
        return false;   // ����Ϊ�ջ��߱�Ŵ�������ĳ���
    }

    DR7 Dr7;
    // ��ȡ�߳������Ļ���
    if (!ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
        return false;
    }

    Dr7.Dr7 = m_Context.Dr7;
    // ��������е�ַ��Ӧ�ĸ��Ĵ���
    if (m_HBPList[number].HBPAddr == (PVOID)m_Context.Dr0) {
        m_Context.Dr0 = 0;      // ��յ�ַ
        Dr7.L0 = 0;             // ȡ��ȫ��
        Dr7.RW0 = 0;            // ȥ������
        Dr7.LEN0 = 0;           // ȡ������

    } else if (m_HBPList[number].HBPAddr == (PVOID)m_Context.Dr1) {
        m_Context.Dr1 = 0;      // ��յ�ַ
        Dr7.L1 = 0;             // ȡ��ȫ��
        Dr7.RW1 = 0;            // ȥ������
        Dr7.LEN1 = 0;           // ȡ������
    } else if (m_HBPList[number].HBPAddr == (PVOID)m_Context.Dr2) {
        m_Context.Dr2 = 0;      // ��յ�ַ
        Dr7.L2 = 0;             // ȡ��ȫ��
        Dr7.RW2 = 0;            // ȥ������
        Dr7.LEN2 = 0;           // ȡ������
    } else if (m_HBPList[number].HBPAddr == (PVOID)m_Context.Dr3) {
        m_Context.Dr3 = 0;      // ��յ�ַ
        Dr7.L3 = 0;             // ȡ��ȫ��
        Dr7.RW3 = 0;            // ȥ������
        Dr7.LEN3 = 0;           // ȡ������
    }

    m_Context.Dr7 = Dr7.Dr7;

    // �����߳������Ļ���
    if (!ProcessOperating::SetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
        printf("ɾ��Ӳ���ϵ�ʧ��\n");
        return false;
    }
    m_HBPList.erase(m_HBPList.begin() + number);

    return true;
}

bool MyDebuger::IsExistsBP(void *BPAddr)
{
    for (auto &item : m_BPList) {
        if (item.BPAddr == BPAddr) {
            // �ϵ��Ѿ�����
            return true;
        }
    }
    return false;
}

void MyDebuger::AppendModule(HANDLE hFile, bool IsLoadDLl)
{
    IMAGE_DOS_HEADER ImageDosHeader;
    IMAGE_NT_HEADERS32 ImageNtHeaders32;
    char buf[MAX_PATH] { 0 };
    // ��ȡ��ģ��·��
    ::GetFinalPathNameByHandleA(hFile, buf, sizeof(buf), VOLUME_NAME_DOS);
    std::string path = (char *)buf + 4; // ���˿�ͷ�� \\?\

    // ��ȡģ����
    std::string name = path.substr(path.find_last_of("\\") + 1);

    if (IsLoadDLl) {
        // ��DOSͷ
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, m_DebugEv.u.LoadDll.lpBaseOfDll,
            &ImageDosHeader, sizeof(ImageDosHeader));
        // ��Ntͷ
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PBYTE)m_DebugEv.u.LoadDll.lpBaseOfDll + ImageDosHeader.e_lfanew,
            &ImageNtHeaders32, sizeof(ImageNtHeaders32));
        // ����ģ����Ϣ
        m_ModuleList.push_back(MODULE_ITEM({ m_DebugEv.u.LoadDll.lpBaseOfDll,
            ImageNtHeaders32.OptionalHeader.SizeOfImage, name, path }));
    } else {
        // ��DOSͷ
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, m_DebugEv.u.CreateProcessInfo.lpBaseOfImage,
            &ImageDosHeader, sizeof(ImageDosHeader));
        // ��Ntͷ
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PBYTE)m_DebugEv.u.CreateProcessInfo.lpBaseOfImage + ImageDosHeader.e_lfanew,
            &ImageNtHeaders32, sizeof(ImageNtHeaders32));
        // ����ģ����Ϣ
        m_ModuleList.push_back(MODULE_ITEM({ m_DebugEv.u.CreateProcessInfo.lpBaseOfImage,
            ImageNtHeaders32.OptionalHeader.SizeOfImage, name, path }));
    }
}

bool MyDebuger::IsExistsModule(std::string ModuleName, unsigned *Index)
{
    for (unsigned int i = 0; i < m_ModuleList.size(); i++) {
        std::string tmp = m_ModuleList[i].ModuleName;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), tolower);
        if (ModuleName == tmp) {
            *Index = i;
            return true;
        }
    }
    return false;
}

ZyanUSize MyDebuger::ShowDisassembly(ZyanU64 runtime_address, ZyanU8 *data, ZyanUSize length, DWORD dwLine,
    bool bIsShow)
{
    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    ZydisDecodedInstruction instruction;
    ZyanUSize ZuSize = length;

    char buffer[256];
    char szBuf[256];    //��ʽ��������

    // ������ʾi��
    for (DWORD i = 0; i < dwLine; i++) {
        std::string record; //�Զ����ټ�¼

        // ���Ҳ���ԭ�ϵ㴦��ָ��
        for (auto &item : m_BPList) {
            if (item.BPAddr == (PVOID)runtime_address) {
                // ��ԭָ��
                BYTE *ptr = (BYTE *)data;
                *ptr = item.OldCode;
            }
        }

        // �����
        if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&m_Decoder, data, length, &instruction))) {
            if (bIsShow) {
                printf("%p  ", (PVOID)runtime_address); // ��ӡ��ַ
            }

            ZydisFormatterFormatInstruction(&formatter, &instruction, &buffer[0], sizeof(buffer),
                runtime_address);

            // ����ָ��
            for (int i = 0; i < 8; i++) {
                if (i < instruction.length) {
                    if (bIsShow) {
                        printf("%02X ", data[i]);   // ��ӡ������
                    }

                    // �Ƿ����
                    if (m_IsTrack)
                    {
                        sprintf(szBuf, "%02X ", data[i]);
                        record += szBuf;//��¼�ַ�
                    }
                } else {
                    if (bIsShow) {
                        printf("   ");
                    }

                    //�Ƿ����
                    if (m_IsTrack)
                    {
                        sprintf(szBuf, "   ");
                        record += szBuf;//��¼�ַ�
                    }
                }
            }

            if (bIsShow) {
                ZYAN_PRINTF("%s\n", &buffer[0]);    // ��ӡָ��
            }

            // �Ƿ����
            if (m_IsTrack)
            {
                sprintf(szBuf, "%s\n", &buffer[0]);
                record += szBuf;//��¼�ַ�

                //��Ӹ�����Ϣ
                AppendTrackData((PVOID)runtime_address, record);
            }

            data += instruction.length;
            length -= instruction.length;
            runtime_address += instruction.length;
        }
    }
    return ZuSize - length;//������ʾ��С
}

ZyanUSize MyDebuger::ShowHex(ZyanU64 ZuAddress, PBYTE pBuf, DWORD dwSize, DWORD dwLine)
{
    size_t nPos = 0;//�ƶ���
    size_t nRemaining = 0;//ʣ��
    size_t nSize = 0;//��С
    size_t nLine = 0;//����
    //���Ҫ��ʾ�Ĵ�С�Ƿ����16
    if (dwSize >= 16)
    {
        nSize = 16;
    } else
    {
        nSize = dwSize;
    }

    //����Ƿ��ƶ������Ҫ��ʾ������
    while (nPos < dwSize && nLine < dwLine)
    {
        printf("%p  ", (PVOID)ZuAddress);
        //��ʾʮ����ֹ����
        for (size_t i = 0; i < nSize; i++)
        {
            if (i == 8)
            {
                printf("- ");
            }

            printf("%.2X ", pBuf[nPos + i]);
        }

        //��ʾASCII
        for (size_t i = 0; i < nSize; i++)
        {
            //������ʾ�ɼ�
            if ((pBuf[nPos + i] >= 32 && pBuf[nPos + i] < 128))
            {
                printf("%c", pBuf[nPos + i]);
            } else
            {
                printf(".");
            }
        }

        nPos += nSize;//�ƶ�����ʾ�Ĵ�С
        nRemaining = dwSize - nPos;//�õ�ʣ��

        //���ʣ�������Ƿ����16
        if (nRemaining >= 16)
        {
            nSize = 16;
        } else
        {
            nSize = nRemaining;
        }
        ZuAddress += 16;//��ַ+16
        nLine++;//����+1

        printf("\r\n");

    }

    return nLine * 16;
}

bool MyDebuger::OnCmdU(char *buf)
{
    int ret = false;
    char *s = strtok(buf, " "); // �Կո��и�
    s = strtok(nullptr, "\n"); // �е��س�����
    if (s != nullptr) {
        // ȡ�õ�ַ
        m_CurrentAddr = strtoul(s, nullptr, 16);    // ��16����
    }

    // ����ַ�Ƿ���Ч
    if (ProcessOperating::IsAddressValid(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr)) {
        // ��ȡ���ָ������
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData));
        // ��ʾ
        m_CurrentAddr += ShowDisassembly(m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData), 10);
        ret = true;
    } else {
        printf("��ַ��%p����Ч�ĵ�ַ\n", (PVOID)m_CurrentAddr);
    }
    return ret;
}

bool MyDebuger::OnCmdDD(char *buf)
{
    int ret = false;
    char *s = strtok(buf, " "); // �Կո��и�
    s = strtok(nullptr, "\n"); // �е��س�����
    if (s != nullptr) {
        // ȡ�õ�ַ
        m_CurrentAddr = strtoul(s, nullptr, 16);    // ��16����
    }

    // ����ַ�Ƿ���Ч
    if (ProcessOperating::IsAddressValid(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr)) {
        // ��ȡ����
        ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData));
        // ��ʾ
        m_CurrentAddr += ShowHex(m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData), 5);
        ret = true;
    } else {
        printf("��ַ��%p����Ч�ĵ�ַ\n", (PVOID)m_CurrentAddr);
    }
    return ret;
}

bool MyDebuger::OnCmdE(char *buf)
{
    int ret = true;
    char *s = strtok(buf, " "); // �Կո��и�
    s = strtok(nullptr, " ");

    if (s != nullptr) {
        //�õ��ڴ��ַ
        void *Addr = (void *)strtoul(s, nullptr, 16);
        s = strtok(nullptr, "\n");  // �е��س�����

        if (s != nullptr) {
            //����Ƿ��������3���ַ�
            if (strlen(s) < 3) {
                BYTE Data = (BYTE)strtoul(s, nullptr, 16);

                // ����ַ�Ƿ���Ч������޸��ڴ����Ա���
                if (ProcessOperating::IsAddressValid(m_DebugEv.dwProcessId, Addr)
                    && ProcessOperating::VirtualProtectEx(m_DebugEv.dwProcessId, Addr, 1, PAGE_EXECUTE_READWRITE)) {
                    if (ProcessOperating::WriteProcessMemory(m_DebugEv.dwProcessId, Addr, &Data, sizeof(Data))) {
                        ret = true;
                    }
                } else {
                    printf("��ַ��%p���޸�ʧ�ܣ���Ч�ڴ��ַ\n", Addr);
                    ret = false;
                }
            } else {
                printf("�޸�ʧ�ܣ�ֻ���޸�1���ֽ�\n");
                ret = false;
            }
        } else {
            printf("�޸�ʧ�ܣ���ʽ����\n");
            ret = false;
        }
    }
    return ret;
}

bool MyDebuger::OnCmdBP(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, " ");
    if (s != nullptr) {
        void *Addr = (void *)strtoul(s, nullptr, 16);
        s = strtok(nullptr, "\n");

        if (s != nullptr) {
            if (strncmp(s, "sys", 3) == 0) {
                //���һ���Զϵ�
                return AddBP(Addr, true);
            } else {
                printf("��Ӷϵ�ʧ��,δ֪��ʽ\n");
                return false;
            }
        } else {
            //��ӷ�һ���Զϵ�
            return AddBP(Addr, false);
        }
    }
    return false;
}

bool MyDebuger::OnCmdBPC(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, "\n");
    if (s != nullptr) {
        unsigned int index = strtoul(s, nullptr, 10);
        // ɾ���ϵ�
        return DelBP(index);
    } else {
        printf("ɾ��ʧ�ܣ������ʽ����\n");
    }
    return false;
}

bool MyDebuger::OnCmdBMC(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, "\n");
    if (s != nullptr) {
        unsigned int index = strtoul(s, nullptr, 10);
        // ɾ���ϵ�
        return DelMemBP(index);
    } else {
        printf("ɾ��ʧ�ܣ���ʽ����\n");
    }
    return false;
}

bool MyDebuger::OnCmdBM(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, " ");
    if (s != nullptr)
    {
        //�õ��ڴ��ַ
        void *Addr = (void *)strtoul(s, nullptr, 16);

        s = strtok(nullptr, " ");
        if (s != nullptr) {
            //�õ�����
            size_t len = strtoul(s, nullptr, 10);
            s = strtok(nullptr, " ");
            if (s != nullptr) {
                //����ַ�Ƿ���Ч
                if (ProcessOperating::IsAddressValid(m_DebugEv.dwProcessId, Addr)) {
                    if (!strncmp(s, "r", 1)) {
                        //����ֻ���ڴ�ϵ�
                        return AddMemBP(Addr, len, TYPE_MEMBP::MEMBP_READ);
                    } else if (!strncmp(s, "w", 1)) {
                        //����д���ڴ�ϵ�
                        return AddMemBP(Addr, len, TYPE_MEMBP::MEMBP_WRITE);
                    }
                }
            }
        }
    }
    printf("����ڴ�ϵ�ʧ��\n");
    return false;
}

bool MyDebuger::OnCmdBH(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, " ");
    if (s != nullptr) {
        // �õ���ַ
        void *Addr = (void *)strtoul(s, nullptr, 16);
        s = strtok(nullptr, " ");
        if (s != nullptr) {
            if (strncmp(s, "e", 1) == 0) {
                // ���Ӳ��ִ�жϵ㣬���ȱ���Ϊ1Byte
                return AddHBP(Addr, TYPE_HBP::HBP_EXECUTE, LEN_HBP::HBP_LEN_1B);
            } else if (strncmp(s, "w", 1) == 0) {
                // ���Ӳ��д��ϵ�
                s = strtok(nullptr, "\n");
                if (s != nullptr) {
                    if (strncmp(s, "1", 1) == 0) {
                        // ����1Byte
                        return AddHBP(Addr, TYPE_HBP::HBP_WRITE, LEN_HBP::HBP_LEN_1B);
                    } else if (strncmp(s, "2", 1) == 0) {
                        // ����2Byte
                        return AddHBP(Addr, TYPE_HBP::HBP_WRITE, LEN_HBP::HBP_LEN_2B);
                    } else if (strncmp(s, "4", 1) == 0) {
                        // ����4Byte
                        return AddHBP(Addr, TYPE_HBP::HBP_WRITE, LEN_HBP::HBP_LEN_4B);
                    }
                }
            } else if (strncmp(s, "a", 1) == 0) {
                // ���Ӳ�����ʶϵ�
                s = strtok(nullptr, "\n");
                if (s != nullptr) {
                    if (strncmp(s, "1", 1) == 0) {
                        // ����1Byte
                        return AddHBP(Addr, TYPE_HBP::HBP_ACCESS, LEN_HBP::HBP_LEN_1B);
                    } else if (strncmp(s, "2", 1) == 0) {
                        // ����2Byte
                        return AddHBP(Addr, TYPE_HBP::HBP_ACCESS, LEN_HBP::HBP_LEN_2B);
                    } else if (strncmp(s, "4", 1) == 0) {
                        // ����4Byte
                        return AddHBP(Addr, TYPE_HBP::HBP_ACCESS, LEN_HBP::HBP_LEN_4B);
                    }
                }
            }
        }
    }

    return false;
}

bool MyDebuger::OnCmdBHC(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, "\n");
    if (s != nullptr) {
        unsigned int index = strtoul(s, nullptr, 10);
        return DelHBP(index);
    } else {
        printf("ɾ��ʧ�ܣ������ʽ����\n");
    }
    return false;
}

bool MyDebuger::OnCmdG(char *buf)
{
    m_IsRun = true; // g����
    m_CanDump = false;  // ����dump

    // ���ϵ��Ƿ�����Ҫ�ָ�
    if (!m_BPReductionList.empty() || !m_MemBPPageReductionList.empty()) {
        // ���õ���
        SetSingleStep();
    }

    char *s = strtok(buf, " ");
    s = strtok(nullptr, "\n");
    if (s != nullptr) {
        void *Addr = (void *)strtoul(s, nullptr, 16);

        // ����ַ�Ƿ���Ч
        if (ProcessOperating::IsAddressValid(m_DebugEv.dwProcessId, Addr)) {
            // ��������ַ�Ƿ���ڶϵ�
            if (!IsExistsBP(Addr)) {
                // ����������һ���Զϵ�
                AddBP(Addr, true);
            }
        } else {
            printf("��ַ��%p��Ч", Addr);
            return false;
        }
    }

    return true;
}

bool MyDebuger::OnCmdT(char *buf)
{
    m_IsRun = false;    // ����g����
    m_CanDump = false;  // ����dump
    // ���õ���
    SetSingleStep(true);
    return true;
}

bool MyDebuger::OnCmdP(char *buf)
{
    m_IsRun = false;    // ����g����
    m_CanDump = false;  // ����dump

    // ��ȡ�ڴ��е�����
    ProcessOperating::ReadProcessMemory(m_DebugEv.dwProcessId, (PVOID)m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData));
    // ��鵱ǰ���Ƿ���call
    if (m_MemoryData[0] == 0xe8
        || m_MemoryData[0] == 0xff
        || m_MemoryData[0] == 0x9a) {
        // ������һ��ָ��λ��
        m_CurrentAddr += ShowDisassembly(m_CurrentAddr, m_MemoryData, sizeof(m_MemoryData), 1, false);

        // �ж��Ƿ��¹��ϵ㣬û�������ʱ�ϵ�
        if (!IsExistsBP((void *)m_CurrentAddr)) {
            AddBP((void *)m_CurrentAddr, true);
        }
    } else {
        // ���õ���
        SetSingleStep();
    }

    return true;
}

bool MyDebuger::OnCmdLS(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, "\n");
    if (s != nullptr) {
        // ����Ƿ���ɹ�
        if (ImportScript(s)) {
            return true;
        }
    }

    printf("����ʧ��\n");
    return false;
}

bool MyDebuger::OnCmdTRACE(char *buf)
{
    char *s = strtok(buf, " ");
    s = strtok(nullptr, " ");
    if (s != nullptr) {
        // ��ȡ��ʼ��ַ
        void *StartAddr = (void *)strtoul(s, nullptr, 16);
        s = strtok(nullptr, " ");
        if (s != nullptr) {
            // ��ȡ������ַ
            void *EndAddr = (void *)strtoul(s, nullptr, 16);

            s = strtok(nullptr, "\n");

            // �����Զ�����
            return SetAutoTrack(StartAddr, EndAddr, s == nullptr ? "" : s);
        } else {
            printf("��ʽ����\n");
            return false;
        }
    } else {
        printf("��ʽ����\n");
        return false;
    }
}

bool MyDebuger::OnCmdDUMP(char *buf)
{
    if(!m_CanDump) {
        // ��ʱ��������dump�ڴ�
        return false;
    }

    char *s = strtok(buf, " ");
    s = strtok(nullptr, " ");
    if(!s) {
        return false;
    }
    s = strtok(s, "\n");  // ��ȡ�ļ���

    auto MainModuelItem = m_ModuleList.front();
    DWORD ImageBase = (DWORD)MainModuelItem.hModule;
    IMAGE_DOS_HEADER ImageDosHeader { 0 };
    IMAGE_NT_HEADERS ImageNtHeaders { 0 };

    // ��ȡDosͷ
    ProcessOperating::ReadProcessMemory(m_pi.dwProcessId, MainModuelItem.hModule, &ImageDosHeader, sizeof(IMAGE_DOS_HEADER));

    // ��ȡNtͷ
    ProcessOperating::ReadProcessMemory(m_pi.dwProcessId, (BYTE *)ImageBase + ImageDosHeader.e_lfanew, &ImageNtHeaders, sizeof(IMAGE_NT_HEADERS));

    // �����ļ�
    HANDLE hFile = ::CreateFileA(s, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        // �����ļ�ʧ��
        return false;
    }

    // д��Dosͷ
    DWORD WriteBytes = 0;
    ::WriteFile(hFile, &ImageDosHeader, sizeof(IMAGE_DOS_HEADER), &WriteBytes, NULL);

    // д��Ntͷ
    ::SetFilePointer(hFile, ImageDosHeader.e_lfanew, NULL, FILE_BEGIN);
    ::WriteFile(hFile, &ImageNtHeaders, sizeof(IMAGE_NT_HEADERS), &WriteBytes, NULL);

    // ��λ���ڱ��λ��
    DWORD SectionTablePointer = ImageDosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + ImageNtHeaders.FileHeader.SizeOfOptionalHeader;

    // �����ڱ�ͽ�����
    for(DWORD i = 0; i < ImageNtHeaders.FileHeader.NumberOfSections; i++, SectionTablePointer += sizeof(IMAGE_SECTION_HEADER)) {
        // ��ȡ�ڱ�
        IMAGE_SECTION_HEADER ImageSectionHeader { 0 };
        ProcessOperating::ReadProcessMemory(m_pi.dwProcessId, (BYTE *)ImageBase + SectionTablePointer, &ImageSectionHeader, sizeof(IMAGE_SECTION_HEADER));

        // д��ڱ�
        ::SetFilePointer(hFile, SectionTablePointer, NULL, FILE_BEGIN);
        ::WriteFile(hFile, &ImageSectionHeader, sizeof(IMAGE_SECTION_HEADER), &WriteBytes, NULL);

        // �ж��Ƿ���δ��ʼ����
        if(ImageSectionHeader.SizeOfRawData == 0) {
            continue;
        }

        // ��ȡ������
        BYTE *SectionData = new BYTE[ImageSectionHeader.SizeOfRawData];
        ProcessOperating::ReadProcessMemory(m_pi.dwProcessId, (BYTE *)ImageBase + ImageSectionHeader.VirtualAddress, SectionData, ImageSectionHeader.SizeOfRawData);

        // д�������
        ::SetFilePointer(hFile, ImageSectionHeader.PointerToRawData, NULL, FILE_BEGIN);
        ::WriteFile(hFile, SectionData, ImageSectionHeader.SizeOfRawData, &WriteBytes, NULL);

        delete[] SectionData;
    }

    ::CloseHandle(hFile);
    return true;
}

void MyDebuger::ShowRegister()
{
    FLAGS_REGISTER Flags;
    if (ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
        Flags.Flags = m_Context.EFlags;

        // ��ʾͨ�üĴ����ͶμĴ���
        printf("EAX = %.8X  EBX = %.8X  ECX = %.8X  EDX = %.8X  EBP = %.8X  ESP = %.8X\n",
            m_Context.Eax, m_Context.Ebx, m_Context.Ecx, m_Context.Edx, m_Context.Ebp, m_Context.Esp);

        printf("ESI = %.8X  EDI = %.8X  EIP = %.8X  FS  = %.8X  GS  = %.8X  ES  = %.8X\n",
            m_Context.Esi, m_Context.Edi, m_Context.Eip, m_Context.SegFs, m_Context.SegGs, m_Context.SegEs);

        printf("DS  = %.8X  CS  = %.8X  SS  = %.8X  DR0 = %.8X  DR1 = %.8X  DR2 = %.8X\n",
            m_Context.SegDs, m_Context.SegCs, m_Context.SegSs, m_Context.Dr0, m_Context.Dr1, m_Context.Dr2);

        printf("DR3 = %.8X  DR6 = %.8X  DR7 = %.8X", m_Context.Dr3, m_Context.Dr6, m_Context.Dr7);

        // ��ʾ��־�Ĵ���
        printf("\t\t    ZF  PF  AF  OF  SF  DF  CF  TF  IF\r\n");
        printf("\t\t\t\t\t\t\t    %.2X  %.2X  %.2X  %.2X  %.2X  %.2X  %.2X  %.2X  %.2X\n",
            Flags.ZF, Flags.PF, Flags.AF,
            Flags.OF, Flags.SF, Flags.DF,
            Flags.CF, Flags.TF, Flags.IF);
    } else {
        printf("��ȡ�Ĵ���ʧ��\n");
    }
}

void MyDebuger::ShowModule()
{
    if (m_ModuleList.empty()) {
        printf("û�м����κ�ģ��\n");
        return;
    }
    // ������ʾģ����Ϣ
    for (auto &item : m_ModuleList) {
        printf("����:%20s  ��ַ:%p  ��С:%.8X  ·��:%s\r\n", item.ModuleName.c_str(), item.hModule,
            item.dwModuleSize, item.ModulePath.c_str());
    }
}

void MyDebuger::ShowHelp()
{
    char help[][CHAR_MAX] = {
        "����", " ˵��",
        " u", "�����",
        " p", "��������",
        " q", "�˳�",
        " r", "�鿴�Ĵ���",
        " t", "��������",
        " ls", "����ű�",
        " es", "�����ű�",
        " dd", "�鿴�ڴ�����",
        " ml", "�鿴�ű�",
        " bpl", "�鿴һ��ϵ�",
        " bhl", "�鿴Ӳ���ϵ�",
        " bml", "�鿴�ڴ�ϵ�",
        " bmpl", "�鿴��ҳ�ϵ�",
        " g [address]", "����",
        " bpc <number>", "ɾ��һ��ϵ�",
        " bp <address> [sys]", "һ��ϵ�",
        " e <address> <value>", "�޸��ڴ�����",
        " bhc <address> <number>", "ɾ��Ӳ���ϵ�",
        " bmc <address> <number>", "ɾ���ڴ�ϵ�",
        " bm <address> <length> <r/w>", "�ڴ�ϵ�",
        " bh <address> <e/r/a> <1/2/4>", "Ӳ���ϵ�",
        " trace <address> <address> <module>", "�Զ����ټ�¼"
    };

    // ��ʾ����ҳ��
    int count = sizeof(help) / CHAR_MAX;
    for (int i = 0; i < count; i += 2) {
        printf("%-35s\t%s\n", help[i], help[i + 1]);
    }
}

void MyDebuger::QuitDebug()
{
    exit(0);
}

void MyDebuger::ShowBPList()
{
    //���ϵ���Ƿ�Ϊ��
    if (!m_BPList.empty()) {
        printf("\n----------------------------�ϵ��б�-------------------------------\n");
        printf("�ϵ�����: %d\n", m_BPList.size());
        printf("���    ��ַ        ԭʼ����   �޸Ĵ���  �Ƿ�һ����\n");
        for (size_t i = 0; i < m_BPList.size(); i++) {
            printf("%4d    %p    %.1X         %.1X        %s\n", i, m_BPList[i].BPAddr,
                m_BPList[i].OldCode, m_BPList[i].NEwCode, (m_BPList[i].IsOnce) ? "true" : "false");
        }
        printf("-------------------------------------------------------------------\n\n");
    } else {
        printf("�ϵ��б�Ϊ��\n");
    }
}

void MyDebuger::ShowMemBPList(bool IsShowPage)
{
    //����Ƿ���ʾ��ҳ
    if (IsShowPage) {
        //����ڴ�ϵ��ҳ���Ƿ�Ϊ��
        if (!m_MemBPPageList.empty()) {
            printf("\n--------------------------�ڴ��ҳ�ϵ��б�-----------------------------\n");
            printf("�ڴ��ҳ�ϵ�����: %d\n", m_MemBPPageList.size());
            printf("��ҳ���    ��ҳ��ַ    ��ҳ��С    ������     ������\n");

            for (size_t i = 0; i < m_MemBPPageList.size(); i++) {
                printf("%8d    %p    %.8X    %.8X   %.8X\n", i, m_MemBPPageList[i].StartAddr,
                    m_MemBPPageList[i].dwSize, m_MemBPPageList[i].dwOldProtect, m_MemBPPageList[i].dwNewProtect);
            }
            printf("-------------------------------------------------------------------\n");
        } else {
            printf("�ڴ��ҳ�ϵ��б�Ϊ��\n");
        }
    } else {
        //����ڴ�ϵ��ҳ���Ƿ�Ϊ��
        if (!m_MemBPList.empty()) {
            printf("\n---------------------------�ڴ�ϵ��б�------------------------------\n");
            printf("�ڴ�ϵ�����: %d\n", m_MemBPList.size());
            printf("�ϵ���    �ϵ��ַ    �ϵ��С    �ϵ�����\n");

            for (size_t i = 0; i < m_MemBPList.size(); i++) {
                printf("%8d    %p    %.8X    %s\n", i, m_MemBPList[i].MemBPAddr,
                    m_MemBPList[i].dwSize, (m_MemBPList[i].MemBPType == TYPE_MEMBP::MEMBP_READ) ? "��" : "д");
            }
            printf("-------------------------------------------------------------------\n");
        } else {
            printf("�ڴ�ϵ��б�Ϊ��\n");
        }
    }
}

void MyDebuger::ShowHBPList()
{
    //���Ӳ���ϵ������Ƿ�Ϊ��
    if (!m_HBPList.empty()) {
        printf("\n---------------------------Ӳ���ϵ��б�------------------------------\n");
        printf("Ӳ���ϵ�����: %d\n", m_HBPList.size());
        printf("�ϵ����    �ϵ��ַ    �ϵ�����    �ϵ��С\n");

        for (size_t i = 0; i < m_HBPList.size(); i++) {
            printf("%8d    %p    ", i, m_HBPList[i].HBPAddr);

            //��ʾ����
            switch (m_HBPList[i].HBPType)
            {
            case TYPE_HBP::HBP_EXECUTE:
                printf("ִ��        ");
                break;

            case TYPE_HBP::HBP_ACCESS:
                printf("����        ");
                break;

            case TYPE_HBP::HBP_WRITE:
                printf("д��        ");
                break;
            }

            //��ʾ��С
            switch (m_HBPList[i].HBPLen)
            {
            case LEN_HBP::HBP_LEN_1B:
                printf("1�ֽ�\n");
                break;

            case LEN_HBP::HBP_LEN_2B:
                printf("2�ֽ�\n");
                break;

            case LEN_HBP::HBP_LEN_4B:
                printf("4�ֽ�\n");
                break;
            }
        }
        printf("-------------------------------------------------------------------\n");
    } else {
        printf("Ӳ���ϵ��б�Ϊ��\n");
    }
}

bool MyDebuger::SetSingleStep(bool IsEnable)
{
    FLAGS_REGISTER FlagsRegister;

    // ��ȡ�߳������Ļ���
    if (ProcessOperating::GetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
        FlagsRegister.Flags = m_Context.EFlags;
        FlagsRegister.TF = IsEnable;    // ����TFλ
        m_Context.EFlags = FlagsRegister.Flags;

        // �����߳������Ļ���
        if (ProcessOperating::SetThreadContext(m_DebugEv.dwThreadId, m_Context)) {
            return true;
        }
    }
    return false;
}

bool MyDebuger::DumpTrackInfo(std::string FileName)
{
    // �����ļ�
    HANDLE hFile = ::CreateFileA(FileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    // ����Ƿ񴴽��ɹ�
    if (hFile == INVALID_HANDLE_VALUE) {
        ErrorReport::Report("CreateFileA");
        return false;
    }

    DWORD WriteBytes = 0;
    for (auto &item : m_AutoTrackList) {
        // д���ļ�
        ::WriteFile(hFile, item.Data.c_str(), item.Data.length(), &WriteBytes, NULL);
    }

    // �ر��ļ����
    ::CloseHandle(hFile);

    // ��ո�������
    m_AutoTrackList.clear();
    return true;
}

bool MyDebuger::ExportScript()
{
    char FileName[MAX_PATH] { 0 };
    // ����ģ��Ϊ�ļ���
    sprintf(FileName, "%s.scp", m_ModuleList.front().ModuleName.c_str());

    // �����ļ�
    HANDLE hFile = ::CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    // ����Ƿ񴴽��ɹ�
    if (hFile == INVALID_HANDLE_VALUE) {
        ErrorReport::Report("CreateFileA");
        return false;
    }

    DWORD WriteBytes = 0;

    // ������������ д��
    for (auto &item : m_CmdRecordList) {
        // ��ȡ�ַ����Ĵ�С
        unsigned int StrSize = item.size();
        // д���ַ���
        ::WriteFile(hFile, item.c_str(), StrSize, &WriteBytes, NULL);
    }

    // �رվ��
    ::CloseHandle(hFile);
    printf("�����ű���%s�ɹ�\n", FileName);
    return true;
}

bool MyDebuger::ImportScript(std::string FileName)
{
    // ���ļ���
    std::ifstream in(FileName);
    std::string cmd;

    // ��ȡ��
    while(std::getline(in, cmd)) {
        m_CmdLineQueue.push(cmd);
    }

    return true;
}

bool MyDebuger::SetAutoTrack(void *StartAddr, void *EndAddr, std::string ModuleName)
{
    // ���ģ����
    if (ModuleName.empty()) {
        // ��ģ����
        m_AutoTrackParam.IsQualifiedModule = false; // ���޶�ģ��
    } else {
        // ��ģ���������ģ���Ƿ����
        unsigned int index = 0;
        if (!IsExistsModule(ModuleName, &index)) {
            printf("ģ�鲻����\n");
            return false;
        }
        m_AutoTrackParam.IsQualifiedModule = true; // �޶�ģ��
        m_AutoTrackParam.ModuleStartAddr = m_ModuleList[index].hModule; // ģ����ʼ��ַ
        m_AutoTrackParam.ModuleEndAddr = (void *)((DWORD64)m_ModuleList[index].hModule + m_ModuleList[index].dwModuleSize); // ģ�������ַ
    }

    m_AutoTrackParam.StartAddr = StartAddr;
    m_AutoTrackParam.EndAddr = EndAddr;

    // ������һ���Զϵ�
    if (AddBP(StartAddr, true) && AddBP(EndAddr, true)) {
        // �����Զ�����
        m_IsAutoTrack = true;
        return true;
    }

    printf("�Զ���������ʧ��\n");
    return false;
}

bool MyDebuger::AppendTrackData(void *TrackAddr, std::string Data)
{
    // ����Ƿ��޶�ģ��
    if (m_AutoTrackParam.IsQualifiedModule) {
        // ����ַ��Χ�Ƿ����޶���ģ����
        if (m_AutoTrackParam.ModuleStartAddr > TrackAddr || m_AutoTrackParam.ModuleEndAddr < TrackAddr) {
            return false;
        }
    }
    // ����������
    if(!IsExistsTrackAddr(TrackAddr)) {
        m_AutoTrackList.push_back(AUTO_TRACK_ITEM({ TrackAddr, Data }));
    }
    return true;
}

bool MyDebuger::IsExistsTrackAddr(void *TrackAddr)
{
    for (auto &item : m_AutoTrackList) {
        if (TrackAddr == item.TrackAddr) {
            return true;    // ����
        }
    }
    return false;
}

bool MyDebuger::IsExistsOtherBreakPoint(void *BreakPointAddr)
{
    // ���һ��ϵ�
    for(auto &item : m_BPList) {
        if(item.BPAddr == BreakPointAddr) {
            return true;
        }
    }

    // ����ڴ�ϵ�
    for(auto &item : m_MemBPPageList) {
        if(item.StartAddr <= BreakPointAddr && (DWORD64)item.StartAddr + item.dwSize >= (DWORD64)BreakPointAddr) {
            return true;
        }
    }

    // ���Ӳ���ϵ�
    for(auto &item : m_HBPList) {
        if(item.HBPAddr <= BreakPointAddr && ((DWORD64)item.HBPAddr + (DWORD64)item.HBPLen + 1) >= (DWORD64)BreakPointAddr) {
            return true;
        }
    }

    return false;
}
