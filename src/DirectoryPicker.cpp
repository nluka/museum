#include <QMimeData>
#include <QFileInfo>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>

#include "DirectoryPicker.hpp"

DirectoryPicker::DirectoryPicker(char const *label,
                                 char const *initial_value,
                                 QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    pathEdit = new QLineEdit(this);
    pathEdit->setText(initial_value);

    QPushButton *browseButton = new QPushButton("Browse…", this);
    QLabel *qlabel = new QLabel(label);

    layout->addWidget(qlabel);
    layout->addWidget(pathEdit);
    layout->addWidget(browseButton);

    connect(browseButton, &QPushButton::clicked,
            this, &DirectoryPicker::onBrowse);

    setAcceptDrops(true);
}

QString DirectoryPicker::path() const {
    return pathEdit->text();
}

void DirectoryPicker::setPath(QString const &path) {
    pathEdit->setText(path);
    emit pathChanged(path);
}

void DirectoryPicker::onBrowse() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Directory",
        pathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        setPath(dir);
    }
}

void DirectoryPicker::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString p = urls.first().toLocalFile();
            QFileInfo info(p);
            if (info.isDir()) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void DirectoryPicker::dropEvent(QDropEvent *event) {
    auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString p = urls.first().toLocalFile();
        QFileInfo info(p);
        if (info.isDir()) {
            setPath(p);
        }
    }
}
