#include "tcpworker.h"
#include<QFile>


TcpWorker::TcpWorker(QString logId)
{

    mlogId = logId;
    // qDebug()<<"TcpWorker thread thread:"<<QThread::currentThreadId();
    initVariable();
    readDataBuff.clear();
    audioSrc = new QFile("playAudioSrc.pcm");
    if (!audioSrc->open(QIODevice::ReadOnly  |QIODevice::WriteOnly))
        return;
}

void TcpWorker::initVariable()
{
    videoFrameMaxLen = 1024 * 3500;
    audioFrameMaxLen = 160;

    isReconnecting = false;
    minLen = 32;
    tcpSocket = nullptr;

    isStartParsing = false;

    isFindHead = false;
    isFindMediaType = false;
    isSaveAudioInfo = false;
    isSaveVideoInfo = false;
    isCheckedDataLong = false;
    isConnected = false;
    isHavaData = false;
    isSendAudioData = false;

    mediaDataType = -1;
    m_streamDateLen = -1;

    timerConnectSer = nullptr;
}

void TcpWorker::creatNewTcpConnect(QString ip, int port)
{

    qDebug()<<"creatNewTcpConnect  currentThread:"<<QThread::currentThreadId();
    qDebug()<<"tcp开始连接  "+ip+"  "+QString::number(port);
    this->ip = ip;
    this->port = port;
    if(tcpSocket == nullptr){
        tcpSocket = new QTcpSocket;
        timerConnectSer = new QTimer;
        connect(tcpSocket,&QTcpSocket::connected,this,&TcpWorker::slot_tcpConnected);
        connect(tcpSocket,&QTcpSocket::readyRead,this,&TcpWorker::slot_readData);
        connect(tcpSocket,&QTcpSocket::disconnected,this,&TcpWorker::slot_tcpDisconnected);
        connect(timerConnectSer,&QTimer::timeout,this,&TcpWorker::slot_timerConnectSer);

        tcpSocket->setReadBufferSize(1024*1024);
        tcpSocket->connectToHost(this->ip,this->port);
        timerConnectSer->start(5000);
    }else{

        tcpSocket->connectToHost(this->ip,this->port);
        timerConnectSer->start(5000);
    }

    writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"tcp连接创建完成");
    qDebug()<<"tcp连接创建完成"+m_did;
}

void TcpWorker::slot_disConnectSer()
{
    isConnected = false;
}

//定时对tcp做连接 以达到自动重连的目的
void TcpWorker::slot_timerConnectSer()
{


    writeDebugfile(__FILE__,__FUNCTION__,__LINE__,tr("定时检测连接:")+QString::number(isHavaData));

    if(!isHavaData){//没有数据默认断开连接

        if(tcpSocket != nullptr){

            writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"tcp开始重连");
            tcpSocket->abort();
            readDataBuff.clear();
            tcpSocket->connectToHost(this->ip,this->port);
        }
    }

    isHavaData = false;
}



void TcpWorker::slot_tcpConnected()
{

    writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"tcp连接成功");
    isStartParsing = true;
    isConnected = true;
    slot_tcpSendAuthentication(m_did,m_usrName,m_password);
}

void TcpWorker::testState()
{
    /*
    enum SocketState {
        UnconnectedState,
        HostLookupState,
        ConnectingState,
        ConnectedState,
        BoundState,
        ListeningState,
        ClosingState
    };*/
    qDebug()<<mlogId<<" "<<tcpSocket->isOpen()<<"   "<<tcpSocket->state()<<"   "<<timerConnectSer->isActive();
}

void TcpWorker::slot_tcpDisconnected()
{
    isConnected = false;
}

