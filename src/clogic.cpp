#include "clogic.h"
#include "dirent.h"
#include "FileTransfer.h"
#include <bits/shared_ptr.h>
#include "Transfer/FileTransferTask.h"
#include "Transfer/FileWriter.h"
#include "Transfer/NoOpWriter.h"
#include  <atomic>
// std::atomic<int64_t> s_requestCounter;
// int64_t GetNextRequestId() {
//     return ++s_requestCounter;
// }
FileTransferClient FileTran("127.0.0.1",9000);
void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ) = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ) = &CLogic::LoginRq;
    NetPackMap(_DEF_PACK_UPLOAD_FILE_RQ) = &CLogic::HandleUpFileRq;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RQ)=&CLogic::FileContentRq;
    NetPackMap(_DEF_PACK_GET_FILE_INFO_RQ)=&CLogic::GetFileInfoRq;

    NetPackMap(_DEF_PACK_DOWNLOAD_FILE_RQ)=&CLogic::DownloadFileRq;
    NetPackMap(_DEF_PACK_FILE_HEADER_RS)=&CLogic::FileHeaderRs;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RS)=&CLogic::FileContentRs;
    NetPackMap(_DEF_PACK_ADD_FOLDER_RQ)=&CLogic::AddFolderRq;
    NetPackMap(_DEF_PACK_SHARE_FILE_RQ)=&CLogic::ShareFileRq;
    NetPackMap(_DEF_PACK_MY_SHARE_RQ)=&CLogic::MyShareRq;
    NetPackMap(_DEF_PACK_GET_SHARE_RQ)=&CLogic::GetShareRq;    
    NetPackMap(_DEF_PACK_DOWNLOAD_FOLDER_RQ)=&CLogic::DownloadFolderRq;
    NetPackMap(_DEF_PACK_DELETE_FILE_RQ)=&CLogic::DeleteFileRq;
    NetPackMap(_DEF_PACK_CONTINUE_DOWNLOAD_RQ)=&CLogic::ContinueDownloadRq;
    NetPackMap(_DEF_PACK_CONTINUE_UPLOAD_RQ)=&CLogic::ContinueUploadRq;

   // NetPackMap(_DEF_PACK_DOWNLOAD_FOLDER_RQ)=&CLogic::DownloadFolderRq;
}

// 注册
void CLogic::RegisterRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    // 拆包
    STRU_REGISTER_RQ registRq = *(STRU_REGISTER_RQ *)szbuf;
    char tel[12];
    char password[33];
    char name[11];
    strcpy(tel, registRq.tel);
    strcpy(password, registRq.password);
    strcpy(name, registRq.name);
    // 数据库查手机号是否已经存在
    char sqlStatement[100];
    
    sprintf(sqlStatement, "select u_id from t_user where u_tel = '%s';", tel);
    list<string> result_list;
    if (m_sql->SelectMysql(sqlStatement, 1, result_list) == false)
    {
        cout << "error sqlStation: " << sqlStatement << endl;
        return;
    }
    // 准备发包
    STRU_REGISTER_RS registRs;
    if (result_list.size() != 0)
    { // 说明已经存在
        registRs.result = user_is_exist;
    }
    else
    {
        // 清除结果集
        result_list.clear();
        // 数据库插入数据
        memset(sqlStatement, 0, sizeof(sqlStatement));
        sprintf(sqlStatement, "insert into t_user ( u_tel , u_password , u_name ) values( '%s' , '%s' , '%s');", tel, password, name);
        if (false == m_sql->UpdataMysql(sqlStatement))
        {
            cout << "insert error:" << sqlStatement << endl;
            return;
        }
        // 查询id
        memset(sqlStatement, 0, sizeof(sqlStatement));
        sprintf(sqlStatement, "select u_id from t_user where u_tel = '%s';", tel);
        if (m_sql->SelectMysql(sqlStatement, 1, result_list) == false)
        {
            cout << "error sqlStation: " << sqlStatement << endl;
            return;
        }
        int userid = stoi(*(result_list.begin()));
        
        cout << "注册成功,用户id: " << userid << endl;
        
        // 创建对应id的目录
        umask(0000);
        char path[100];
        sprintf(path, "%s%d", _DEF_PATH, userid);
        mkdir(path, 0777);
        registRs.result = register_success;
    }
    // 根据插入结果, 返回
    SendData(clientfd, (char *)&registRs, sizeof(registRs));
}

// 登录
void CLogic::LoginRq(sock_fd clientfd, char *szbuf, int nlen)
{
    //    cout << "clientfd:"<< clientfd << __func__ << endl;
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    // 拆包
    STRU_LOGIN_RQ LoginRq = *(STRU_LOGIN_RQ *)szbuf;
    char password[33];
    char tel[12];
    strcpy(password, LoginRq.password);
    strcpy(tel, LoginRq.tel);

    // 建发送包
    STRU_LOGIN_RS loginRs;
    list<string> result_list;
    char sqlStatement[100];
    memset(sqlStatement, 0, sizeof(sqlStatement));
    sprintf(sqlStatement, "select u_id from t_user where u_tel = '%s';", tel);
    //查找对应id,存在则说明成功并返回id；
    if (m_sql->SelectMysql(sqlStatement, 1, result_list) == false)
    {
        cout << "error sqlStation: " << sqlStatement << endl;
        return;
    }
    else if (result_list.size() == 0)
    {
        loginRs.result = user_not_exist;
        
    }
    else
    {
        //为请求协议赋值
        loginRs.userid = stoi(*(result_list.begin()));
        cout<<"用户id："<<loginRs.userid<<endl;
        
        memset(sqlStatement, 0, sizeof(sqlStatement));
        result_list.clear();
        sprintf(sqlStatement, "select * from t_user where u_tel = '%s' and u_password = '%s';", tel, password);
        if (m_sql->SelectMysql(sqlStatement, 1, result_list) == false)
        {
            cout << "error sqlStation: " << sqlStatement << endl;
            return;
        }
        else if (result_list.size() == 0)
        {
            loginRs.result = password_error;
        }
        else
        {
            loginRs.result = login_success;
        }
    }
    SendData(clientfd, (char *)&loginRs, sizeof(loginRs));
}

