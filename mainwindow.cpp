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
#include <sstream>


#ifdef _WIN32
#include<Windows.h>
#else
#include <unistd.h>
#endif

Ui::MainWindow *uiPtr=NULL;
QString commandcom="";
QString learningcom="";
QString imagecom="";
QString homecom="";

using namespace std;

#include <stdio.h>
#include <stdarg.h>

void GetMatches ( const char * str, const char * format, ... )
{
  va_list args;
  va_start (args, format);
  vsscanf (str, format, args);
  va_end (args);
}


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
  tmp+="91.0000% - \"frog\"\n";
  tmp+="9.0000% - \"ship\"\n";
  tmp+="0.0000% - \"automobile\"\n";
  tmp+="0.0000% - \"bird\"\n";
  tmp+="0.0000% - \"cat\"\n";
  tmp+="\n";
  tmp+="Script took 0.272104024887 seconds.\n";
  return tmp;
}
string MakeSureDirectoryPathExists(string path)
{
    #ifdef _WIN32
    typedef BOOL (WINAPI * CreateDirFun ) (PCSTR DirPath );
    HMODULE h = LoadLibraryA((PCSTR) "dbghelp.dll" );
    CreateDirFun pFun = (CreateDirFun) GetProcAddress( h, "MakeSureDirectoryPathExists" );
    (*pFun)((PCSTR) (path+"\\").c_str() );
    FreeLibrary( h );
    #endif
    return path;
}
string ExecuteCommand(string command)
{
    string response="";
    #ifdef _WIN32
    response=DigitResponseMock();//Digits not support windows yet
    return response;
    #else
   qDebug() <<homecom.toStdString().c_str()<<"***";
    chdir(homecom.toStdString().c_str());
    FILE* file = popen(command.c_str(), "r");
    response=GetData(file);
    pclose(file);
    return response;
    #endif

}


void messagebox(string title, string message)
{
    #ifdef _WIN32
    QMessageBox msgBox;
    msgBox.setText(title.c_str());
    msgBox.setInformativeText(message.c_str());
    msgBox.exec();
    #else
    char command[4000]="";
    sprintf(command,"notify-send '%s' '%s'",title.c_str(), message.c_str());
    FILE* file= popen(command, "r");
    pclose(file);
    #endif
}


bool checkCameraAvailability()
{
    if (QCameraInfo::availableCameras().count() > 0)
        return true;
    else
        return false;
}

QCameraImageCapture *imageCapture=NULL;
QCamera *camera=NULL;

struct data
{
   float value;
   char category[100];
};

std::list<data> parse(string rsp)
{
    std::stringstream ss(rsp);
    std::list<data> pairs;
    string line="";

    if (rsp.c_str() != NULL)
    {
        while(std::getline(ss,line,'\n'))
        {

           char cat[100]="";
           char *ptr=strstr((char*)line.c_str()," - ");
           if(ptr!=NULL)
           {
                  memset(cat,0,100);
                  #ifdef _WIN32
                  strcpy_s(cat,ptr+3);
                  #else
                  strcpy(cat,ptr+3);
                  #endif
                  data obj;
                  obj.value=atof(line.c_str());
                  #ifdef _WIN32
                  strcpy_s(obj.category,ptr+3);
                  #else
                  strcpy(obj.category,ptr+3);
                  #endif
                  pairs.push_back(obj);

           }

        }
    }
    return pairs;
}


float cal_sum(std::list<data> &parsedata)
{
    float average=0.0;
    std::list<data>::const_iterator it;
    for (it = parsedata.begin(); it != parsedata.end(); ++it)
    {
        if (it !=parsedata.begin())
        {

            average+=(*it).value;

        }
    }
    return average;
}

const char *getCategory(std::list<data> &parsedata)
{
   std::list<data>::const_iterator it=parsedata.begin();
   return (*it).category;
}

float getValue(std::list<data> &parsedata)
{
   std::list<data>::const_iterator it=parsedata.begin();
   return (*it).value;
}


void DrawText(QImage &qImageScaled,QString &text)
{
    const QColor textColor=0xff0000;
    QPainter painter(&qImageScaled);
    int fontSize=20;
    //painter.fillRect(0, 0, text_width, text_height, backgroundColor);
    painter.setBrush(textColor);
    painter.setPen(textColor);
    painter.setFont(QFont("Sans", fontSize));
    painter.drawText(5, 20, text);
}



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
        currentFrame = QImage(newFrame.bits(),frame.width(),frame.height(),newFrame.bytesPerLine(),m_imageFormat);

        //qDebug()<<currentFrame.size()<<" "<<newFrame.bits()<<" "<<newFrame.width()<<" "<<newFrame.height();
        //              " "<<newFrame.bytesPerLine()<<" "<<m_imageFormat;
        //QSize(1600, 1200)   0x7f44b0763010   1600   1200   6400   4

        #ifdef _WIN32
        currentFrame=currentFrame.mirrored();
        currentFrame.save("c:/temp/picture.jpg");
        #else
        currentFrame.save("/tmp/picture.jpg");// /dev/shm/picture.jpg
        #endif



        //qDebug()<<command<<"\n";
        string rsp="";
        QString command=commandcom+" "+learningcom+" "+imagecom;
        qDebug()<<command.toStdString().c_str()<<"\n";
        rsp=ExecuteCommand(command.toStdString());
        qDebug()<<rsp.c_str()<<"\n";
        std::list<data> parsedata=parse(rsp);

        //float sum=cal_sum(parsedata);
        QString text="";


        //qDebug() <<getValue(parsedata)<<"***************************************"<<sum<<"\n";
        //if (sum < 10 && getValue(parsedata) > 99)
        if (getValue(parsedata) > 99.0)
        {
            text=getCategory(parsedata);

        }
        else
        {
            text="Not categorised";
        }

        QImage qImageScaled = currentFrame.scaled(QSize(uiPtr->label->width(),uiPtr->label->height()),Qt::KeepAspectRatio,Qt::FastTransformation);

        DrawText(qImageScaled,text);

        uiPtr->label->setPixmap(QPixmap::fromImage(qImageScaled));
        //uiPtr->label->resize(uiPtr->label->pixmap()->size());

        newFrame.unmap();
        qDebug() << "Draw time:" << time.elapsed()<<"ms";
        return true;
    }
}mySurface;

void MainWindow::handleButton()
{
   cout<<"*******************************************";
    commandcom = uiPtr->plainTextEdit->toPlainText();
    learningcom = uiPtr->plainTextEdit2->toPlainText();
    imagecom = uiPtr->plainTextEdit3->toPlainText();
    homecom = uiPtr->plainTextEdit4->toPlainText();
}
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
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
        MainWindow::handleButton();
    }


    connect(ui->pushButton, SIGNAL (clicked()), this, SLOT (handleButton()));

}

MainWindow::~MainWindow()
{
    delete ui;
}
