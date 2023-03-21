#include "ftpwindow.h"

#include <QtWidgets>
#include <QtNetwork>

FTPWindow::FTPWindow(QWidget *parent)
  : QDialog(parent), ftp(nullptr), networkSession(0), downloadBytes(0),
  downloadTotalBytes(0), downloadTotalFiles(0), downloadPath(QCoreApplication::applicationDirPath() + "/DownDir"),
  downFinished(true), enterSubDir(false), currentDownPath(""), localPath(QCoreApplication::applicationDirPath())
{
  // ftp服务器输入框
  ftpServerLabel = new QLabel(tr("FTP &server:"));
  ftpServerLineEdit = new QLineEdit("ftp://parallels:990117@10.211.55.3:21");
  ftpServerLabel->setBuddy(ftpServerLineEdit);

  // 状态提示框
  statusLabel = new QLabel(tr("请输入FTP server的信息"));

  // 本地路径提示框
  localPathLabel = new QLabel(tr("LocalPath: %1").arg(QCoreApplication::applicationDirPath()));

  // 本地Widget窗口唤出按钮
  localMainButton = new QPushButton(tr("Main"));

  // 本地列表Widget
  localMain = new QWidget;
  
  // 目录列表
  fileList = new QTreeWidget;
  fileList->setEnabled(false);
  fileList->setRootIsDecorated(false);
  fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  fileList->setHeaderLabels(QStringList() << tr("文件名") << tr("大小") << tr("时间"));
  fileList->setColumnWidth(0, 400);
  fileList->setColumnWidth(1, 200);
  fileList->setColumnWidth(2, 200);
  fileList->header()->setStretchLastSection(false);

  // 本地目录列表
  localList = new QTreeWidget(localMain);
  localList->resize(QSize(800, 500));
  localList->setRootIsDecorated(false);
  localList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  localList->setHeaderLabels(QStringList() << tr("文件名") << tr("大小") << tr("时间"));
  localList->setColumnWidth(0, 400);
  localList->setColumnWidth(1, 200);
  localList->setColumnWidth(2, 200);
  localList->header()->setStretchLastSection(false);

  // 连接按钮
  connectButton = new QPushButton(tr("Connect"));
  connectButton->setDefault(true);

  // 返回上层目录按钮
  cdToParentButton = new QPushButton;
  cdToParentButton->setIcon(QPixmap(":/images/cdtoparent.png"));
  cdToParentButton->setEnabled(false);

  // 下载按钮
  downloadButton = new QPushButton(tr("Download"));
  downloadButton->setEnabled(false);

  // 退出按钮
  quitButton = new QPushButton(tr("Quit"));

  // buttonBox
  buttonBox = new QDialogButtonBox;
  buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);
  buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

  // 下载对话框(显示下载进度)
  progressDialog = new QProgressDialog(this);
  progressDialog->reset();
  progressDialog->setAutoClose(false);
  progressDialog->setAutoReset(false);

  // connect
  connect(fileList, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(processItem(QTreeWidgetItem *, int)));
  connect(fileList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(enableDownloadButton()));

  connect(localMainButton, SIGNAL(clicked()), this, SLOT(showLocalMain()));
  connect(localList, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(localProcessItem(QTreeWidgetItem *, int)));

  connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));
  connect(connectButton, SIGNAL(clicked()), this, SLOT(connectOrDisconnect()));
  connect(cdToParentButton, SIGNAL(clicked()), this, SLOT(cdToParent()));
  connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
  connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

  // 设置布局
  QHBoxLayout *topLayout = new QHBoxLayout;
  topLayout->addWidget(ftpServerLabel);
  topLayout->addWidget(ftpServerLineEdit);
  topLayout->addWidget(cdToParentButton);
  topLayout->addWidget(localMainButton);
  topLayout->addWidget(connectButton);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(fileList);
  mainLayout->addWidget(statusLabel);
  mainLayout->addWidget(localPathLabel);
  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  setWindowTitle(tr("FTP Client"));
}

// 界面宽高
QSize FTPWindow::sizeHint() const
{
  return QSize(800, 500);
}

