#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMediaService>
#include <QMediaRecorder>
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QPalette>
#include <QtWidgets>
#include <QCamera>
#include <QCameraImageCapture>
#include <QGraphicsVideoItem>
#include <QAbstractVideoSurface>
#include <QCameraViewfinderSettingsControl>
#include <QMessageBox>
#include <QTime>
#include <iostream>
#include <stdio.h>

#ifdef _WIN32
#include<Windows.h>
#else
#include <unistd.h>
#endif

Ui::MainWindow *uiPtr=NULL;

using namespace std;

string GetData(FILE* file)
{
    char line[200]="";;
    std::string result="";
    while (fgets(line, 200, file))
    {
        result+= line;
    }
    return result;

}
string DigitResponseMock()
{
  string tmp="";
  tmp+="Processed 1/1 images ...\n";
  tmp+="Classification took 0.00332999229431 seconds.\n";
  tmp+="------------------------ Prediction for /tmp/picture.jpeg ------------------------\n";
  tmp+="94.1835% - \"frog\"\n";
  tmp+=" 5.8165% - \"ship\"\n";
  tmp+=" 0.0000% - \"automobile\"\n";
  tmp+=" 0.0000% - \"bird\"\n";
  tmp+=" 0.0000% - \"cat\"\n";
  tmp+="\n";
  tmp+="Script took 0.272104024887 seconds.\n";
  return tmp;
}
void MakeSureDirectoryPathExists(string path)
{
    typedef BOOL (WINAPI * CreateDirFun ) (PCSTR DirPath );
    HMODULE h = LoadLibraryA((PCSTR) "dbghelp.dll" );
    CreateDirFun pFun = (CreateDirFun) GetProcAddress( h, "MakeSureDirectoryPathExists" );
    (*pFun)((PCSTR) (path+"\\").c_str() );
    FreeLibrary( h );
}
string ExecuteCommand(string command)
{
    string response="";
    #ifdef linux
        FILE* file = popen(command.c_str(), "r");
        response=GetData(file);
        pclose(file);
        return response;
    #else _WIN32
        response=DigitResponseMock();//Digits not support windows yet
        return response;
    #endif
}

void messagebox(string title, string message)
{
    #ifdef linux
    char command[4000]="";
    sprintf(command,"notify-send '%s' '%s'",title.c_str(), message.c_str());
    FILE* file= popen(command, "r");
    pclose(file);
    #else _WIN32
    QMessageBox msgBox;
    msgBox.setText(title.c_str());
    msgBox.setInformativeText(message.c_str());
    msgBox.exec();
    #endif
}


bool checkCameraAvailability()
{
    if (QCameraInfo::availableCameras().count() > 0)
        return true;
    else
        return false;
}

//QTimer qTimer;
QCameraImageCapture *imageCapture=NULL;
QCamera *camera=NULL;
/*
void CaptureImage(QString path=NULL)
{
    //on half pressed shutter button
    camera->searchAndLock();
    //on shutter button pressed
    imageCapture->capture();
    //on shutter button released
    camera->unlock();
}
*/
class MyVideoSurface : public QAbstractVideoSurface
{
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const
    {
        Q_UNUSED(handleType);

        // Return the formats you will support
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
    }

    QImage::Format m_imageFormat = QImage::Format_RGB32;

    QImage currentFrame;
    QTime time;

    bool present(const QVideoFrame &frame)
    {
        time.start();
        Q_UNUSED(frame);
        // Handle the frame and do your processing
        QVideoFrame newFrame(frame);
        newFrame.map(QAbstractVideoBuffer::ReadWrite);

        //memmove(newFrame, img.mirrored().bits(),img.byteCount());

        currentFrame = QImage(newFrame.bits(),640,480,newFrame.bytesPerLine(),m_imageFormat);
        currentFrame=currentFrame.mirrored();
        //qDebug()<<currentFrame.size()<<" "<<newFrame.bits()<<" "<<newFrame.width()<<" "<<newFrame.height()<<
        //              " "<<newFrame.bytesPerLine()<<" "<<m_imageFormat;
        //QSize(1600, 1200)   0x7f44b0763010   1600   1200   6400   4



        #ifdef linux
        currentFrame.save("/tmp/picture.bmp");// /dev/shm/picture.jpg
        #elif _WIN32
        //memmove(pRgb32Buffer, img.mirrored().bits(),img.byteCount());
        currentFrame.save("c:/temp/picture.bmp");
        #else
        std::cout<<"Unknown OS: Picture not saved";
        #endif
        QString command = uiPtr->lineEdit->text();
        //qDebug()<<command<<"\n";
        string rsp="";
        rsp=ExecuteCommand(command.toStdString().c_str());
        //qDebug()<<rsp.c_str();
            //TO DO 2:
            //Parse prosentvalue1,Category1,prosentvalue2,Category2, ... pairs from the response string and add them to std::list

            //TO DO 3;

            //Get highest (prosentvalue,category) pair from std::list and draw the category name e.g "Frog" to currentFrame

        QImage qImageScaled = currentFrame.scaled(QSize(uiPtr->label->width(),uiPtr->label->height()),Qt::KeepAspectRatio,Qt::FastTransformation);


        const QString text="Not categorised";
        const QColor textColor=0xff0000;
        QPainter painter(&qImageScaled);
        int fontSize=20;
        //painter.fillRect(0, 0, text_width, text_height, backgroundColor);
        painter.setBrush(textColor);
        painter.setPen(textColor);
        painter.setFont(QFont("Sans", fontSize));
        painter.drawText(5, 20, text);
        uiPtr->label->setPixmap(QPixmap::fromImage(qImageScaled));
        //uiPtr->label->resize(uiPtr->label->pixmap()->size());

        newFrame.unmap();
        qDebug() << "Draw time:" << time.elapsed()<<"ms";
        return true;
    }
}mySurface;

/*
    void imageLabel::paintEvent(QPaintEvent *event)
    {
        QLabel::paintEvent(event);
        if (!m_qImage.isNull())
        {
            QImage qImageScaled = m_qImage.scaled(QSize(width(),height()),Qt::KeepAspectRatio,Qt::FastTransformation);
        double dAspectRatio = (double)qImageScaled.width()/(double)m_qImage.width();
        int iX = m_iX*dAspectRatio;
        int iY = m_iY*dAspectRatio;
        int iWidth = m_iWidth*dAspectRatio;
        int iHeight = m_iHeight*dAspectRatio;

        QPainter qPainter(this);
        qPainter.drawImage(0,0,qImageScaled);
        qPainter.setBrush(Qt::NoBrush);
        qPainter.setPen(Qt::red);
        qPainter.drawRect(iX,iY,iWidth,iHeight);
        }
    }
    */












MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    uiPtr=ui;
    #ifdef _WIN32
    MakeSureDirectoryPathExists("c:\\temp");
    #endif
    if(checkCameraAvailability()==true)
    {

        QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        foreach (const QCameraInfo &cameraInfo, cameras)
        {
            //if (cameraInfo.deviceName()!=NULL)
            //{
               std::cout<<"Camera name "<<cameraInfo.deviceName().toStdString()<<"\n";
            //}
        }
        camera = new QCamera();
        imageCapture = new QCameraImageCapture(camera);
        camera->setCaptureMode(QCamera::CaptureStillImage);
        camera->setViewfinder(&mySurface);
        camera->start();
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}
