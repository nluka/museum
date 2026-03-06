#include <QSignalBlocker>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMimeData>

#include "FilePicker.hpp"

FilePicker::FilePicker(char const *label,
                       char const *initial_value,
                       QWidget *parent,
                       bool accept_drops,
                       std::function<void ()> pre_browse_callback_)
    : QWidget(parent)
    , pre_browse_callback(pre_browse_callback_)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    fileEdit = new QLineEdit(this);
    fileEdit->setText(initial_value);

    browseButton = new QPushButton("Browse…", this);
    QLabel *qlabel = new QLabel(label);

    layout->addWidget(qlabel);
    layout->addWidget(fileEdit);
    layout->addWidget(browseButton);

    connect(browseButton, &QPushButton::clicked,
            this, &FilePicker::onBrowse);

    setAcceptDrops(accept_drops);
}

QString FilePicker::file() const {
    return fileEdit->text();
}

void FilePicker::setFile(QString const &path) {
    prev_value = fileEdit->text();
    fileEdit->setText(path);
    emit fileChanged(path);
}

void FilePicker::restorePrevFileSilently() {
    QSignalBlocker blocker(fileEdit);
    fileEdit->setText(prev_value);
}

void FilePicker::onBrowse() {
    if (pre_browse_callback != nullptr) {
        pre_browse_callback();
    }

    QString path = QFileDialog::getOpenFileName(
        this,
        "Select File",
        fileEdit->text()
    );

    if (!path.isEmpty())
        setFile(path);
}

void FilePicker::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            QFileInfo info(path);
            if (info.isFile()) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void FilePicker::dropEvent(QDropEvent *event) {
    auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString path = urls.first().toLocalFile();
        QFileInfo info(path);
        if (info.isFile()) {
            setFile(path);
        }
    }
}
