/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QSpacerItem>

#include <QApplication>
#include <QDesktopWidget>

#include <QFile>
#include <QTextStream>

#include <QtSerialPort/QSerialPortInfo>

QT_USE_NAMESPACE

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , transactionCount(0)
    , serialPortLabel(new QLabel(tr("Serial port:")))
    , serialPortComboBox(new QComboBox())
    , waitRequestLabel(new QLabel(tr("Wait request, msec:")))
    , waitRequestSpinBox(new QSpinBox())
    , responseLabel(new QLabel(tr("Response:")))
    , responseLineEdit(new QLineEdit(tr("Hello, I'm Slave.")))
    , trafficLabel(new QLabel(tr("No traffic.")))
    , statusLabel(new QLabel(tr("Status: Not running.")))
    , runButton(new QPushButton(tr("Start")))
    ,baudRateComboBox(new QComboBox())
    ,connectionDeviceButton(new QPushButton(tr("Connection Device")))
    ,disConnectionDeviceButton(new QPushButton(tr("DisConnection Device")))
    ,autoRadioButton(new QRadioButton(tr("auto")))
    ,manualRadioButton(new QRadioButton(tr("manual")))

    ,periodLabel(new QLabel(tr("period/minute")))
    ,periodComboBox(new QComboBox())

    ,sendDataLabel(new QLabel("01h"))
    ,sendDatabutton(new QPushButton(tr("send Data")))
{
    waitRequestSpinBox->setRange(0, 10000);
    waitRequestSpinBox->setValue(10000);

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        serialPortComboBox->addItem(info.portName());
    serialPortComboBox->setCurrentIndex(2);

    initBaudRateCombobox();
    initPeriodComboBox();
    initLayout();
    initSigAndSlot();
    initViewModel();

    setWindowTitle(tr("ShangHaiKaiRen"));
    serialPortComboBox->setFocus();
    manualRadioButton->setChecked(true);
}

void Dialog::slotRunButtonClicked()
{
    if(autoRadioButton->isChecked())
    {
        //qDebug() << "auto: ---------- " <<  periodComboBox->currentText().toInt() * 60 * 1000;
        m_timer.start(periodComboBox->currentText().toInt() * 60 * 1000);
        runButton->setEnabled(false);
    }else if(manualRadioButton->isChecked())
    {
        slotSendData();
    }
}

void Dialog::showRequest(const QString &s)
{
    trafficLabel->setText(tr("Traffic, transaction #%1:"
                             "\n\r-request: %2"
                             "\n\r-response: %3")
                          .arg(++transactionCount).arg(s).arg(responseLineEdit->text()));

    uint rowCount= m_model.rowCount();
    qDebug() << rowCount;
    QStringList list;
    getStringListByRawData(s, list);
    QList<QStandardItem*> currentDataRowList;
//    if(list.size() != 6)
//        return;
    for(int column = 0; column < 6; column++)
//        for(int column = 0, column < 5, colo)
    {
        QStandardItem *item = new QStandardItem(list[column]);
        item->setTextAlignment(Qt::AlignCenter);
        m_model.setItem(rowCount, column, item);
    }
    saveDataToFile(list);
    m_view.setModel(&m_model);
    resizeView();
    //m_model.insertRow(1, currentDataRowList);

    //qDebug() << currentDataRowList;

}

void Dialog::processError(const QString &s)
{
    slotActivateRunButtonAndStopTimer();
    statusLabel->setText(tr("Status: Not running, %1.").arg(s));
    trafficLabel->setText(tr("No traffic."));
}

void Dialog::processTimeout(const QString &s)
{
    slotActivateRunButtonAndStopTimer();
    statusLabel->setText(tr("Status: Running, %1.").arg(s));
    trafficLabel->setText(tr("No traffic."));
}

void Dialog::slotActivateRunButtonAndStopTimer()
{
    runButton->setEnabled(true);
    m_timer.stop();
}