void CLogic::HandleUpFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog << "clientfd:" << clientfd << " userId:" << ((STRU_UPLOAD_FILE_RQ*)szbuf)->userid << " " << __func__ << std::endl;

    // 拆包
    STRU_UPLOAD_FILE_RQ *rq = reinterpret_cast<STRU_UPLOAD_FILE_RQ*>(szbuf);

    int64_t serverReqId = rq->timestamp;

    // TODO: 查看是否秒传
    {
        // 判断文件是否已经上传
        char sqlbuf[1000] = "";
        sprintf(sqlbuf,
                "SELECT f_id FROM t_file WHERE f_MD5='%s' AND f_state=1;",
                rq->md5);

        std::list<std::string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf, 1, lstRes);
        if (!res) {
            std::cerr << "select fail: " << sqlbuf << std::endl;
            return;
        }
        if (!lstRes.empty()) { // 已上传
            int fileid = std::stoi(lstRes.front());

            // 插入用户文件关系，触发器会自动增加引用计数
            sprintf(sqlbuf,
                    "INSERT INTO t_user_file(u_id, f_id, f_dir, f_name, f_uploadtime)"
                    " VALUES(%d, %d, '%s', '%s', '%s');",
                    rq->userid, fileid, rq->dir, rq->fileName, rq->time);
            res = m_sql->UpdataMysql(sqlbuf);
            if (!res) sqlLog << "update fail" << sqlbuf << std::endl;

            // 构造秒传回复，使用 serverReqId 而非客户端 timestamp
            STRU_QUICK_UPLOAD_RS qs;
            qs.result = 1;
            qs.timestamp = serverReqId;
            qs.userid = rq->userid;
            SendData(clientfd, reinterpret_cast<char*>(&qs), sizeof(qs));
            return;
        }
    }

    // 不是秒传，准备接收上传数据
    FileInfo *info = new FileInfo;
    char strpath[1000] = "";
    sprintf(strpath, "%s%d%s%s", _DEF_PATH, rq->userid, rq->dir, rq->md5);

    info->absolutePath = strpath;
    info->dir = rq->dir;
    info->md5 = rq->md5;
    info->name = rq->fileName;
    info->size = rq->size;
    info->time = rq->time;
    info->type = rq->type;

    info->filefd = open(strpath, O_CREAT | O_WRONLY | O_TRUNC, 00777);
    if (info->filefd < 0) {
        std::cerr << "file open fail:"<< strpath << std::endl;
        delete info;
        return;
    }

    // 使用服务器请求 ID 作为 map 键的一部分，避免冲突
    int64_t mapKey = static_cast<int64_t>(rq->userid) * GetTenBillion() + serverReqId;
    m_UidTimeToFileinfo.insert(mapKey, info);
    char  sqlbuf[1000] = "";
    // 数据库记录文件元信息
    sprintf(sqlbuf,
            "INSERT INTO t_file(f_size, f_path, f_MD5, f_count, f_state, f_type)"
            " VALUES(%d, '%s', '%s', 0, 0, 'file');",
            rq->size, strpath, rq->md5);
    if (!m_sql->UpdataMysql(sqlbuf)) sqlLog << "insert fial" << sqlbuf << std::endl;

    // 查找文件 ID
    sprintf(sqlbuf,
            "SELECT f_id FROM t_file WHERE f_path='%s' AND f_md5='%s';",
            strpath, rq->md5);
    std::list<std::string> lstRes2;
    if (m_sql->SelectMysql(sqlbuf, 1, lstRes2) && !lstRes2.empty()) {
        info->fid = std::stoi(lstRes2.front());
    } else {
        sqlLog << "select f_id fail" << sqlbuf << std::endl;
    }

    // 插入用户文件关系
    sprintf(sqlbuf,
            "INSERT INTO t_user_file(u_id, f_id, f_dir, f_name, f_uploadtime)"
            " VALUES(%d, %d, '%s', '%s', '%s');",
            rq->userid, info->fid, rq->dir, rq->fileName, rq->time);
    if (!m_sql->UpdataMysql(sqlbuf)) sqlLog << "insert user_file fail" << sqlbuf << std::endl;

    // 发送上传完成回复，仍使用 serverReqId
    STRU_UPLOAD_FILE_RS rs;
    rs.fileid = info->fid;
    rs.result = 1;
    rs.timestamp = serverReqId;
    rs.userid = rq->userid;
    SendData(clientfd, reinterpret_cast<char*>(&rs), sizeof(rs));
}
void CLogic::FileContentRq(sock_fd clientfd ,char* szbuf,int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_FILE_CONTENT_RQ*rq=(STRU_FILE_CONTENT_RQ*)szbuf;
    rq->content;
    rq->fileid;
    rq->len;
    rq->timestamp;
    rq->userid;
    //获取文件信息
    int64_t user_time=rq->userid*GetTenBillion()+rq->timestamp;
    FileInfo*info=nullptr;
    if(!m_UidTimeToFileinfo.find(user_time,info)){
        cout<<"file not find"<<endl;
        return;
    }
    STRU_FILE_CONTENT_RS rs;
    //写入
    int len=write(info->filefd,rq->content,rq->len);
    if(len==-1){
        perror("write file error");
        return;
    }
    if(len!=rq->len){
         //失败  跳回到读取之前
        rs.result=0;
        cout<<"写入失败"<<endl;
        cout<<"要求写入："<<rq->len<<" 实际写入:"<<len<<endl;
        lseek(info->filefd,-1*len,SEEK_CUR);
    }else{
        //成功  pos更新位置
        rs.result=1;
        info->pos+=len;
            //看是否到达末尾
            if(info->pos>=info->size){
                //更新数据库,把文件信息的状态更新为1,表示已完成
                char sqlbuf[1000];
                sprintf(sqlbuf,"update t_file set f_state=1 where f_id=%d",rq->fileid);
                bool res=m_sql->UpdataMysql(sqlbuf);
                // 创建转移任务
                //  1. 选择写入策略：可以替换为 NoOpWriter、FileWriter、MQWriter……
                auto writer = std::make_shared<FileWriter>("transfer_list.txt");
                // 2. 构造任务
                FileTransferTask task(writer);
                // 3. 添加待转移项（文件名或描述符）
                task.add(info->filefd); // 传入文件名
                // 4. 执行
                task.execute();
                
                if(res==0){
                    cout<<"updata fail:"<<sqlbuf<<endl;
                }
                close(info->filefd);
                //回收map节点
                m_UidTimeToFileinfo.erase(user_time);
                delete info;
                info=nullptr;
                
            }
    }
     //返回结果
        rs.fileid=rq->fileid;
        //失败所跳回的长度，
        rs.len=rq->len;
        rs.timestamp=rq->timestamp;
        rs.userid=rq->userid;
    SendData(clientfd,(char*)&rs,sizeof(rs));
}

