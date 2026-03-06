#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QPushButton>

class FilePicker : public QWidget {
    Q_OBJECT

public:
    explicit FilePicker(char const *label,
                        char const *initial_value = "",
                        QWidget *parent = nullptr,
                        bool accept_drops = true,
                        std::function<void ()> pre_browse_callback = nullptr);

    QString file() const;

public slots:
    void setFile(QString const &path);
    void restorePrevFileSilently();
    void onBrowse();

signals:
    void fileChanged(QString const &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public:
    QPushButton *browseButton;

private:
    QString prev_value;
    QLineEdit *fileEdit;
    std::function<void ()> pre_browse_callback;
};