// 下载文件夹中的所有文件
void FTPWindow::downAllFile(QString rootDir)
{
  QString thisRoot(rootDir + "/");

  QList<QTreeWidgetItem *> selectedItemList = fileList->selectedItems();
  for (int i = 0; i < selectedItemList.size(); i++)
  {
    QString fileName = selectedItemList[i]->text(0);
    if (isDirectory.value(fileName))
    {
      if(fileName != "..")
      downDirs.push(thisRoot + fileName);
    }
    else {
      // 统计下载的字节量
      downloadTotalBytes += selectedItemList[i]->text(1).toLongLong();
      QString dirTmp(downloadPath);
      dirTmp.append(rootDir);
      QDir downDir(dirTmp);

      if (!downDir.exists(dirTmp))
      {
        downDir.mkpath(dirTmp);
      }

      QFile *file = new QFile(dirTmp.append("/").append(fileName));
      if (!file->open(QIODevice::WriteOnly))
      {
        QMessageBox::information(this, tr("FTP"), tr("无法保存文件 %1: %2.")
                                                  .arg(fileName).arg(file->errorString()));

        delete file;
        return;
      }

      // 文件下载请求，异步操作
      int id = ftp->get(QString::fromLatin1((selectedItemList[i]->text(0)).toStdString().c_str()), file);
      // 本地IO设备和命令绑定并存储
      files.insert(id, file);
    }
  }

  // 待下载目录堆栈不空，就处理
  if (downDirs.size() > 0) {
    enterSubDir = true; //代表正在下载目录

    QString nextDir(downDirs.pop()); //取需要处理的下个目录
    ftp->cd(nextDir); //切换到下个目录

    currentDownPath = nextDir;
    ftp->list();
  }
  else
  {
    enterSubDir = false;
    ftp->cd(currentPath == "" ? "/" : currentPath);
    ftp->list();

    downloadButton->setEnabled(false);
    downloadTotalFiles = files.size();
  }
}

// 连接和断开连接
void FTPWindow::connectOrDisconnect()
{
  // 检测ftp是否指向连接
  if (ftp)
  {
    ftp->abort();
    ftp->deleteLater();
    ftp = nullptr;

    fileList->setEnabled(false);
    cdToParentButton->setEnabled(false);
    downloadButton->setEnabled(false);
    connectButton->setEnabled(true);
    connectButton->setText(tr("Connect"));

#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif

    statusLabel->setText(tr("请输入FTP server的信息"));
    return;
  }

#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif

  if (!networkSession || !networkSession->isOpen())
  {
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
    {
      if (!networkSession)
      {
        // 获取network设置
        QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // 如果保存的设置不是当前的，则使用系统默认
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) != QNetworkConfiguration::Discovered)
        {
          config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, SIGNAL(opened()), this, SLOT(connectToFtp()));
        connect(networkSession, SIGNAL(error(QNetworkSession::SessionError)), this, SLOT(enableConnectButton()));
      }

      connectButton->setEnabled(false);
      statusLabel->setText(tr("正在开启network会话"));
      networkSession->open();
      return;
    }
  }

  connectToFtp();
}

// 连接ftp服务器
void FTPWindow::connectToFtp()
{
  ftp = new QFtp(this);

  connect(ftp, SIGNAL(commandFinished(int, bool)), this, SLOT(ftpCommandFinished(int, bool)));
  connect(ftp, SIGNAL(listInfos(QVector<QUrlInfo>)), this, SLOT(addToList(QVector<QUrlInfo>)));
  connect(ftp, SIGNAL(dataTransferProgress(qint64, qint64)), this, SLOT(updateDataTransferProgress(qint64, qint64)));

  fileList->clear();
  currentPath.clear();
  isDirectory.clear();

  QUrl url(ftpServerLineEdit->text());
  // 检查填写的信息是否有效
  if (!url.isValid() || url.scheme().toLower() != QLatin1String("ftp"))
  {
    ftp->connectToHost(ftpServerLineEdit->text(), 21);
    ftp->login();
  }else
  {
    ftp->connectToHost(url.host(), url.port(21));

    if (!url.userName().isEmpty())
      ftp->login(QUrl::fromPercentEncoding(url.userName().toLatin1()), url.password());
    else
      ftp->login();

    if (!url.path().isEmpty())
      ftp->cd(url.path());
  }

  fileList->setEnabled(true);
  connectButton->setEnabled(false);
  connectButton->setText(tr("Disconnect"));
  statusLabel->setText(tr("连接到FTP服务器: %1...").arg(ftpServerLineEdit->text()));
}