void CLogic::GetFileInfoRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_GET_FILE_INFO_RQ *rq=(STRU_GET_FILE_INFO_RQ*)szbuf;
    //以获取以下2个变量
    rq->userid;
    rq->dir;
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_id,f_name,f_size,f_uploadtime,f_type from user_file_info where u_id=%d and f_dir='%s' and f_state=1;",rq->userid,rq->dir);
    //and f_state=1
    list<string>lstRes;
    bool res=m_sql->SelectMysql(sqlbuf,5,lstRes);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl;
    }
    if(lstRes.size()==0){
        cout<<"select fail,size==0:"<<sqlbuf<<endl;
        return;
    }
    //cout<<"select success: "<<sqlbuf<<endl;
    int count=lstRes.size()/5;
    //写回复包
    int packlen=sizeof(STRU_GET_FILE_INFO_RS)+count*sizeof(STRU_FILE_INFO);
    STRU_GET_FILE_INFO_RS*rs=(STRU_GET_FILE_INFO_RS*)malloc(packlen);
    rs->init();
    rs->count=count;
    strcpy(rs->dir,rq->dir);
    //rs->fileInfo

    for(int i=0;i<count;++i){
        int f_id=stoi(lstRes.front());
        lstRes.pop_front();
        string name=lstRes.front();
        lstRes.pop_front();
        int size=stoi(lstRes.front());
        lstRes.pop_front();
        string time=lstRes.front();
        lstRes.pop_front();
        string f_type=lstRes.front();
        lstRes.pop_front();
        rs->fileinfo[i].fileid=f_id;
        strcpy(rs->fileinfo[i].filetype,f_type.c_str());
        strcpy(rs->fileinfo[i].name,name.c_str());
        strcpy(rs->fileinfo[i].time,time.c_str());
        rs->fileinfo[i].size=size;
    }
    //根据id dir 查表(视图) 获取文件信息
    //STRU_GET_FILE_INFO_RS *rs=
    //写回复包

    //发送
    SendData(clientfd,(char*)rs,packlen);
    //回收

    free(rs);
}

