/*
  11/01/2022 https://github.com/su8/kurger
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.
*/
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

static void RaiseWarning(const QString &msg1, const QString &msg2);
static void pdf2img(const char *str);
static size_t index_last_sep(const char *str);

#define VLA 4999

static std::string PdfFile = "";

Ui::MainWindow *UI;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    UI = ui;

    ui->comboBox->addItems({"png", "jpg", "bmp", "tiff"});
    ui->comboBox_2->addItems({"png16m", "pngalpha", "pnggray", "jpeg", "jpegcmyk", "jpeggray", "bmp16m", "bmpgray", "tiff24nc", "tiffgray"});
}

MainWindow::~MainWindow()
{
    delete UI;
    delete ui;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

void MainWindow::on_pushButton_clicked()
{
    if (PdfFile != "")
    {
        pdf2img(PdfFile.c_str());
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    int spin1 = UI->spinBox_2->value();
    int spin2 = UI->spinBox_3->value();

    if (spin1 > spin2) {
      RaiseWarning("Reversed Numbers", "From page can't be greater than To page.");
      return;
    }

    QString filename = QFileDialog::getOpenFileName(
                nullptr,
                QObject::tr("Open PDF file"),
                QDir::currentPath(),
                QObject::tr("PDF File (*.pdf)"));
    if ((!filename.isEmpty()) && (!filename.isNull()))
    {
        PdfFile = filename.toStdString();
        pdf2img(filename.toStdString().c_str());
    }
}

void RaiseWarning(const QString &msg1, const QString &msg2)
{
    QMessageBox msgWarning;
    msgWarning.setText(msg2);
    msgWarning.setIcon(QMessageBox::Warning);
    msgWarning.setWindowTitle(msg1);
    msgWarning.exec();
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    unsigned short int current = 0;

    if (index == 0) {
      current = 0;
    } else {
      if (index == 1) {
        current = 3;
      } else {
        current = (index == 2) ? 6 : 8;
      }
    }
    UI->comboBox_2->setCurrentIndex(current);
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    unsigned short int next = 0, current = 0;

    if (index == 0)
        next = 0;
    else
      if (index == 1)
        next = 1;
      else
        if (index == 2)
          next = 2;
        else    /* 'active' not in png group */
        {
          current = 1; /* The jpg group */
          if (index == 3)
            next = 3;
          else
            if (index == 4)
              next = 4;
            else
              if (index == 5)
                next = 5;
              else    /* 'active' not in jpg group */
              {
                current = 2; /* The bmp group */
                if (index == 6)
                  next = 6;
                else
                  if (index == 7)
                    next = 7;
                  else    /* 'active' not in bmp group */
                  {
                    current = 3; /* The tiff group */
                    next = (index == 8) ? 8 : 9;
                  }
              }
          }
    UI->comboBox->setCurrentIndex(current);
    UI->comboBox_2->setCurrentIndex(next);
}

/*
   return the last seperator '/' index number,
   we will use this index number to create our own
   `basename' alternative in C
*/
size_t index_last_sep(const char *str) {
  const char *ptr = str;
  size_t sep_index = 0 , x = 0;

  for (; *ptr; x++, ptr++)
    if ('/' == *ptr) {
      sep_index = x; /* keep in mind that we use loop */
    }

  return sep_index;
}

void pdf2img(const char *str)
{
    const char *image_combo = UI->comboBox->currentText().toStdString().c_str();
    const char *sdevice_combo = UI->comboBox_2->currentText().toStdString().c_str();
    char pdfname[VLA+1], BaseName[VLA+1], params[VLA+1];
    char ren1[VLA+1], ren2[VLA+1], created_dir[VLA+1];
    int spin1 = UI->spinBox_2->value();
    int spin2 = UI->spinBox_3->value();
    int small_range = (spin2 - spin1) + 2, big_range = spin1, y = 0;
    size_t dirname_len = index_last_sep(str), x = 0, z = 0;
    size_t fit = strlen(str);
    size_t fit2 = fit - 4; /* exclude the .pdf file extension */
    struct stat DiR;

    if (1850 <= fit) {
      RaiseWarning("Warning!", "The given filename is too long!");
      return;
    }
    if (spin1 > spin2) {
      RaiseWarning("Reversed Numbers", "From page can't be greater than To page.");
      return;
    }

    snprintf(pdfname, VLA, "%s", str);

    for (z = 0, x = dirname_len+1; x < fit2; x++, z++)
      BaseName[z] = pdfname[x]; /* /path/to/some.pdf -> some      */
    BaseName[z] = '\0';

    if (240 < z) {
      RaiseWarning("Warning!", "The given filename is too long!");
      return;
    }

    snprintf(created_dir, VLA, "%s converted", pdfname);
    stat(created_dir, &DiR);
    if (0 == S_ISDIR(DiR.st_mode)) {
      mkdir(created_dir);
    }

    snprintf(params, VLA, "C:\\gs\\bin\\gswin64.exe -dBATCH -dNOPAUSE -dQUIET -dFirstPage=%d -dLastPage=%d "
                    "-sOutputFile=\"%s\"_pAge_%%01d.%s -sDEVICE=%s -r%d "
                    "-dGraphicsAlphaBits=%s -sBandListStorage=memory "
                    "-dBufferSpace=99000 -dNumRenderingThreads=8 %s\"%s\"",
      spin1, spin2, pdfname, image_combo, sdevice_combo, UI->spinBox->value(), "4", "", str);
    system(params);

    for (y = 1; y < small_range; y++, big_range++) {
      snprintf(ren1, VLA, "%s_pAge_%d.%s", pdfname, y, image_combo);
      snprintf(ren2, VLA, "%s/%s_page_%d.%s", created_dir, BaseName, big_range, image_combo);
      remove(ren2);
      rename(ren1, ren2);
    }
}