void Dialog::slotSendData()
{
    statusLabel->setText(tr("Status: Running, connected to port %1.")
                         .arg(serialPortComboBox->currentText()));
    thread.startSlave(serialPortComboBox->currentText(),
                      (QSerialPort::BaudRate)baudRateComboBox->currentData().toUInt(),
                      responseLineEdit->text());
}

void Dialog::initBaudRateCombobox()
{
    QList<QSerialPort::BaudRate> baudRateList;
    baudRateList.append(QSerialPort::Baud1200);
    baudRateList.append(QSerialPort::Baud2400);
    baudRateList.append(QSerialPort::Baud4800);
    baudRateList.append(QSerialPort::Baud9600);
    baudRateList.append(QSerialPort::Baud115200);
    foreach (QSerialPort::BaudRate baudRate, baudRateList) {
        baudRateComboBox->addItem(QString::number(baudRate));
    }
    baudRateComboBox->setCurrentIndex(3);

}

void Dialog::initPeriodComboBox()
{
    QList<int> periodList;
    periodList.append(1);
    periodList.append(2);
    periodList.append(3);
    foreach (int period, periodList) {
        periodComboBox->addItem(QString::number(period));
    }
    periodComboBox->setCurrentIndex(0);

}

void Dialog::initSigAndSlot()
{
    connect(runButton, SIGNAL(clicked()),
            this, SLOT(slotRunButtonClicked()));
    connect(&thread, SIGNAL(request(QString)),
            this, SLOT(showRequest(QString)));
    connect(&thread, SIGNAL(error(QString)),
            this, SLOT(processError(QString)));
    connect(&thread, SIGNAL(timeout(QString)),
            this, SLOT(processTimeout(QString)));

    connect(serialPortComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(slotActivateRunButtonAndStopTimer()));
    connect(waitRequestSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(slotActivateRunButtonAndStopTimer()));
    connect(responseLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(slotActivateRunButtonAndStopTimer()));

    connect(periodComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotActivateRunButtonAndStopTimer()));
    connect(baudRateComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotActivateRunButtonAndStopTimer()));
    connect(manualRadioButton,SIGNAL(toggled(bool)),  this, SLOT(slotActivateRunButtonAndStopTimer()));
    connect(autoRadioButton,SIGNAL(toggled(bool)),  this, SLOT(slotActivateRunButtonAndStopTimer()));

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotSendData()));
}

void Dialog::initLayout()
{
    QGridLayout *mainLayout = new QGridLayout;

    mainLayout->addWidget(serialPortLabel, 0, 0, Qt::AlignLeft);
    mainLayout->addWidget(serialPortComboBox, 0, 1, Qt::AlignLeft);
    mainLayout->addWidget(baudRateComboBox, 0, 2, Qt::AlignLeft);
    mainLayout->addWidget(connectionDeviceButton, 0, 3, Qt::AlignLeft);
    mainLayout->addWidget(disConnectionDeviceButton, 0, 4, Qt::AlignLeft);

    mainLayout->addWidget(autoRadioButton, 1, 0, Qt::AlignLeft);
    mainLayout->addWidget(manualRadioButton, 1, 1, Qt::AlignLeft);
    mainLayout->addWidget(periodLabel, 1, 2, Qt::AlignLeft);
    mainLayout->addWidget(periodComboBox, 1, 3, Qt::AlignLeft);
    mainLayout->addWidget(runButton, 1, 4, Qt::AlignLeft);
    mainLayout->addItem(new QSpacerItem(0, 0), 1, 5, Qt::AlignLeft);
//    mainLayout->addWidget(waitRequestLabel, 1, 0);
//    mainLayout->addWidget(waitRequestSpinBox, 1, 1);
//    mainLayout->addWidget(runButton, 0, 2, 2, 1);
//    mainLayout->addWidget(responseLabel, 2, 0);
//    mainLayout->addWidget(responseLineEdit, 2, 1, 1, 3);

    mainLayout->addWidget(&m_view, 2, 0);

    mainLayout->addWidget(trafficLabel, 3, 0, 1, 4);
    mainLayout->addWidget(statusLabel, 4, 0, 1, 5);
    setLayout(mainLayout);
}