void CLogic::DownloadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_DOWNLOAD_FILE_RQ*rq=(STRU_DOWNLOAD_FILE_RQ*)szbuf;
    // rq->dir;
    // rq->fileid;
    // rq->timestamp;
    // rq->userid;

    //查数据库  f_name,f_path,f_md5,f_size 如果没有    返回
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_name,f_path,f_md5,f_size from user_file_info where u_id=%d and f_dir='%s' and f_id=%d;",rq->userid,rq->dir,rq->fileid);
    list<string>lstRes;
    bool res=m_sql->SelectMysql(sqlbuf,4,lstRes);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    if(lstRes.size()==0){//empty    返回
        cout<<"member==0 fail "<<endl;
        return;
    }
    string strName=lstRes.front();lstRes.pop_front();
    string strPath=lstRes.front();lstRes.pop_front();
    string strMD5=lstRes.front();lstRes.pop_front();
    int size=stoi(lstRes.front());lstRes.pop_front();
    //有,写文件信息        
    FileInfo *info=new FileInfo;
    info->absolutePath=strPath;
    info->dir=rq->dir;
    info->fid=rq->fileid;
    
    info->md5=strMD5;
    info->name=strName;
    info->size=size;
    info->type="file";
    

    info->filefd=open(info->absolutePath.c_str(),O_RDONLY);
    if(info->filefd<=0){
        cout<<" file open fail first"<<endl;
        cout<<"尝试从分布式文件系统中获取"<<endl;
        //TODO:通知转移服务，从分布式文件系统中获取文件
        
        if(-1==FileTran.downloadFile(strMD5)){//服务器中文件名和MD5相同
            cout<<"尝试从分布式文件系统中获取失败"<<endl;
            return;
        }else{
            info->filefd=open(info->absolutePath.c_str(),O_RDONLY);//获取后重新打开
        }
    }

    //求key
    int64_t   user_time=rq->userid*GetTenBillion()+rq->timestamp;
    //写到map中
    m_UidTimeToFileinfo.insert(user_time,info);
    //发送文件头请求
    STRU_FILE_HEADER_RQ headrq;
    strcpy(headrq.dir,rq->dir);
    headrq.fileid=rq->fileid;
    strcpy(headrq.fileName,info->name.c_str());
    strcpy(headrq.md5,info->md5.c_str());
    headrq.size=info->size;
    strcpy(headrq.fileType,"file");
    headrq.timestamp=rq->timestamp;

    SendData(clientfd,(char*)&headrq,sizeof(headrq));

}
void CLogic::FileHeaderRs(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_FILE_HEADER_RS*rs=( STRU_FILE_HEADER_RS*)szbuf;
    rs->fileid;
    rs->result;
    rs->timestamp;
    rs->userid;
    
    //拿到文件信息
    int64_t user_time =rs->userid*GetTenBillion()+rs->timestamp;
    FileInfo*info=nullptr;
    if(!m_UidTimeToFileinfo.find(user_time,info)){
        return;
    }
    //发文件内容请求
    STRU_FILE_CONTENT_RQ rq;
    //读文件
    rq.len=read(info->filefd,rq.content,_DEF_BUFFER);
    if(rq.len<0){
        perror("read file error");
        return;
    }
    rq.fileid=rs->fileid;
    rq.timestamp=rs->timestamp;
    rq.userid=rs->userid;

    //文件块，文件内容
    SendData(clientfd,(char*)&rq,sizeof(rq));
}
void CLogic::FileContentRs(sock_fd clientfd, char *szbuf, int nlen){
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_FILE_CONTENT_RS*rs=(STRU_FILE_CONTENT_RS*)szbuf;
    rs->fileid;
    rs->len;
    rs->result;
    rs->timestamp;
    rs->userid;
    //文件信息结构
    int64_t user_time=rs->userid*GetTenBillion()+rs->timestamp;
    FileInfo*info=nullptr;
    if(!m_UidTimeToFileinfo.find(user_time,info)){
        return;
    }
    //判断是否成功
    if(rs->result!=1){
        //不成功 跳回
        lseek(info->filefd,-1*(rs->len),SEEK_CUR);
        cout<<"client写入失败"<<endl;
        cout<<"已写入："<<rs->len<<endl;
        cout<<"文件大小："<<info->size<<endl;
    }else{
        info->pos+=rs->len;
        //成功
        //是否结束结束
        if(info->pos>=info->size){
            //关闭文件
            close(info->filefd);
            m_UidTimeToFileinfo.erase(user_time);
            delete info;
            info=nullptr;
            return;
        }    
    }
    //写文件内容请求
    STRU_FILE_CONTENT_RQ rq;

    //读文件
    rq.len=read(info->filefd,rq.content,_DEF_BUFFER);
    if(rq.len==0){//前面已经关闭文件，所以包括=0的情况
        return;
    }
    if(rq.len<0){
        perror("read fail");
        return;
    }

    rq.fileid=rs->fileid;
    rq.timestamp=rs->timestamp;
    rq.userid=rs->userid;
    //发送

    SendData(clientfd,(char*)&rq,sizeof(rq));
}
#include <filesystem>
void CLogic::AddFolderRq(sock_fd clientfd ,char* szbuf,int nlen){
   // 拆包
    STRU_ADD_FOLDER_RQ* rq = reinterpret_cast<STRU_ADD_FOLDER_RQ*>(szbuf);

    // 准备返回包
    STRU_ADD_FOLDER_RS rs;
    rs.result    = 0;               // 默认失败
    rs.timestamp = rq->timestamp;   
    rs.userid    = rq->userid;
    strcpy(rs.userAbsPath, rq->userAbsPath);
    strcpy(rs.dir, rq->dir);
    //数据库写表，插入文件信息 
    // //f_size,f_path,f_count,f_MD5,f_state,f_type
    
    //解决文件路径可能过长的问题
   
    // 1. 构建完整目录路径:但是不熟悉这个函数拼接错误
    // namespace fs = std::filesystem;
    // fs::path basePath(_DEF_PATH);
    // basePath /= std::to_string(rq->userid);
    // basePath /= rq->dir;
    // basePath /= rq->fileName;
    // std::string fullPath = basePath.string();
     char pathbuf[1000]="";
    //拼接绝对路径
    sprintf(pathbuf,"%s%d%s%s",_DEF_PATH,rq->userid,rq->dir,rq->fileName);
    //2.创建目录，告诉客户端创建失败
    if(mkdir(pathbuf,0777)==-1){
        logicLog << "mkdir fail, " << " path=" << pathbuf << endl;
        SendData(clientfd, reinterpret_cast<char*>(&rs), sizeof(rs));
        return;
    }
       // 3. 插入 t_file 表
    char sqlbuf[1024] = {0};
    sprintf(sqlbuf,
        "INSERT INTO t_file(f_size,f_path,f_count,f_MD5,f_state,f_type) "
        "VALUES(0,'%s',0,'?',1,'folder');",
        pathbuf);
    if (!m_sql->UpdataMysql(sqlbuf)) {
        logicLog << "t_file insert fail: " << sqlbuf << endl;
        SendData(clientfd, reinterpret_cast<char*>(&rs), sizeof(rs));
        return;
    }
    // 4. 查询新插入的 f_id
    sprintf(sqlbuf, "SELECT f_id FROM t_file WHERE f_path='%s';", pathbuf);
    std::list<std::string> lstRes;
    if (!m_sql->SelectMysql(sqlbuf, 1, lstRes) || lstRes.empty()) {
        logicLog << "t_file select f_id fail: " << sqlbuf << endl;
        SendData(clientfd, reinterpret_cast<char*>(&rs), sizeof(rs));
        return;
    }
    int fid = std::stoi(lstRes.front());

    // 5. 写入 t_user_file
    sprintf(sqlbuf,
        "INSERT INTO t_user_file(u_id,f_id,f_dir,f_name,f_uploadtime) "
        "VALUES(%d,%d,'%s','%s','%s');",
        rq->userid, fid, rq->dir, rq->fileName, rq->time);
    if (!m_sql->UpdataMysql(sqlbuf)) {
        logicLog << "t_user_file insert fail: " << sqlbuf << endl;
        SendData(clientfd, reinterpret_cast<char*>(&rs), sizeof(rs));
        return;
    }
    // 6. 全部成功
    rs.result = 1;
    SendData(clientfd, reinterpret_cast<char*>(&rs), sizeof(rs));
}
void CLogic::ShareFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_SHARE_FILE_RQ*rq=(STRU_SHARE_FILE_RQ*)szbuf;
    
    //随机生成分享链接
    //分享码规则    9位分享码
    int link=0;
    do{
        link =1+random()%9;     //随机出1——9
        link *=100000000;
        link+=random()%100000000;
        //去重 查链接是否已经存在
        char sqlbuf[1000]="";
        sprintf(sqlbuf,"select s_link from t_user_file where s_link =%d ;",link);
        list<string>lstRes;
        bool res=m_sql->SelectMysql(sqlbuf,1,lstRes);
        if(!res){
            cout<<"select fail: "<<sqlbuf<<endl;
            return ;
        }
        if(lstRes.size()>0){
            link=0;
        }
    }while(link==0);   

    //遍历所有文件，将其分享链接设置
    int itemCount=rq->itemCount;
    for(int i=0;i<itemCount;i++){
        ShareItem(rq->userid,rq->fileidArray[i],rq->dir,rq->shareTime,link);

    }
    //写回复
    STRU_SHARE_FILE_RS rs;
    rs.result=link;
    SendData(clientfd,(char*)&rs,sizeof(rs));
}

