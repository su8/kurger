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
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QProcess>
#include <QSignalBlocker>
#include <QUrl>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

static void RaiseMsg(const QString &title, const QString &message, unsigned short int isWarning);

/*
 * Ghostscript treats '%' in -sOutputFile as the start of a page-number
 * placeholder. A literal '%' in the input path has to be doubled.
 */
static QString escapeGhostscriptPattern(const QString &text)
{
    QString escaped;
    escaped.reserve(text.size() * 2);
    for (const QChar character : text)
    {
        escaped += character;
        if (character == QLatin1Char('%'))
        {
            escaped += character;
        }
    }
    return escaped;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* Load the icon from the executable instead of the current directory. */
    setWindowIcon(QIcon(QStringLiteral(":/media/icon.xpm")));

    ui->comboBox->addItems({"png", "jpg", "bmp", "tiff"});
    ui->comboBox_2->addItems({"png16m", "pngalpha", "pnggray", "jpeg", "jpegcmyk", "jpeggray", "bmp16m", "bmpgray", "tiff24nc", "tiffgray"});

    /* Tray icon, based on https://amin-ahmadi.com/2019/06/20/implementing-minimize-to-tray-functionality-using-qt-c
    closing = 0U;

    exitAction = new QAction(tr("&Exit"), this);
    connect(exitAction, &QAction::triggered, [this]() { closing = 1U; close(); });

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(exitAction);
    sysTrayIcon = new QSystemTrayIcon(this);
    sysTrayIcon->setContextMenu(trayIconMenu);
    sysTrayIcon->setIcon(QIcon("media/icon.xpm"));
    sysTrayIcon->show();

    connect(sysTrayIcon, &QSystemTrayIcon::activated, [this](auto reason)
    {
        if (reason == QSystemTrayIcon::Trigger)
        {
            if (isVisible()) { hide(); }
            else { show(); activateWindow(); }
        }
    });
*/
}

MainWindow::~MainWindow()
{
    /* Tray icon
    delete sysTrayIcon;
    delete trayIconMenu;
    delete exitAction;
    */

    delete ui;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

/* Tray icon
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (closing == 1U)
    {
        event->accept();
    }
    else
    {
        this->hide();
        event->ignore();
    }
}
*/

void MainWindow::on_pushButton_clicked()
{
    if (m_pdfFile.isEmpty())
    {
        RaiseMsg("No pdf selected", "You must select some PDF file first and then click this button.", 1U);
        return;
    }
    pdf2img(m_pdfFile);
}

void MainWindow::on_pushButton_2_clicked()
{
    const QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Open PDF file"),
                m_lastDirectory.isEmpty() ? QDir::currentPath() : m_lastDirectory,
                tr("PDF File (*.pdf)"));
    if (filename.isEmpty())
    {
        return;
    }

    m_pdfFile = filename;
    m_lastDirectory = QFileInfo(filename).absolutePath();
    pdf2img(filename);
}

static void RaiseMsg(const QString &title, const QString &message, unsigned short int isWarning)
{
    QMessageBox msg;
    msg.setText(message);
    msg.setIcon((isWarning == 1U) ? QMessageBox::Warning : QMessageBox::Information);
    msg.setWindowTitle(title);
    msg.setWindowIcon(QIcon(QStringLiteral(":/media/icon.xpm")));
    msg.exec();
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    /* The default sDevice for png, jpg, bmp and tiff. */
    static const int defaultDevice[] = {0, 3, 6, 8};
    const int deviceCount = static_cast<int>(sizeof(defaultDevice) / sizeof(defaultDevice[0]));

    if (index < 0 || index >= deviceCount)
    {
        return;
    }

    /* Do not let the second combo box change the image format back. */
    const QSignalBlocker blocker(ui->comboBox_2);
    ui->comboBox_2->setCurrentIndex(defaultDevice[index]);
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    int imageCombo = 0;

    if (index >= 3 && index <= 5)       /* jpeg, jpegcmyk, jpeggray */
    {
        imageCombo = 1;
    }
    else if (index >= 6 && index <= 7)  /* bmp16m, bmpgray */
    {
        imageCombo = 2;
    }
    else if (index >= 8)                /* tiff24nc, tiffgray */
    {
        imageCombo = 3;
    }

    const QSignalBlocker blocker(ui->comboBox);
    ui->comboBox->setCurrentIndex(imageCombo);
}