void TcpWorker::slot_readData()
{

    isHavaData = true;
    readDataBuff.append(tcpSocket->readAll());
    isCheckedDataLong = false;
    if(readDataBuff.length() > videoFrameMaxLen*2){
        writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"readDataBuff 数据过长");
        isCheckedDataLong = true;
        //        QTextStream out(debugfile);
        //        out << " data is too long: " << readDataBuff.toHex() << "\n";
        resetAVFlag();
    }
    parseRecevieData();
}

int TcpWorker::saveVideoInfo(QByteArray &arr)
{
    int index = 0;
    int fps = 0x000f & arr.at(index++);
    int rcmode = 0x000f & arr.at(index++);
    int frameType = 0x000f & arr.at(index++);
    int staty0 = 0x000f & arr.at(index++);
    //VideoReslution_T

    QByteArray arrW = arr.mid(index,4);
    int width = byteArr2Int(arrW);
    index += 4;


    QByteArray arrH = arr.mid(index,4);

    int height = byteArr2Int(arrH);

    index += 4;

    //  bitrate
    QByteArray arrBitrate = arr.mid(index,4);
    int bitrate =  byteArr2Int(arrBitrate);
    index += 4;
    //pts


    // qDebug()<< "bitrate:"<<bitrate<<"    rcmode"<<rcmode<<" fps"<<fps <<" width:"<<width<<"    height "<<height;
    index += 8;
    //裸流数据长度

    QByteArray arrDatalen = arr.mid(index,4);

    int datalen = byteArr2Int(arrDatalen);
    index += 4;

    memcpy(&infoV,arr.data(),sizeof (QueueAudioInputInfo_T));
    arr.remove(0,index);

    return datalen;
}
int TcpWorker::saveAudioInfo(QByteArray &arr)
{

    //qDebug()<<"revice audio";
    int index = 0;
    QByteArray arrS = arr.mid(index,4);
    int samplerate = byteArr2Int(arrS);
    index += 4;

    QByteArray arrP = readDataBuff.mid(index,4);
    int prenum = byteArr2Int(arrP);
    index += 4;

    QByteArray arrB = readDataBuff.mid(index,4);
    int bitwidth = byteArr2Int(arrB);
    index += 4;

    QByteArray arrSoude = readDataBuff.mid(index,4);
    int soundmode = byteArr2Int(arrSoude);
    index += 4;

    QByteArray arrH = readDataBuff.mid(index,4);
    int highPts = byteArr2Int(arrH);
    index += 4;

    QByteArray arrL = readDataBuff.mid(index,4);
    int lowPts = byteArr2Int(arrL);
    index += 4;


    QByteArray arrDatalen = readDataBuff.mid(index,4);
    int datalen = byteArr2Int(arrDatalen);
    index += 4;

    memcpy(&infoA,arr.data(),sizeof (QueueAudioInputInfo_T));
    arr.remove(0,index);

    return datalen;
}

void TcpWorker::resetAVFlag()
{
    m_streamDateLen = -1;
    mediaDataType = -1;
    isFindHead = false;
    isSaveAudioInfo = false;
    isSaveVideoInfo = false;
    isFindMediaType = false;
}