void CLogic::MyShareRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_MY_SHARE_RQ * rq=(STRU_MY_SHARE_RQ *)szbuf;

    //根据id查表获得分享文件列表
    //查的内容  f_name f_size f_linkTime s_link
    //select f_name ,f_size,s_linkTime,s_link from NetDisk.user_file_info where s_link is not null and s_linkTime is not null;
    char sqlbuf[1000];
    sprintf(sqlbuf,"select f_name ,f_size,s_linkTime,s_link from NetDisk.user_file_info where u_id=%d and s_link is not null and s_linkTime is not null;",rq->userid);
    list<string>lst;
    bool res=m_sql->SelectMysql(sqlbuf,4,lst);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    int count=lst.size();
    if((count/4)==0||(count%4!=0)){
        cout<<"查找分享列表失败，当前count非4的倍数："<<count<<endl;
        cout<<"sql: "<<sqlbuf<<endl;
        return;
    }
    count/=4;
    //写回复
    int packlen=sizeof(STRU_MY_SHARE_RS)+count*sizeof(STRU_MY_SHARE_FILE);
    
    STRU_MY_SHARE_RS*rs=(STRU_MY_SHARE_RS*)malloc(packlen);
    rs->init();
    rs->itemCount=count;
    for(int i=0;i<count;i++){
            string name=lst.front();lst.pop_front();
            int size=stoi(lst.front());lst.pop_front();
            string time=lst.front();lst.pop_front();
            int link=stoi(lst.front());lst.pop_front();

            strcpy(rs->items[i].name,name.c_str());
            rs->items[i].size=size;
            strcpy(rs->items[i].time,time.c_str());
            rs->items[i].shareLink=link;

    }
    //发送
    //cout<<"send right"<<endl;
    SendData(clientfd,(char*)rs,packlen);

    
    free(rs);
}
void CLogic::ShareItem(int userid,int fileid,string dir,string time,int link)
{
    cout<<__func__<<endl;
    char sqlbuf[1000]="";
    //string转化成char*
    sprintf(sqlbuf,"update t_user_file set s_link = '%d' , s_linkTime = '%s' where u_id = %d and f_id = %d and f_dir = '%s';",link,time.c_str(),userid,fileid,dir.c_str());
    list<string>lstRes;
    bool res=m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout<<"updata fail: "<<sqlbuf<<endl;
        return ;
    }
    //如果分享的是文件夹，就分享这个文件夹，
    //等到获取分享的时候时再去获取文件，把所有文件加到文件夹的路径下面
    //这样分享时就没有了预览
}
void CLogic::GetShareRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_GET_SHARE_RQ*rq=(STRU_GET_SHARE_RQ*)szbuf;
    cout<<"路径为"<<rq->dir<<endl;
    rq->shareLink;
    rq->time;
    rq->userid;
    //根据分享码，查询到一系列文件
    //查询属性  f_id f_name f_dir(分享人的) f_type u_id(分享人的)
    //select f_id,f_name,f_dir,f_type,u_id from user_file_info where s_link=%d; 
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_id,f_name,f_dir,f_type,u_id from user_file_info where s_link=%d;",rq->shareLink);
    list<string>lst;
    //mark
    bool res=m_sql->SelectMysql(sqlbuf,5,lst);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    STRU_GET_SHARE_RS rs;
    if(lst.size()==0){
        rs.result=0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
    }
    rs.result=1;
    //遍历文件列表
    if(lst.size()%5!=0){
        //rs.result=0;
        return;
    }
    while(lst.size()!=0){
        //cout<<"select success: "<<sqlbuf<<endl;
        int fileid=stoi(lst.front());lst.pop_front();
        string name=lst.front();lst.pop_front();
        string fromdir=lst.front();lst.pop_front();
        string type=lst.front();lst.pop_front();
        int fromuserid=stoi(lst.front());lst.pop_front();
        //如果是文件
        if(type=="file"){
            //插入信息到用户文件关系表
            GetShareByFile(rq->userid,fileid,rq->dir,name,rq->time);//insert into t_user_file
        }else{
        //如果是文件夹
            //插入信息到用户文件关系表
            //拼接路径 获取人路径 /-》06 分享人的目录 /-》/06/
            //根据新路径 在分享人那边查询 文件夹下的文件
            //遍历列表 递归

            //
            GetShareByFolder(rq->userid,fileid,rq->dir,name,rq->time,fromuserid,fromdir);
        }
    }
    //写回复包
    strcpy(rs.dir,rq->dir);
    //发送
    SendData(clientfd,(char*)&rs,sizeof(rs));
}
void CLogic::GetShareByFile(int userid,int fileid,string dir,string name,string time){
    cout<<__func__<<endl;
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"insert into t_user_file ( u_id , f_id , f_dir , f_name , f_uploadtime ) values( %d , %d , '%s' , '%s' , '%s' );",userid,fileid,dir.c_str(),name.c_str(),time.c_str());
    //不写c_str()会出现问题
    std::cout<<"success"<<sqlbuf<<endl;
    bool res=m_sql->UpdataMysql(sqlbuf);
    //std::cout<<sqlbuf<<"下半部"<<endl;
    if(!res){
        sqlLog<<sqlbuf<<endl;
    }
}