// 下载文件
void FTPWindow::downloadFile()
{
  files.clear();
  downDirs.clear();
  downloadBytes = 0;
  downloadTotalBytes = 0;
  enterSubDir = false;
  downFinished = false;

  downAllFile(currentPath);
  showProgressDialog();
}

// 取消下载
void FTPWindow::cancelDownload()
{
  ftp->abort();

  downloadBytes = 0;
  downloadTotalBytes = 0;

  if (enterSubDir)
  {
    QString curTmp = currentPath;
    enterSubDir = false;

    downDirs.clear();
    connectButton->click();
    connectButton->click();

    ftp->cd(curTmp == "" ? "/" : curTmp);

    QTimer::singleShot(100, this, SLOT(clearDownFilesWhenCancelDownDir()));
  }

  enterSubDir = false;
}

// 展示下载进度
void FTPWindow::showProgressDialog()
{
  if (!downFinished)
  {
    progressDialog->setLabelText(tr("正在下载所选文件..."));
    progressDialog->exec();
  }
}

// 取消下载时清除未完成文件
void FTPWindow::clearDownFilesWhenCancelDownDir()
{
  QMapIterator<int, QFile *> iterator(files);
  
  while (iterator.hasNext())
  {
    iterator.next();
    QFile *file = iterator.value();

    file->close();
    file->remove();
    delete file;
  }

  files.clear();
}

// 更新下载进度
void FTPWindow::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes)
{
  downloadBytes += readBytes;

  if (!progressDialog->isHidden())
  {
    progressDialog->setMaximum(downloadTotalBytes);
    progressDialog->setValue(downloadBytes);
  }
}

// 唤出本地Widget
void FTPWindow::showLocalMain()
{
  localMain->setWindowTitle(tr("本地窗口"));
  addToLocalList(localPath);
  localMain->show();
  localList->show();
}

// 显示本地目录列表
void FTPWindow::addToLocalList(QString path)
{
  localList->clear();
  int dirIndex = 0;

  QDir dir(path);
  if (!dir.exists())
    return;

  dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDot);
  QFileInfoList list = dir.entryInfoList();

  int file_count = list.count();
  if (file_count <= 0)
    return;

  for (int i = 0; i < file_count; i++)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem;
    QFileInfo file_info = list.at(i);

    item->setText(0, file_info.fileName().toLatin1());
    item->setText(1, QString::number(file_info.size()));
    item->setText(2, file_info.lastModified().toString("MMM dd yyyy"));
    
    QPixmap pixmap_dir(":/images/dir.png");
    QPixmap pixmap_file(":/images/file.png");

    if (file_info.isDir())
      item->setIcon(0, pixmap_dir);
    else
      item->setIcon(0, pixmap_file);

    localDirectory[file_info.fileName()] = file_info.isDir();
    if (file_info.isDir())
      localList->insertTopLevelItem(dirIndex++, item);
    else
      localList->addTopLevelItem(item);
  }
}

// 将目录添加到列表
void FTPWindow::addToList(const QVector<QUrlInfo> &urlInfos)
{
  fileList->clear();
  int dirIndex = 0;

  for (int i = 0; i < urlInfos.size(); i++)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem;
    QUrlInfo urlInfo = urlInfos[i];

    if (urlInfo.name().compare(".") != 0)
    {
      item->setText(0, urlInfo.name().toLatin1());
      item->setText(1, QString::number(urlInfo.size()));
      /* item->setText(2, QString::number(urlInfo.isDir()));
      item->setText(3, urlInfo.owner());
      item->setText(4, urlInfo.group()); */
      item->setText(2, urlInfo.lastModified().toString("MMM dd yyyy"));

      QPixmap pixmap(urlInfo.isDir() ? ":/images/dir.png" : ":/images/file.png");
      item->setIcon(0, pixmap);

      isDirectory[urlInfo.name()] = urlInfo.isDir();
      if (urlInfo.isDir())
        fileList->insertTopLevelItem(dirIndex++, item);
      else
        fileList->addTopLevelItem(item);
    }
  }

  // 检查目标内是否有文件夹
  if (!enterSubDir)
  {
    if (!fileList->currentItem())
    {
      fileList->setCurrentItem(fileList->topLevelItem(0));
      fileList->setEnabled(true);
    }
  }else
  {
    fileList->selectAll(); //选中列表中的所有项目
    downAllFile(currentDownPath); //递归调用下载处理函数
  }
}

