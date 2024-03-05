#ifndef CLOGIC_H
#define CLOGIC_H

#include"TCPKernel.h"

class CLogic
{
public:
    CLogic( TcpKernel* pkernel )
    {
        m_pKernel = pkernel;
        m_sql = pkernel->m_sql;
        m_tcp = pkernel->m_tcp;
    }
public:
    //设置协议映射
    void setNetPackMap();
    /************** 发送数据*********************/
    void SendData( sock_fd clientfd, char*szbuf, int nlen )
    {
        m_pKernel->SendData( clientfd ,szbuf , nlen );
    }
    /************** 网络处理 *********************/
    //注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);
    //文件上传请求
    void HandleUpFileRq(sock_fd clientfd, char*szbuf, int nlen);
    //文件块请求
    void FileContentRq(sock_fd clientfd ,char* szbuf,int nlen);
    //获取文件信息请求
    void GetFileInfoRq(sock_fd clientfd ,char* szbuf,int nlen);
    //下载文件请求
    void DownloadFileRq(sock_fd clientfd ,char* szbuf,int nlen);
    //下载文件夹请求
    void DownloadFolderRq(sock_fd clientfd ,char* szbuf,int nlen);
    //文件头回复
    void FileHeaderRs(sock_fd clientfd ,char* szbuf,int nlen);
   
    //文件内容回复
    void FileContentRs(sock_fd clientfd ,char* szbuf,int nlen);
    //新建文件夹请求
    void AddFolderRq(sock_fd clientfd ,char* szbuf,int nlen);
    //新建文件夹请求
    void ShareFileRq(sock_fd clientfd ,char* szbuf,int nlen);
    //获取个人分享的所有文件信息
    void MyShareRq(sock_fd clientfd ,char* szbuf,int nlen);
    //获取分享添加到目录
    void GetShareRq(sock_fd clientfd ,char* szbuf,int nlen);
    void GetShareByFile(int userid,int fileid,string dir,string name,string time);
    void GetShareByFolder(int userid,int fileid,string dir,string name,string time,int fromuserid,string fromdir);
   //删除文件请求
    void DeleteFileRq(sock_fd clientfd ,char* szbuf,int nlen);
    void DeleteOneItem(int userid,int fileid,string dir);
    void DeleteFile(int u_id, int f_id, string dir, string path); 
    void DeleteFolder(int u_id, int f_id, string dir, string name);               
    //分享一个文件
    void ShareItem(int userid, int fileid, string dir, string time, int link);
    void DownloadFile(int userid,int &timestamp,sock_fd clientfd,list<string>&lstRes);
    void DownloadFolder(int userid,int &timestamp,sock_fd clientfd,list<string>&lstRes);
    void ContinueDownloadRq(sock_fd clientfd ,char* szbuf,int nlen);
    void ContinueUploadRq(sock_fd clientfd ,char* szbuf,int nlen);
    /*******************************************/
    static int64_t GetTenBillion(){
        return 10000000000;
    }

private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;
    MyMap<int64_t, FileInfo*> m_UidTimeToFileinfo;    
};

#endif // CLOGIC_H