void CLogic::DownloadFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_DOWNLOAD_FOLDER_RQ*rq=(STRU_DOWNLOAD_FOLDER_RQ*)szbuf;
    //rq->dir;
    //rq->fileid;
    rq->userid;
    //查数据库表 拿到信息 查的属性:name path md5 size--->folder
    //f_id,f_name,f_path,f_MD5,f_size,f_dir,f_type
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type,f_id,f_name,f_path,f_MD5,f_size,f_dir from user_file_info where u_id=%d and f_dir='%s' and f_id=%d;",rq->userid,rq->dir,rq->fileid);
    list<string>lstRes;
    bool res=m_sql->SelectMysql(sqlbuf,7,lstRes);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    if(lstRes.size()==0){//empty    返回
        cout<<"member==0,fail "<<endl;
        return;
    }
    string type=lstRes.front();lstRes.pop_front();
    int timestamp=rq->timestamp;
    //下载文件夹
    DownloadFolder( rq->userid,timestamp,clientfd,lstRes);
}

void CLogic::DownloadFolder(int userid,int& timestamp,sock_fd clientfd,list<string>&lstRes){
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    int fileid=stoi(lstRes.front());lstRes.pop_front();
    string strName=lstRes.front();lstRes.pop_front();
    string strPath=lstRes.front();lstRes.pop_front();
    string strMD5=lstRes.front();lstRes.pop_front();
    int size=stoi(lstRes.front());lstRes.pop_front();
    string dir=lstRes.front();lstRes.pop_front();
    //string type=lstRes.front();lstRes.pop_front();

    //发送创建文件夹请求
    STRU_FOLDER_HEADER_RQ rq;
    rq.timestamp=++timestamp; //时间戳处理
    strcpy(rq.dir,dir.c_str());
    rq.fileid=fileid;
    strcpy(rq.fileName,strName.c_str());
    SendData(clientfd,(char*)&rq,sizeof(rq));
    //拼接路径
    string newDir=dir+strName+"/";
    //查询newdir userid所有文件信息（包含type）
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type,f_id,f_name,f_path,f_MD5,f_size,f_dir from user_file_info where u_id=%d and f_dir='%s';",userid,newDir.c_str());
    list<string>newlstRes;
    bool res=m_sql->SelectMysql(sqlbuf,7,newlstRes);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    while(newlstRes.size()!=0){
        string type=newlstRes.front();newlstRes.pop_front();
        if(type=="file"){
            //如果是文件 下载文件流程
            DownloadFile(userid,timestamp,clientfd,newlstRes);
        }else{
            //如果是文件夹 递归
            DownloadFolder(userid,timestamp,clientfd,newlstRes);
        } 
    }
}