void TcpWorker::parseRecevieData()
{
    int needlen = 2;

    /*
        循环解析整个readDataBuff，把所有有效的数据都解析出来
        每次找到一个信息都会把包含该信息的字节数据删除
        needlen代表解析数据还需要的字节长度
        1、找头，
        2、找媒体信息类型
        3、找媒体裸流数据（消息回应）
    */


   // writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"解析一次接收数据:"+QString::number(readDataBuff.length())+"   needlen:"+QString::number(needlen));

    while(readDataBuff.length() >= needlen)
    {
        //QMutexLocker lock(&parseMutex);

        //writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"while一次数据:"+QString::number(readDataBuff.length())+"   needlen:"+QString::number(needlen));

        if(!isStartParsing)
            break;

        if(!isFindHead)
        {

            if(readDataBuff.at(0) == D_SYNCDATA_HEAD0 && readDataBuff.at(1)==D_SYNCDATA_HEAD1)
            {

                readDataBuff.remove(0,2);
                isFindHead = true;
                needlen = 2;
            }else {
                readDataBuff.remove(0,1);
                continue;
            }
        }


        //找到头后，找媒体类型
        if(!isFindMediaType)
        {
            if(readDataBuff.length()>=needlen){
                int tmp = readDataBuff.at(1);
                mediaDataType =  tmp & 0x000000ff;
                readDataBuff.remove(0,2);


                if(mediaDataType >= MediaType_H264 && mediaDataType <= MediaType_MSG){

                    isFindMediaType = true;

                }else {//不合理的媒体类型则从新开始找头
                    resetAVFlag();
                    continue;
                }
            }
            else
                continue;
        }



        if(mediaDataType == MediaType_H264)
        {
            needlen = 28;

            if(!isSaveVideoInfo)
            {
                if(readDataBuff.length() >= needlen)
                {
                    m_streamDateLen = saveVideoInfo(readDataBuff);
                    isSaveVideoInfo = true;


                    if(m_streamDateLen > videoFrameMaxLen || m_streamDateLen <0)
                    {
                        qDebug()<<"视频帧数据长度异常:"<<m_streamDateLen;
                        resetAVFlag();
                        continue;
                    }
                }else
                    continue;
            }

            needlen = m_streamDateLen;

            if(readDataBuff.length()>=needlen)
            {

                //emit signal_writeMediaVideoQueue(readDataBuff.data(),m_streamDateLen,infoV,MediaDataProcess::mMediaVeidoType);
                quint64 ptsH = 0x00000000ffffffff & infoV.highPts;
                quint64 ptsL = 0x00000000ffffffff & infoV.lowPts;
                quint64 pts = ptsH *256 *255*256 + ptsL;


                //writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"解析到视频");
                // qDebug()<<"解析到视频";
                emit signal_sendH264(readDataBuff.data(),m_streamDateLen,pts);

                readDataBuff.remove(0,m_streamDateLen);
                resetAVFlag();

                needlen = 2;
                continue;

            }else{

                continue;
            }
        }else if(MediaType_G711A == mediaDataType){

            needlen = 28;
            if(!isSaveAudioInfo){

                if(readDataBuff.length() >= needlen)
                {
                    m_streamDateLen = saveAudioInfo(readDataBuff);
                    isSaveAudioInfo = true;

                    if(m_streamDateLen > audioFrameMaxLen)
                    {
                        qDebug()<<"音频数据长度异常:"<<m_streamDateLen;
                        resetAVFlag();
                        continue;
                    }


                }else
                    continue;
            }

            needlen = m_streamDateLen;
            if(m_streamDateLen > 0 && readDataBuff.length()>=needlen)
            {

                quint64 ptsH = 0x00000000ffffffff & infoA.highPts;
                quint64 ptsL = 0x00000000ffffffff & infoA.lowPts;
                quint64 pts = ptsH *256 *255*256 + ptsL;


                // writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"解析到音频");
                //qDebug()<<"解析到音频";
                emit signal_sendPcmALaw(readDataBuff.data(),m_streamDateLen,pts);

                QThread::msleep(5);

                readDataBuff.remove(0,m_streamDateLen);
                m_streamDateLen = 2;
                resetAVFlag();
                continue;
            }else
                continue;
        }
        else if(MediaType_MSG  == mediaDataType)
        {

            needlen = 136;
            if(readDataBuff.length() >= needlen){
                int index = 0;
                QByteArray arrlen = readDataBuff.mid(index,4);
                int datalen = byteArr2Int(arrlen);
                index += 4;

                QByteArray arrstatuscode = readDataBuff.mid(index,4);
                int statuscode = byteArr2Int(arrstatuscode);
                index += 4;

                QByteArray arrDid = readDataBuff.mid(index,128);

                readDataBuff.remove(0,136);
                needlen = 2;
                resetAVFlag();
                continue;
            }else {
                continue;
            }
        }
        resetAVFlag();
        needlen = 2;
    }

    //qDebug()<<"解析耗时："<<time.elapsed()<<"ms";
    if(!isStartParsing){
        deleteLater();
    }
}