// 返回上一层目录
void FTPWindow::cdToParent()
{
#ifndef QT_NO_CURSOR
  setCursor(Qt::WaitCursor);
#endif

  fileList->clear();
  isDirectory.clear();
  currentPath = currentPath.left(currentPath.lastIndexOf('/'));

  if (currentPath.isEmpty())
  {
    cdToParentButton->setEnabled(false);
    ftp->cd("/");
  }else
  {
    ftp->cd(currentPath);
  }

  ftp->list();
}

// 处理TreeWidgetItem
void FTPWindow::processItem(QTreeWidgetItem *item, int column)
{
  QString name = item->text(0);

  // 如果所选的是文件夹，则打开
  if (isDirectory.value(name))
  {
    fileList->clear();
    isDirectory.clear();

    currentPath += '/';
    currentPath += name;

    ftp->cd(name);
    ftp->list();

    cdToParentButton->setEnabled(true);

#ifndef QT_NO_CURSOR
    setCursor(Qt::WaitCursor);
#endif

    return;
  }
}

// 本地目录处理函数
void FTPWindow::localProcessItem(QTreeWidgetItem *item, int column)
{
  QString name = item->text(0);

  if (localDirectory.value(name))
  {
    localList->clear();
    localDirectory.clear();

    localPath += '/';
    localPath += name;

    addToLocalList(localPath);

#ifndef QT_NO_CURSOR
    setCursor(Qt::WaitCursor);
#endif

    return;
  }

}

// 将下载按钮启用
void FTPWindow::enableDownloadButton()
{
  downloadButton->setEnabled(true);
}

// 将连接按钮启用
void FTPWindow::enableConnectButton()
{
  QNetworkConfiguration config = networkSession->configuration();
  QString id;

  if (config.type() == QNetworkConfiguration::UserChoice)
    id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
  else
    id = config.identifier();

  QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
  settings.beginGroup(QLatin1String("QtNetwork"));
  settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
  settings.endGroup();


  connectButton->setEnabled(true);
  statusLabel->setText(tr("请输入FTP server的信息"));
}

// ftp命令执行完毕
void FTPWindow::ftpCommandFinished(int id, bool error)
{
#ifndef QT_NO_CURSOR
  setCursor(Qt::ArrowCursor);
#endif

  if (ftp->currentCommand() == QFtp::ConnectToHost)
  {
    if (error)
    {
      QMessageBox::information(this, tr("FTP"),
                                tr("无法连接到FTP服务器，请检查格式是否正确").arg(ftpServerLineEdit->text()));

      connectOrDisconnect();
      return;
    }

    statusLabel->setText(tr("登录至 %1").arg(ftpServerLineEdit->text()));
    fileList->setFocus();
    downloadButton->setDefault(true);
    connectButton->setEnabled(true);

    return;
  }

  if (ftp->currentCommand() == QFtp::Login)
    ftp->list();

  if (ftp->currentCommand() == QFtp::Get)
  {
    QFile *file = files.take(id);

    if (error)
    {
      statusLabel->setText(tr("%1 取消下载").arg(file->fileName()));
      
      file->close();
      file->remove();

      QString dd = currentPath == "" ? "/" : currentPath;
      bool f = ftp->cd(dd);
      ftp->list();

      qDebug() << "取消成功: " << file->fileName();
    }else
    {
      QStringList fileInfo = file->fileName().split("/");
      statusLabel->setText(tr("下载成功，文件名: %1").arg(fileInfo[fileInfo.size() - 1]));
      file->close();
      qDebug() << "下载成功，文件名: " << file->fileName();
    }

    delete file;
    file = nullptr;

    if (files.size() == 0 && !enterSubDir)
    {
      downFinished = true;
      enableDownloadButton();
      progressDialog->reset();
      progressDialog->hide();
      downloadBytes = 0;
      downloadTotalBytes = 0;
      downloadTotalFiles = 0;
    }else if (ftp->currentCommand() == QFtp::List)
    {
      if (isDirectory.isEmpty())
      {
        fileList->addTopLevelItem(new QTreeWidgetItem(QStringList() << tr("<empty>")));
        fileList->setEnabled(false);
      }
    }
  }
}