void CLogic::ContinueUploadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_CONTINUE_UPLOAD_RQ*rq=(STRU_CONTINUE_UPLOAD_RQ*)szbuf;
    int64_t user_time=rq->userid*GetTenBillion()+rq->timestamp;

    //需要看map中是否存在
    FileInfo * info=nullptr;
    if(!m_UidTimeToFileinfo.find(user_time,info)){
        //不存在 创建
        info=new FileInfo;
        info->dir=rq->dir;
        info->fid=rq->fileid;
        info->type="file";//文件夹不续传
        //查表 获取信息 给info赋值 然后打开文件 info加到map中
        //查4列 分别是f_path,f_MD5,f_name,f_size
        char sqlbuf[1000]="";
        sprintf(sqlbuf,"select f_name,f_path,f_size,f_MD5 from user_file_info where u_id=%d and f_id=%d and f_dir='%s' and f_state=0;",rq->userid,rq->fileid,rq->dir);
        list<string>lstRes;
        cout<<"test is or not wrong from select"<<endl;
        bool res=m_sql->SelectMysql(sqlbuf,4,lstRes);
        cout<<"no"<<endl;
        if(!res){
            cout<<"select fail: "<<sqlbuf<<endl;
            return;
        }
        if(lstRes.size()==0){
            cout<<"ContinueUploadRq res无结果"<<sqlbuf<<endl;
            return;
        }
        info->name=lstRes.front();lstRes.pop_front();
        info->absolutePath=lstRes.front();lstRes.pop_front();
        info->size=stoi(lstRes.front());lstRes.pop_front();
        info->md5=lstRes.front();lstRes.pop_front();

        //给info赋值 然后打开文件 info加到map
        info->filefd=open(info->absolutePath.c_str(),O_WRONLY);
        if(info->filefd<=0){
            cout<<"file open fail : "<<errno<<endl;
            return ;
        }
        m_UidTimeToFileinfo.insert(user_time,info);
    }    
    //现在已经有信息了 lseek 跳转并读取文件当前写的位置（文件末尾） 更新 pos 文件跳转
    info->pos=lseek(info->filefd,0,SEEK_END);

    //写回复 返回
    STRU_CONTINUE_UPLOAD_RS rs;
    rs.fileid=rq->fileid;
    rs.pos=info->pos;
    rs.timestamp=rq->timestamp;
    //rs.type;
    SendData(clientfd,(char*)&rs,sizeof(rs));

}

void CLogic::ContinueDownloadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    //拆包
    STRU_CONTINUE_DOWNLOAD_RQ * rq=(STRU_CONTINUE_DOWNLOAD_RQ *)szbuf;
    //看是否存在文件信息
    FileInfo *info=nullptr;
    int64_t user_time=rq->userid*GetTenBillion()+rq->timestamp;

    if(!m_UidTimeToFileinfo.find(user_time,info)){
        //没有 创建文件信息 由查表而来 添加到map
        info=new FileInfo;
        //info;
        //查数据库信息
        //查数据库  f_name,f_path,f_md5,f_size 如果没有    返回
        char sqlbuf[1000]="";
        sprintf(sqlbuf,"select f_name,f_path,f_md5,f_size from user_file_info where u_id=%d and f_dir='%s' and f_id=%d;",rq->userid,rq->dir,rq->fileid);
        list<string>lstRes;
        bool res=m_sql->SelectMysql(sqlbuf,4,lstRes);
        if(!res){
            cout<<"select fail: "<<sqlbuf<<endl;
            return;
        }//empty    返回
        if(lstRes.size()==0){
            cout<<"member==0 fail "<<endl;
            return;
        }
        string strName=lstRes.front();lstRes.pop_front();
        string strPath=lstRes.front();lstRes.pop_front();
        string strMD5=lstRes.front();lstRes.pop_front();
        int size=stoi(lstRes.front());lstRes.pop_front();
        //有,写文件信息        
        //FileInfo *info=new FileInfo;
        info->absolutePath=strPath;
        info->dir=rq->dir;
        info->fid=rq->fileid;
        info->md5=strMD5;
        info->name=strName;
        info->size=size;
        info->type="file";
    
        info->filefd=open(info->absolutePath.c_str(),O_RDONLY);
        if(info->filefd<=0){
            cout<<"file open fail"<<endl;
            return;
        }
        //求key
        int64_t   user_time=rq->userid*GetTenBillion()+rq->timestamp;
        //写到map中
        m_UidTimeToFileinfo.insert(user_time,info);

        //m_UidTimeToFileinfo.insert(user_time,info);
    }
    //现在有信息了   
    //文件指针跳转 pos位置
    lseek(info->filefd,rq->pos,SEEK_SET);//set是起始位置
    info->pos=rq->pos;
    //读文件块 发送文件块请求
    STRU_FILE_CONTENT_RQ contentRq;
    contentRq.fileid=rq->fileid;
    contentRq.timestamp=rq->timestamp;
    contentRq.userid=rq->userid;
    //contentRq.content;
    contentRq.len=read(info->filefd,contentRq.content,_DEF_BUFFER);
    SendData(clientfd,(char*)&contentRq,sizeof(contentRq));
}

void CLogic::DownloadFile(int userid,int &timestamp,sock_fd clientfd,list<string>&lstRes){
    logicLog<<"clientfd:"<<"userId:"<< clientfd <<" "<< __func__ << endl;
    int fileid=stoi(lstRes.front());lstRes.pop_front();
    string strName=lstRes.front();lstRes.pop_front();
    string strPath=lstRes.front();lstRes.pop_front();
    string strMD5=lstRes.front();lstRes.pop_front();
    int size=stoi(lstRes.front());lstRes.pop_front();
    string dir=lstRes.front();lstRes.pop_front();
    //string type=lstRes.front();lstRes.pop_front();

    //fileid
    //timestamp
    //clientfd
    //有,写文件信息        
    FileInfo *info=new FileInfo;
    info->absolutePath=strPath;
    info->dir=dir;
    info->fid=fileid;
    
    info->md5=strMD5;
    info->name=strName;
    info->size=size;
    info->type="file";
    info->filefd=open(info->absolutePath.c_str(),O_RDONLY);
    if(info->filefd<=0){
        cout<<"file open fail"<<endl;
        return;
    }
   //求key
   int64_t   user_time=userid*GetTenBillion()+(++timestamp);
   //写到map中
    m_UidTimeToFileinfo.insert(user_time,info);
    //发送文件头请求
    STRU_FILE_HEADER_RQ headrq;
    strcpy(headrq.dir,dir.c_str());
    headrq.fileid=fileid;
    strcpy(headrq.fileName,info->name.c_str());
    strcpy(headrq.md5,info->md5.c_str());
    headrq.size=info->size;
    strcpy(headrq.fileType,"file");
    headrq.timestamp=timestamp;

    SendData(clientfd,(char*)&headrq,sizeof(headrq));
}

