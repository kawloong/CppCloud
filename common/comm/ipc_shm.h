/******************************************************************* 
 *  summery:        IPC通信之共享内存类
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    对shmget shmat shmdt等(先不封装)
 ******************************************************************/
#ifndef _IPC_SHM_H_
#define _IPC_SHM_H_


class IpcShm
{
public:
    IpcShm(key_t key);
    IpcShm(const char* pathfile, int proj_id); // call ftok()

    int init(unsigned int size);

private:
    int m_shmid; // 共享内存标识;
    void* m_ptr;
};

#endif