void Dialog::initViewModel()
{
    m_view.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum); //设置充满表宽度
    m_view.setModel(&m_model);
    m_model.setHorizontalHeaderItem(0, new QStandardItem(tr("DateTime")));
    m_model.setHorizontalHeaderItem(1, new QStandardItem(tr("Work Curve")));
    m_model.setHorizontalHeaderItem(2, new QStandardItem(tr("Measurement time")));
    m_model.setHorizontalHeaderItem(3, new QStandardItem(tr("Repeat times")));
    m_model.setHorizontalHeaderItem(4, new QStandardItem(tr("result Content")));
    m_model.setHorizontalHeaderItem(5, new QStandardItem(tr("Standard Deviation")));

    resizeView();
}

void Dialog::resizeView()
{
    int columnWidth = QApplication::desktop()->width() / 10;
    for(int i = 0; i < 6; i++)
    {
        m_view.horizontalHeader()->resizeSection(i, columnWidth);
    }
//    m_view.horizontalHeader()->resizeSection(0, 200);
//    m_view.horizontalHeader()->resizeSection(1, 200);
//    m_view.horizontalHeader()->resizeSection(2, 200);
//    m_view.horizontalHeader()->resizeSection(3, 200);
//    m_view.horizontalHeader()->resizeSection(4, 200);
//    m_view.horizontalHeader()->resizeSection(5, 200);
    m_view.horizontalHeader()->setStretchLastSection(true);
    m_view.setFixedWidth((columnWidth * 6 + 50));
}

void Dialog::saveDataToFile(QStringList list)
{
    QFile file("./" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".txt");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text |QIODevice::Append))
    {
        file.close();
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text |QIODevice::Append))
        {
            qDebug() << "open err";
            return;
        }
    }

    QTextStream in(&file);

    foreach (QString str, list) {
        in << str << "   ";
    }
    in << "\n";
}

void Dialog::getStringListByRawData(QString rawData, QStringList &list)
{
    /*
        仪器发送数据使用ASC II码表示，共31字节，0dh为结束符,数据格式为：
        例  2010年08月06日 11：30测量硫含量,
        选定工作曲线号1,测量时间060秒,重复次数02次
        结果为含量为0.1250，标准偏差为0.0001
        通讯收到字节依次为:
        32 30 31 30   30 38    30 36    31 31   33 30 (dateTime)
        31 30   36 30(second)    30 32(count)    30 2E 31 32 35 30(value)    30 2E 30 30 30 31      0D

    */

//    if(rawData.size() != 31 || rawData.right(2).toInt() != 0x0D)
//    {
//        list.clear();
//        return;
//    }

    QString dateTime = revertStr(rawData.left(8))  + "/" + revertStr(rawData.mid(8, 4)) + "/" + revertStr(rawData.mid(12, 4)) //Date
            + " " + revertStr(rawData.mid(16, 4)) + ":" + revertStr(rawData.mid(20, 4)); //Time
    QString workCurve = revertStr(rawData.mid(24, 2));
    QString measurementTime = revertStr(rawData.mid(26, 6));
    QString repeatTimes = revertStr(rawData.mid(32, 4));
    QString resultData = revertStr(rawData.mid(36, 12));
    QString standardDeviation = revertStr(rawData.mid(48, 12));
//    list[0] = dateTime;
//    lsit[1] = workCurve;
    list << dateTime <<workCurve << measurementTime << repeatTimes << resultData << standardDeviation;
    qDebug() <<list;
}

QString Dialog::revertStr(QString rawData)
{
    if(rawData.size()%2 != 0)
        return  NULL;
    char tmpData[100];
    memset(tmpData, 0, 100);
    QString returnString;
    for(int i = 0; i < rawData.size()/2; i++)
    {
        //qDebug() << rawData.mid(i * 2, 2).right(1);
        if(!rawData.mid(i * 2, 2).compare("2e", Qt::CaseInsensitive))
        {
            tmpData[i] = '.';
            continue;
        }
        tmpData[i] = rawData.mid(i * 2, 2).right(1).toLocal8Bit().data()[0];
    }
    return QString(tmpData);
}