void CLogic::DeleteFileRq(sock_fd clientfd, char *szbuf, int nlen){
    //拆包
    STRU_DELETE_FILE_RQ*rq=(STRU_DELETE_FILE_RQ*)szbuf;
    //id列表
    //dir
    //fileCount;
    //userid;
    
    for(int i=0;i<rq->fileCount;i++){
        int fileid=rq->fileidArray[i];
        //删除每一项
        DeleteOneItem(rq->userid,fileid,rq->dir);
    }
    

    //写回复
    STRU_DELETE_FILE_RS rs;
    rs.result=1;
    strcpy(rs.dir,rq->dir);
    SendData(clientfd,(char*)&rs,sizeof(rs));
}

void CLogic::DeleteOneItem(int userid,int fileid,string dir){
    cout<<__func__<<endl;
    //删除文件需要 u_id f_dir f_id
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type,f_name,f_path from user_file_info where u_id=%d and f_id=%d and f_dir='%s';",userid,fileid,dir.c_str());
    list<string>lst;
    bool res=m_sql->SelectMysql(sqlbuf,3,lst);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    if(lst.size()==0){
        return;
    }
    string type=lst.front();lst.pop_front();
    string name=lst.front();lst.pop_front();
    string path=lst.front();lst.pop_front();
    if(type=="file"){
        DeleteFile(userid,fileid,dir,path);
    }else{
        DeleteFolder(userid,fileid,dir,name);
    }
    //需要知道到底是什么类型 type name  path
        //删除文件不需要name名字，文件夹需要

}

void CLogic::DeleteFile(int userid,int fileid,string dir,string path){
     cout<<__func__<<endl;
    //删除用户文件对应的关系
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"delete from t_user_file where u_id=%d and f_id=%d and f_dir='%s';",userid,fileid,dir.c_str());
    bool res=m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout<<"delete fail: "<<sqlbuf<<endl;
        return;
    }
    
    //再次查询id看能不能找到数据库中有这个记录，如果不能，删除这个文件
    sprintf(sqlbuf,"select f_id from t_file where f_id=%d ;",fileid);
    list<string>lst;
    res=m_sql->SelectMysql(sqlbuf,1,lst);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    //if(lst.size()==0){
        cout<<path<<endl;
        unlink(path.c_str());//文件io，删除文件
    //}

}

void CLogic::DeleteFolder(int userid,int fileid,string dir,string name){
     cout<<__func__<<endl;
    //删除用户文件对应的关系 u_id f_dir f_id
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"delete from t_user_file where u_id=%d and f_id=%d and f_dir='%s';",userid,fileid,dir.c_str());
    bool res=m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout<<"delete fail: "<<sqlbuf<<endl;
        return;
    }
    //拼接新路径
    string newdir=dir+name+"/";

    //查表，根据新路径查表，得到列表 f_id,f_type,name,path
    //char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type,f_id,f_name,f_path from user_file_info where u_id=%d and f_dir='%s';",userid,dir.c_str());
            //newdir.c_str());
    list<string>lst;
    res=m_sql->SelectMysql(sqlbuf,4,lst);
    if(!res){
        cout<<"select fail: "<<sqlbuf<<endl;
        return;
    }
    if(lst.size()==0){
        return;
    }
    //循环判定
    while(lst.size()!=0){
        string type=lst.front();lst.pop_front();
        int fileid=stoi(lst.front());lst.pop_front();
        string name=lst.front();lst.pop_front();
        string path=lst.front();lst.pop_front(); 
        if(type=="file") {
            //如果是文件
        DeleteFile(userid,fileid,newdir,path);
        }
        else{
            //如果是文件夹
        DeleteFolder(userid,fileid,newdir,name);
        }
        
    }
    
}

void CLogic::GetShareByFolder(int userid,int fileid,string dir,string name,string time,int fromuserid,string fromdir){
    cout<<__func__<<endl;
    //如果是文件夹
        //插入信息到用户文件关系表
        GetShareByFile(userid,fileid,dir,name,time);
        //拼接路径
        //获取人路径 /-》06
        string newDir=dir+name+"/";
        //分享人的目录 /-》/06/
        string newfromdir=fromdir+name+"/";
        //根据新路径 在分享人那边查询 文件夹下的文件
        char sqlbuf[1000]="";
        sprintf(sqlbuf,"select f_id,f_name,f_type from user_file_info where u_id=%d and f_dir='%s';",fromuserid,newfromdir.c_str());
        list<string>lst;
        bool res=m_sql->SelectMysql(sqlbuf,3,lst);
        if(!res){
            cout<<"select fail: "<<endl;
            return;
        }
        //遍历列表 递归
        while (lst.size()!=0)
        {
            int fileid=stoi(lst.front());lst.pop_front();
            string name=lst.front();lst.pop_front();
            string type=lst.front();lst.pop_front();
            //是文件
            if(type=="file"){
                GetShareByFile(userid,fileid,newDir,name,time);
            }else{
            //是文件夹，递归
                GetShareByFolder(userid,fileid,newDir,name,time,fromuserid,newfromdir);
            }
        }
}

