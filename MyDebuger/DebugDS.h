#pragma once

#include <Windows.h>
#include <string>

// ��־�Ĵ���
union FLAGS_REGISTER {
    unsigned int Flags;
    struct {
        unsigned int CF : 1;
        unsigned int : 1;
        unsigned int PF : 1;
        unsigned int : 1;
        unsigned int AF : 1;
        unsigned int : 1;
        unsigned int ZF : 1;
        unsigned int SF : 1;
        unsigned int TF : 1;
        unsigned int IF : 1;
        unsigned int DF : 1;
        unsigned int OF : 1;
    };
};

// Ӳ��DR7�Ĵ���
union DR7 {
    // ����ȫ�� L0-L3
    // �ϵ㳤�� LEN0-LEN3    00��1�ֽ�       01��2�ֽ�      11��4�ֽ�
    // �ϵ����� RW0-RW3      00��ִ��        01��д��       11����д
    struct {
        unsigned int L0 : 1;
        unsigned int G0 : 1;
        unsigned int L1 : 1;
        unsigned int G1 : 1;
        unsigned int L2 : 1;
        unsigned int G2 : 1;
        unsigned int L3 : 1;
        unsigned int G3 : 1;
        unsigned int LE : 1;
        unsigned int GE : 1;
        unsigned int : 3;
        unsigned int GD : 1;
        unsigned int : 2;

        unsigned int RW0 : 2;
        unsigned int LEN0 : 2;

        unsigned int RW1 : 2;
        unsigned int LEN1 : 2;

        unsigned int RW2 : 2;
        unsigned int LEN2 : 2;

        unsigned int RW3 : 2;
        unsigned int LEN3 : 2;

    };
    unsigned int Dr7;
};


// ģ��ڵ�
struct MODULE_ITEM {
    HANDLE hModule;             // ģ����
    DWORD dwModuleSize;         // ģ���С
    std::string ModuleName;     // ģ������
    std::string ModulePath;     // ģ��·��
};

// �߳̽ڵ�
struct THREAD_ITEM {
    HANDLE hThread;                         // �߳̾��
    void *ThreadLocalBase;                  // ���ݿ�ָ��
    LPTHREAD_START_ROUTINE StartAddress;    // �ص���ַ
};

// �ϵ�ڵ�
struct BP_ITEM {
    void *BPAddr;       // �ϵ��ַ
    BYTE OldCode;       // ԭʼ����
    BYTE NEwCode;       // �´���
    bool IsOnce;        // �Ƿ�һ����
};

// �ڴ�ϵ������
enum TYPE_MEMBP {
    MEMBP_READ,   // ��
    MEMBP_WRITE   // д
};

// �ڴ�ϵ�ڵ�
struct MEMBP_ITEM {
    void *MemBPAddr;        // �ڴ�ϵ��ַ
    DWORD dwSize;          // ��С
    TYPE_MEMBP MemBPType;   // �ڴ�ϵ�����
};

// �ڴ��ҳ�ڵ�
struct MEM_PAGE_ITEM {
    void *StartAddr;        // ��ʼ��ַ
    DWORD dwSize;           // ��С
    DWORD dwOldProtect;     // ԭ�ȱ�������
    DWORD dwNewProtect;     // �µı�������
};

// Ӳ���ϵ������
enum TYPE_HBP {
    HBP_EXECUTE = 0,    // ִ��
    HBP_WRITE = 1,      // д
    HBP_ACCESS = 3      // ����
};

// Ӳ���ϵ�ĵ�ַ����
enum LEN_HBP {
    HBP_LEN_1B = 0,     // 1�ֽ�
    HBP_LEN_2B = 1,     // 2�ֽ�
    HBP_LEN_4B = 3      // 4�ֽ�
};

// Ӳ���ϵ�ڵ�
struct HBP_ITEM {
    DWORD DrId;         // �ϵ�Ĵ����ı��Dr0-Dr3
    void *HBPAddr;      // �ϵ��ַ
    TYPE_HBP HBPType;   // Ӳ���ϵ�����
    LEN_HBP HBPLen;     // Ӳ���ϵ㳤��
};

// �Զ����ٲ���
struct AUTO_TRACK_PARAM {
    void *StartAddr;            // ��ʼ��ַ
    void *EndAddr;              // ������ַ
    bool IsQualifiedModule;     // �Ƿ��޶�ģ��
    void *ModuleStartAddr;      // ģ�鿪ʼ��ַ
    void *ModuleEndAddr;        // ģ�������ַ
};

// �Զ����ٽڵ�
struct AUTO_TRACK_ITEM {
    void *TrackAddr;    // ��ַ
    std::string Data;   // ��¼
};