void MainWindow::pdf2img(const QString &pdfPath)
{
    const int firstPage = ui->spinBox_2->value();
    const int lastPage = ui->spinBox_3->value();
    const int resolution = ui->spinBox->value();
    const QString imageFormat = ui->comboBox->currentText();
    const QString device = ui->comboBox_2->currentText();

    if (firstPage > lastPage)
    {
        RaiseMsg("Reversed Numbers", "From page can't be greater than To page.", 1U);
        return;
    }

    const QFileInfo pdfInfo(pdfPath);
    if (!pdfInfo.exists() || !pdfInfo.isFile())
    {
        RaiseMsg("No pdf selected", "The selected PDF file does not exist.", 1U);
        return;
    }

    if (pdfInfo.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) != 0)
    {
        RaiseMsg("Not a PDF file", "Please select a file with the .pdf extension.", 1U);
        return;
    }

    const QString outputDir = pdfPath + QStringLiteral("_converted");
    if (!QDir().mkpath(outputDir))
    {
        RaiseMsg("Cannot create directory", "Could not create the output directory:\n" + outputDir, 1U);
        return;
    }

    QString gsProgram = qEnvironmentVariable("KURGER_GS");
    if (gsProgram.isEmpty())
    {
#ifdef Q_OS_WIN
        gsProgram = QStringLiteral("C:/gs/bin/gswin64.exe");
#else
        gsProgram = QStringLiteral("gs");
#endif
    }

    const QString outputPattern = escapeGhostscriptPattern(pdfPath) + QStringLiteral("_pAge_%01d.") + imageFormat;
    const QStringList arguments{
        QStringLiteral("-dBATCH"),
        QStringLiteral("-dNOPAUSE"),
        QStringLiteral("-dQUIET"),
        QStringLiteral("-dFirstPage=%1").arg(firstPage),
        QStringLiteral("-dLastPage=%1").arg(lastPage),
        QStringLiteral("-sOutputFile=") + outputPattern,
        QStringLiteral("-sDEVICE=") + device,
        QStringLiteral("-r%1").arg(resolution),
        QStringLiteral("-dGraphicsAlphaBits=4"),
        QStringLiteral("-sBandListStorage=memory"),
        QStringLiteral("-dBufferSpace=99000"),
        QStringLiteral("-dNumRenderingThreads=8"),
        pdfPath
    };

    RaiseMsg("Please wait...", "Please wait until we convert the requested images.", 0U);

    /* QProcess takes an argument list, so file names never reach a shell. */
    const int exitCode = QProcess::execute(gsProgram, arguments);
    if (exitCode != 0)
    {
        RaiseMsg("Conversion failed",
                 QStringLiteral("Ghostscript could not convert the file (exit code %1).\n"
                                "Make sure Ghostscript is installed and that the PDF is valid.")
                     .arg(exitCode),
                 1U);
        return;
    }

    const QString baseName = pdfInfo.completeBaseName();
    const int pageCount = lastPage - firstPage + 1;
    int failedRenames = 0;

    for (int page = 0; page < pageCount; ++page)
    {
        const QString source = pdfPath + QStringLiteral("_pAge_%1.").arg(page + 1) + imageFormat;
        const QString target = QDir(outputDir).filePath(
            QStringLiteral("%1_page_%2.%3").arg(baseName).arg(firstPage + page).arg(imageFormat));

        QFile::remove(target);
        if (!QFile::rename(source, target))
        {
            ++failedRenames;
        }
    }

    if (failedRenames != 0)
    {
        RaiseMsg("Conversion problem",
                 QStringLiteral("%1 of %2 converted pages could not be renamed.")
                     .arg(failedRenames).arg(pageCount),
                 1U);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
}
