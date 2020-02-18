#pragma once
#include <string>
#include <vector>
#include <queue>
#include <Windows.h>
#include<Zycore/Format.h>
#include<Zycore/LibC.h>
#include<Zydis/Zydis.h>
#include "DebugDS.h"

class MyDebuger
{
public:
    MyDebuger(std::string strApp);
    virtual ~MyDebuger();



    // ��������ʼ����
    void Work();


protected:
    // �����쳣�����¼�
    DWORD OnDebugExceptionEvent();

    // �������߳��¼�
    DWORD OnCreateThreadEvent();

    // �����������¼�
    DWORD OnCreateProcessEvent();

    // �����˳��߳��¼�
    DWORD OnExitThreadEvent();

    // �����˳������¼�
    DWORD OnExitProcessEvent();

    // �������DLL�¼�
    DWORD OnLoadDllEvent();

    // ����ж��DLL�¼�
    DWORD OnUnloadDLLEvent();

    // ����ϵ��쳣
    DWORD OnExceptionBreakPoint();

    // �����ڴ�����쳣
    DWORD OnExceptionAccessViolation();

    // �������쳣
    DWORD OnExceptionSingleStep();

    // ����Ӳ���ϵ��쳣
    DWORD OnExceptionHardwareBreakPoint();

    // ��ȡ�û�������
    DWORD GetCmdLine();

    /*
     * ��Ӷϵ�
     *      BPAddr���ϵ��ַ
     *      IsOnecBP���Ƿ���һ���Զϵ�
     */
    bool AddBP(void *BPAddr, bool IsOnecBP = false);

    /*
     * ɾ���ϵ�
     *      number���ϵ�ı��
     */
    bool DelBP(unsigned int number);

    /*
     * �ϵ��Ƿ����
     *      BPAddr���ϵ��ַ
     */
    bool IsExistsBP(void *BPAddr);

    /*
     * ����ڴ�ϵ�
     *      BPAddr���ϵ��ַ
     *      BPSize����С
     *      BPType���ϵ�����
     */
    bool AddMemBP(void *BPAddr, unsigned int BPSize, TYPE_MEMBP BPType);

    /*
     * ����ڴ��ҳ�Ƿ����
     *      PageAddr����ҳ��ַ
     *      Index������
     */
    bool IsExistsMemPage(void *PageAddr, unsigned int *Index = nullptr);

    /*
     * �Ƿ�����ڴ�ϵ�
     *      PageAddr����ҳ��ַ
     *      Index������
     */
    bool IsExistMemBP(void *PageAddr, unsigned int *Index = nullptr);


    /*
     * ɾ���ڴ�ϵ�
     *      number���ϵ�ı��
     */
    bool DelMemBP(unsigned int number);

    /*
     * ���Ӳ���ϵ�
     *      BPAddr���ϵ��ַ
     *      HBPType���ϵ�����
     *      HBPSize���ϵ��С
     */
    bool AddHBP(void *BPAddr, TYPE_HBP HBPType, LEN_HBP HBPSize);

    /*
     * ɾ��Ӳ���ϵ�
     *      number���ϵ�ı��
     */
    bool DelHBP(unsigned int number);

    /*
     * ���ģ�鵽������
     *      hFile��ģ����ļ����
     *      IsLoadDLl���Ƿ��Ǽ���DLL
     */
    void AppendModule(HANDLE hFile, bool IsLoadDLl = false);

    /*
     * ���ģ���Ƿ����
     *      ModuleName��ģ����
     *      Index������
     */
    bool IsExistsModule(std::string ModuleName, unsigned int *Index);


    /*
     * ����ಢ��ʾ
     *      runtime_address   ����ʱ��ַ��ʾ��ַ
     *      data              ���ݻ�����
     *      length            ���ݴ�С
     *      dwLine            ��ʾ����
     *      bIsShow           �Ƿ���ʾĬ����ʾ(�������ʾ�����������㷴����ַλ��)
     */
    ZyanUSize ShowDisassembly(ZyanU64 runtime_address, ZyanU8* data, ZyanUSize length, DWORD dwLine, bool bIsShow = true);


    /*
     * ��ʾʮ������
     *      dwAddress         ��ʾ��ַ
     *      pBuf              ���ݻ�����
     *      dwSize            ���ݴ�С
     *      dwLine            ��ʾ����
    */
    ZyanUSize ShowHex(ZyanU64 ZuAddress, PBYTE pBuf, DWORD dwSize, DWORD dwLine);


    /*
     * ����u���u [address]
     *      buf���������
     */
    bool OnCmdU(char *buf);

    /*
     * ����dd���dd [address]
     *      buf���������
     */
    bool OnCmdDD(char *buf);

    /*
     * ����e���e <address> <value>
     *      buf���������
     */
    bool OnCmdE(char *buf);

    /*
     * ����bp���bp <address> [sys]
     *      buf���������
     */
    bool OnCmdBP(char *buf);

    /*
     * ����bpc���bpc <number>
     *      buf���������
     */
    bool OnCmdBPC(char *buf);

    /*
     * ����bmc���bmc <number>
     *      buf���������
     */
    bool OnCmdBMC(char *buf);

