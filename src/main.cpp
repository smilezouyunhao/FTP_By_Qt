#include <QApplication>
#include "ftpwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  FTPWindow ftpWin;
  ftpWin.show();
  
  return ftpWin.exec();
}