int TcpWorker::byteArr2Int(QByteArray arr)
{

    int index = 0;
    int i1 = 0x000000ff & arr.at(index++);
    int i2 = 0x000000ff & arr.at(index++);
    int i3 = 0x000000ff & arr.at(index++);
    int i4 = 0x000000ff & arr.at(index++);

    return (i1 + i2*256 + i3*65536 + i4*16777216);
}


void TcpWorker::slot_tcpRecAuthentication(QString did,QString usrName,QString password)
{

    qDebug()<<"did usrName password:"<<did<<"   "<<usrName<<"   "<<password;
    m_did = did;
    m_usrName = usrName;
    m_password = password;
    // slot_timerConnectSer();
}

void TcpWorker::slot_tcpSendAuthentication(QString did,QString usrName,QString password)
{

    //qDebug()<<"slot_tcpSendAuthentication  currentThread:"<<QThread::currentThreadId();


    m_did = did;
    m_usrName = usrName;
    m_password = password;

    if(did != ""){
        QByteArray arr;

        unsigned int datelen = 128 + 64 +64;
        int datelen0 = (0x000000ff & datelen);
        int datelen1 = (0x000000ff & (datelen>>8));
        int datelen2 = (0x000000ff & (datelen>>16));
        int datelen3 = (0x000000ff & (datelen>>24));

        arr.append(D_SYNCDATA_HEAD0);
        arr.append(D_SYNCDATA_HEAD1);
        arr.append(Msg_GetPlay);
        arr.append(MediaType_MSG);
        arr.append(datelen0);
        arr.append(datelen1);
        arr.append(datelen2);
        arr.append(datelen3);
        arr.append(did);
        arr.append(128-did.size(),0);
        arr.append(usrName);
        arr.append(64-usrName.size(),0);
        arr.append(password);
        arr.append(64-password.size(),0);

        int writeLen = tcpSocket->write(arr.data(),arr.length());

        qDebug()<<"发送鉴权:"<<writeLen<<"  "<<arr.toHex();
    }
}



void TcpWorker::startParsing()
{
    isStartParsing = true;
}

void TcpWorker::stopParsing()
{

}

void TcpWorker::slot_destory()
{
    qDebug()<<"slot_destory1";


    writeDebugfile(__FILE__,__FUNCTION__,__LINE__,"销毁tcp服务器");

    isStartParsing = false;
}

void TcpWorker::setLogIdetity(QString str)
{
    mlogId = str;
}

void TcpWorker::writeDebugfile(QString filename,QString funname,int lineCount,QString strContent){

    DebugLogFile::instance()->writeLog(mlogId,filename,QString::number(lineCount),strContent);

}

TcpWorker::~TcpWorker()
{
    qDebug()<<m_did +  " 析构   tcpWorker";

    if(timerConnectSer != nullptr)
    {
        disconnect(timerConnectSer,&QTimer::timeout,this,&TcpWorker::slot_timerConnectSer);
        timerConnectSer->stop();
        timerConnectSer->deleteLater();
    }

    if(tcpSocket != nullptr)
    {
        disconnect(tcpSocket,&QTcpSocket::readyRead,this,&TcpWorker::slot_readData);
        disconnect(tcpSocket,&QTcpSocket::disconnected,this,&TcpWorker::slot_tcpDisconnected);
        disconnect(tcpSocket,&QTcpSocket::connected,this,&TcpWorker::slot_tcpConnected);
        tcpSocket->abort();
        tcpSocket->close();
        tcpSocket == nullptr;
    }

    qDebug()<<m_did +  " 析构   tcpWorker 结束";
}



