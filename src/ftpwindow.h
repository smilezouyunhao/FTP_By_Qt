#ifndef FTPWINDOW_H
#define FTPWINDOW_H

#include <QDebug>

#include <QDialog>
#include <QHash>
#include <QMap>
#include <QStack>
#include <QNetworkConfigurationManager>

#include "qftp/qftp.h"

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QFile;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressDialog;
class QPushButton;
class QUrlInfo;
class QNetworkSession;
QT_END_NAMESPACE

class FTPWindow : public QDialog
{
  Q_OBJECT

public:
  FTPWindow(QWidget *parent = 0);
  QSize sizeHint() const;
  void downAllFile(QString rootDir); //下载的真正操作函数

private slots:
  // 连接函数
  void connectOrDisconnect();
  void connectToFtp();

  // 下载函数
  void downloadFile();
  void cancelDownload();
  void showProgressDialog();
  void clearDownFilesWhenCancelDownDir();
  void updateDataTransferProgress(qint64 readBytes, qint64 totalBytes);

  // 本地Widget
  void showLocalMain();
  void addToLocalList(QString path);

  // 上传函数
  void uploadFile();

  // 遍历目录函数
  void addToList(const QVector<QUrlInfo> &urlInfos);
  void cdToParent();
  void localCdToParent();
  void processItem(QTreeWidgetItem *item, int column);
  void localProcessItem(QTreeWidgetItem *item, int column);

  void enableDownloadButton();
  void enableConnectButton();

  void ftpCommandFinished(int id, bool error);

private:
  // 界面
  QLabel *ftpServerLabel;
  QLineEdit *ftpServerLineEdit;
  QLabel *statusLabel;
  QLabel *localPathLabel;
  QTreeWidget *fileList;
  QTreeWidget *localList;
  QPushButton *cdToParentButton;
  QPushButton *connectButton;
  QPushButton *downloadButton;
  QPushButton *quitButton;
  QPushButton *localMainButton;
  QPushButton *uploadButton;
  QPushButton *localCdToParentButton;
  QDialogButtonBox *buttonBox;
  QProgressDialog *progressDialog;
  QWidget *localMain;

  // ftp目录和连接
  QHash<QString, bool> isDirectory;
  QString currentPath;
  QFtp *ftp;
  QMap<int, QFile *> files;

  // 本地系统
  QHash<QString, bool> localDirectory;
  QString localPath;

  // 网络会话
  QNetworkSession *networkSession;
  QNetworkConfigurationManager manager;

  // 下载
  long long downloadBytes;
  long long downloadTotalBytes;
  int downloadTotalFiles;
  QString downloadPath;
  bool downFinished;
  bool enterSubDir;
  QString currentDownPath;
  QStack<QString> downDirs; //下载处理堆栈

  // 上传
  long long uploadBytes;
  long long uploadTotalBytes;
};

#endif  //FTPWINDOW_H
