#pragma once

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QTreeWidget>

class QFileInfo;
class QPoint;
class QTreeWidgetItem;

class ProjectTreePanel : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ProjectTreePanel(QWidget* parent = nullptr);

    void addDirectory(const QString& path);
    void addFile(const QString& path);
    void setActiveShaderPath(const QString& path);

signals:
    void fileActivationRequested(const QString& path);

private:
    void rebuildTree();
    void addGalleryToTree();
    void addDirectoryToTree(const QString& path);

    QTreeWidgetItem* createFileItem(
        QTreeWidgetItem* parent,
        const QFileInfo& file,
        const QString& root_directory
    );

    void addFileToTree(const QString& path);
    void closeDirectory(const QString& directory_path);
    void closeFile(const QString& file_path);

    void showRemoveFromProjectMenu(
        const QString& path,
        bool is_directory,
        const QPoint& global_position
    );

    void scanForChanges();
    void loadProjectState();
    void saveProjectState();

    QStringList open_directories_;
    QStringList open_files_;
    QHash<QString, QDateTime> known_modified_times_;
    QHash<QString, bool> modified_flags_;
    QSet<QString> collapsed_project_paths_;
    QString active_shader_path_;
    QTimer scan_timer_;
};