    /*
     * ����bm���bm <address> <length> <type>
     *      buf���������
     */
    bool OnCmdBM(char *buf);

    /*
     * ����bh���bh <address> <type> <length> 
     *      buf���������
     */
    bool OnCmdBH(char *buf);

    /*
     * ����bhc���bhc <number>
     *      buf���������
     */
    bool OnCmdBHC(char *buf);


    /*
     * ����g���g [address]
     *      buf���������
     */
    bool OnCmdG(char *buf);

    /*
     * ����t���t
     *      buf���������
     */
    bool OnCmdT(char *buf);

    /*
     * ����p���p
     *      buf���������
     */
    bool OnCmdP(char *buf);

    /*
     * ����ls���ls <file>
     *      buf���������
     */
    bool OnCmdLS(char *buf);

    /*
     * ����trace���trace <address> <address> [module]
     *      buf���������
     */
    bool OnCmdTRACE(char *buf);

    /*
     * ����dump���dump <file name>
     *      buf���������
     */
    bool OnCmdDUMP(char *buf);

    // ��ʾ�Ĵ���
    void ShowRegister();

    // ��ʾģ��
    void ShowModule();

    // �鿴����
    void ShowHelp();

    // �˳�
    void QuitDebug();

    // ��ʾ�ϵ��б�
    void ShowBPList();

    /*
     * ��ʾ�ڴ�ϵ��б�
     *      IsShowPage���Ƿ���ʾ��ҳ
     */
    void ShowMemBPList(bool IsShowPage = false);

    // ��ʾӲ���ϵ��б�
    void ShowHBPList();

    /*
     * ���õ���
     *      IsEnable���Ƿ�������
     */
    bool SetSingleStep(bool IsEnable = true);

    /*
     * dump������Ϣ
     *      FileName���ļ���
     */
    bool DumpTrackInfo(std::string FileName);

    // �����ű�
    bool ExportScript();

    /*
     * ����ű�
     *      FileName���ļ���
     */
    bool ImportScript(std::string FileName);

    /*
     * �����Զ����ٲ���
     *      StartAddr����ʼ��ַ
     *      EndAddr��������ַ
     *      ModuleName��ģ����
     */
    bool SetAutoTrack(void *StartAddr, void *EndAddr, std::string ModuleName = "");

    /*
     * ���Ӹ��ٽڵ�����
     *      TrackAddr����ַ
     *      Data������
     */
    bool AppendTrackData(void *TrackAddr, std::string Data);

    /*
     * ���ٵĵ�ַ�Ƿ��ظ�
     *      TrackAddr����ַ
     */
    bool IsExistsTrackAddr(void *TrackAddr);

    /*
     * �Ƿ���������ϵ�
     *      BreakPointAddr���ϵ��ַ
     */
    bool IsExistsOtherBreakPoint(void *BreakPointAddr);

private:
    // ���������zydis���
    ZydisDecoder m_Decoder;
    ZyanU8 m_MemoryData[0x1000];    // ��Ŵ���Ļ�����
    ZyanU64 m_CurrentAddr = 0;  // ��ǰ�쳣�ĵ�ַ


    // ���������
    bool m_IsStart = false;   // �Ƿ��������
    DEBUG_EVENT m_DebugEv;    // �����¼�
    DWORD m_dwContinueStatus = DBG_CONTINUE;    // ���Դ���״̬
    bool m_IsSysBP = true;  // �Ƿ���ϵͳ�ϵ�
    CONTEXT m_Context;      // �߳������Ļ���
    bool m_IsAutoTrack = false;   // �Ƿ��Զ�׷��
    bool m_IsTrack = false; // �Ƿ�������
    bool m_IsRun = false;   // g��������
    bool m_CanDump = true;  // �Ƿ����dump�ļ�

    std::vector<AUTO_TRACK_ITEM> m_AutoTrackList;   // �Զ���������
    AUTO_TRACK_PARAM m_AutoTrackParam;  // �Զ����ٲ���

    std::vector<MODULE_ITEM> m_ModuleList;  // ģ������
    std::vector<THREAD_ITEM> m_ThreadList;  // �߳�����

    std::vector<BP_ITEM> m_BPList;          // �ϵ�����
    std::vector<BP_ITEM> m_BPReductionList; // �ϵ㻹ԭ����
    std::vector<MEMBP_ITEM> m_MemBPList;                    // �ڴ�ϵ�����
    std::vector<MEM_PAGE_ITEM> m_MemBPPageList;           // �ڴ��ҳ�ϵ�����
    std::vector<MEM_PAGE_ITEM> m_MemBPPageReductionList;  // �ڴ��ҳ�ϵ㻹ԭ����
    std::vector<HBP_ITEM> m_HBPList;            // Ӳ���ϵ�����
    std::vector<HBP_ITEM> m_HBPReductionList;   // Ӳ���ϵ㻹ԭ����

    std::queue<std::string> m_CmdLineQueue;     // �������
    std::vector<std::string> m_CmdRecordList;   // �����¼����

    // �����Խ������
    std::string m_strApp;   // �����Գ���
    STARTUPINFO m_si { 0 };
    PROCESS_INFORMATION m_pi { 0 };
};

