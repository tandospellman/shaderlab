#include "project_tree_panel.h"

#include "shader_file_utils.h"

#include <QAction>
#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPalette>
#include <QPoint>
#include <QSettings>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QWidget>

#include <algorithm>
#include <functional>

namespace
{
constexpr int path_role = Qt::UserRole;
constexpr int type_role = Qt::UserRole + 1;

QColor newFileHighlightColor(const QWidget* widget)
{
    const QPalette palette = widget->palette();
    const QColor base = palette.color(QPalette::Base);
    const QColor highlight = palette.color(QPalette::Highlight);
    constexpr qreal blend = 0.18;

    return QColor(
        base.red() + (highlight.red() - base.red()) * blend,
        base.green() + (highlight.green() - base.green()) * blend,
        base.blue() + (highlight.blue() - base.blue()) * blend
    );
}

QString abbreviatedPath(const QString& absolute_path)
{
    QStringList parts = absolute_path.split('/', Qt::SkipEmptyParts);

    if (parts.size() <= 2)
    {
        return absolute_path;
    }

    for (int i = 0; i < parts.size() - 2; ++i)
    {
        if (parts[i].size() > 3)
        {
            parts[i] = parts[i].left(3);
        }
    }

    return "/" + parts.join("/");
}

QString galleryDisplayName(const QFileInfo& file)
{
    QString name = file.completeBaseName();
    name.replace('_', ' ');

    if (!name.isEmpty())
    {
        name[0] = name[0].toUpper();
    }

    return name;
}

class ClickableLabel : public QLabel
{
public:
    explicit ClickableLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent)
    {
    }

    std::function<void()> on_double_click;

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (on_double_click)
        {
            on_double_click();
        }

        QLabel::mouseDoubleClickEvent(event);
    }
};
}

ProjectTreePanel::ProjectTreePanel(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderLabel("EXPLORER");
    setIndentation(16);
    setAnimated(true);
    setUniformRowHeights(true);
    setRootIsDecorated(true);

    connect(
        this,
        &QTreeWidget::itemDoubleClicked,
        this,
        [this](QTreeWidgetItem* item, int)
        {
            const QString type = item->data(0, type_role).toString();

            if (type != "file")
            {
                return;
            }

            emit fileActivationRequested(item->data(0, path_role).toString());
        }
    );

    connect(
        this,
        &QTreeWidget::itemCollapsed,
        this,
        [this](QTreeWidgetItem* item)
        {
            if (item->parent() != nullptr)
            {
                return;
            }

            const QString path = item->data(0, path_role).toString();

            if (!path.isEmpty())
            {
                collapsed_project_paths_.insert(path);
            }
        }
    );

    connect(
        this,
        &QTreeWidget::itemExpanded,
        this,
        [this](QTreeWidgetItem* item)
        {
            if (item->parent() != nullptr)
            {
                return;
            }

            const QString path = item->data(0, path_role).toString();

            if (!path.isEmpty())
            {
                collapsed_project_paths_.remove(path);
            }
        }
    );

    connect(&scan_timer_, &QTimer::timeout, this, &ProjectTreePanel::scanForChanges);

    loadProjectState();
    scan_timer_.start(1000);
    rebuildTree();
}

void ProjectTreePanel::addDirectory(const QString& path)
{
    QFileInfo info(path);

    if (!info.exists() || !info.isDir())
    {
        return;
    }

    const QString absolute_path = info.absoluteFilePath();

    if (!open_directories_.contains(absolute_path))
    {
        open_directories_.append(absolute_path);
    }

    rebuildTree();
    saveProjectState();
}

void ProjectTreePanel::addFile(const QString& path)
{
    QFileInfo info(path);

    if (!info.exists())
    {
        return;
    }

    const QString absolute_path = info.absoluteFilePath();

    if (!open_files_.contains(absolute_path))
    {
        open_files_.append(absolute_path);
    }

    rebuildTree();
    saveProjectState();
}

void ProjectTreePanel::setActiveShaderPath(const QString& path)
{
    active_shader_path_ = path;

    QFileInfo info(path);

    if (info.exists())
    {
        modified_flags_[path] = false;
        known_modified_times_[path] = info.lastModified();
    }

    rebuildTree();
}

void ProjectTreePanel::closeDirectory(const QString& directory_path)
{
    open_directories_.removeAll(directory_path);
    collapsed_project_paths_.remove(directory_path);

    for (auto it = known_modified_times_.begin(); it != known_modified_times_.end();)
    {
        if (it.key().startsWith(directory_path + "/"))
        {
            it = known_modified_times_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = modified_flags_.begin(); it != modified_flags_.end();)
    {
        if (it.key().startsWith(directory_path + "/"))
        {
            it = modified_flags_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    rebuildTree();
    saveProjectState();
}

void ProjectTreePanel::closeFile(const QString& file_path)
{
    open_files_.removeAll(file_path);
    collapsed_project_paths_.remove(file_path);
    known_modified_times_.remove(file_path);
    modified_flags_.remove(file_path);

    rebuildTree();
    saveProjectState();
}

void ProjectTreePanel::showRemoveFromProjectMenu(
    const QString& path,
    bool is_directory,
    const QPoint& global_position)
{
    QMenu menu(this);
    QAction* remove_action = menu.addAction("Remove from Project");

    connect(
        remove_action,
        &QAction::triggered,
        this,
        [this, path, is_directory]()
        {
            if (is_directory)
            {
                closeDirectory(path);
            }
            else
            {
                closeFile(path);
            }
        }
    );

    menu.exec(global_position);
}

void ProjectTreePanel::scanForChanges()
{
    struct FileUpdate
    {
        QString path;
        QDateTime modified;
    };

    QList<FileUpdate> updates;

    auto check_path =
        [&updates, this](const QFileInfo& info)
        {
            const QString path = info.absoluteFilePath();

            if (path == active_shader_path_)
            {
                return;
            }

            const QDateTime modified = info.lastModified();

            if (!known_modified_times_.contains(path) ||
                modified > known_modified_times_[path])
            {
                updates.append({path, modified});
            }
        };

    for (const QString& directory : open_directories_)
    {
        QDir root(directory);

        if (!root.exists())
        {
            continue;
        }

        QDirIterator iterator(
            root.absolutePath(),
            shaderFileFilters(),
            QDir::Files,
            QDirIterator::Subdirectories
        );

        while (iterator.hasNext())
        {
            iterator.next();
            check_path(iterator.fileInfo());
        }
    }

    for (const QString& file_path : open_files_)
    {
        QFileInfo info(file_path);

        if (info.exists())
        {
            check_path(info);
        }
    }

    if (updates.isEmpty())
    {
        return;
    }

    constexpr int bulk_change_threshold = 2;
    const bool is_bulk_change = updates.size() >= bulk_change_threshold;
    bool changed = false;

    for (const FileUpdate& update : updates)
    {
        known_modified_times_[update.path] = update.modified;

        if (!is_bulk_change)
        {
            modified_flags_[update.path] = true;
            changed = true;
        }
    }

    if (changed)
    {
        rebuildTree();
    }
}

void ProjectTreePanel::rebuildTree()
{
    clear();
    addGalleryToTree();

    for (const QString& directory : open_directories_)
    {
        addDirectoryToTree(directory);
    }

    for (const QString& file_path : open_files_)
    {
        addFileToTree(file_path);
    }

    for (int i = 0; i < topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* top_item = topLevelItem(i);
        const QString path = top_item->data(0, path_role).toString();

        if (!path.isEmpty())
        {
            top_item->setExpanded(!collapsed_project_paths_.contains(path));
        }
    }
}

void ProjectTreePanel::addGalleryToTree()
{
    const QString gallery_path =
        QDir(QCoreApplication::applicationDirPath()).filePath("shaders/gallery");

    QDir gallery_directory(gallery_path);

    if (!gallery_directory.exists())
    {
        return;
    }

    auto* gallery_item = new QTreeWidgetItem(this);
    gallery_item->setText(0, "Shader Gallery");
    gallery_item->setData(0, type_role, "gallery");
    gallery_item->setExpanded(true);

    const QStringList categories = {"essentials", "animated", "effects"};

    for (const QString& category : categories)
    {
        QDir category_directory(gallery_directory.filePath(category));

        if (!category_directory.exists())
        {
            continue;
        }

        auto* category_item = new QTreeWidgetItem(gallery_item);
        QString title = category;

        if (!title.isEmpty())
        {
            title[0] = title[0].toUpper();
        }

        category_item->setText(0, title);
        category_item->setData(0, type_role, "category");
        category_item->setExpanded(true);

        const QFileInfoList files = category_directory.entryInfoList(
            shaderFileFilters(),
            QDir::Files,
            QDir::Name
        );

        for (const QFileInfo& file : files)
        {
            QTreeWidgetItem* item = createFileItem(
                category_item,
                file,
                category_directory.absolutePath()
            );

            QString name = galleryDisplayName(file);

            if (file.absoluteFilePath() == active_shader_path_)
            {
                name = "▶ " + name;
            }

            item->setText(0, name);
        }
    }
}

void ProjectTreePanel::addDirectoryToTree(const QString& path)
{
    QDir root(path);

    if (!root.exists())
    {
        return;
    }

    auto* root_item = new QTreeWidgetItem(this);
    root_item->setData(0, path_role, root.absolutePath());
    root_item->setData(0, type_role, "directory");

    auto* root_widget = new QWidget(this);
    auto* root_layout = new QHBoxLayout(root_widget);
    root_layout->setContentsMargins(2, 0, 2, 0);
    root_layout->setSpacing(4);

    auto* path_label = new QLabel(root.dirName(), root_widget);
    path_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    path_label->setToolTip(root.absolutePath());
    path_label->setContextMenuPolicy(Qt::CustomContextMenu);

    const QString directory_path = root.absolutePath();

    connect(
        path_label,
        &QWidget::customContextMenuRequested,
        this,
        [this, path_label, directory_path](const QPoint& position)
        {
            showRemoveFromProjectMenu(
                directory_path,
                true,
                path_label->mapToGlobal(position)
            );
        }
    );

    auto* close_button = new QToolButton(root_widget);
    close_button->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    close_button->setToolTip("Remove from Project");
    close_button->setAutoRaise(true);
    close_button->setFixedSize(22, 22);

    root_layout->addWidget(path_label);
    root_layout->addStretch();
    root_layout->addWidget(close_button);

    setItemWidget(root_item, 0, root_widget);

    connect(
        close_button,
        &QToolButton::clicked,
        this,
        [this, directory_path]()
        {
            closeDirectory(directory_path);
        }
    );

    QFileInfoList files;

    QDirIterator iterator(
        root.absolutePath(),
        shaderFileFilters(),
        QDir::Files,
        QDirIterator::Subdirectories
    );

    while (iterator.hasNext())
    {
        iterator.next();
        files.append(iterator.fileInfo());
    }

    std::sort(
        files.begin(),
        files.end(),
        [&root](const QFileInfo& a, const QFileInfo& b)
        {
            return root.relativeFilePath(a.absoluteFilePath()).toLower() <
                   root.relativeFilePath(b.absoluteFilePath()).toLower();
        }
    );

    for (const QFileInfo& file : files)
    {
        createFileItem(root_item, file, directory_path);
    }
}

QTreeWidgetItem* ProjectTreePanel::createFileItem(
    QTreeWidgetItem* parent,
    const QFileInfo& file,
    const QString& root_directory)
{
    const QString path = file.absoluteFilePath();
    const bool is_standalone = root_directory.isEmpty();

    QTreeWidgetItem* item =
        parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(this);

    item->setData(0, path_role, path);
    item->setData(0, type_role, "file");

    QString display_name =
        is_standalone
            ? abbreviatedPath(path)
            : QDir(root_directory).relativeFilePath(path);

    const bool is_active = path == active_shader_path_;

    if (modified_flags_.value(path, false))
    {
        display_name = "● " + display_name;
    }

    if (is_active)
    {
        display_name = "▶ " + display_name;
    }

    if (!is_standalone)
    {
        item->setText(0, display_name);
        item->setToolTip(0, path);

        if (is_active)
        {
            item->setForeground(0, palette().color(QPalette::Highlight));
        }

        if (modified_flags_.value(path, false))
        {
            item->setBackground(0, QBrush(newFileHighlightColor(this)));
        }

        return item;
    }

    auto* row_widget = new QWidget(this);
    auto* row_layout = new QHBoxLayout(row_widget);
    row_layout->setContentsMargins(2, 0, 2, 0);
    row_layout->setSpacing(4);

    auto* name_label = new ClickableLabel(display_name, row_widget);
    name_label->setToolTip(path);
    name_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    name_label->on_double_click =
        [this, path]()
        {
            emit fileActivationRequested(path);
        };

    name_label->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(
        name_label,
        &QWidget::customContextMenuRequested,
        this,
        [this, name_label, path](const QPoint& position)
        {
            showRemoveFromProjectMenu(
                path,
                false,
                name_label->mapToGlobal(position)
            );
        }
    );

    if (is_active)
    {
        QPalette label_palette = name_label->palette();
        label_palette.setColor(
            QPalette::WindowText,
            palette().color(QPalette::Highlight)
        );
        name_label->setPalette(label_palette);
    }

    if (modified_flags_.value(path, false))
    {
        row_widget->setAutoFillBackground(true);
        QPalette row_palette = row_widget->palette();
        row_palette.setColor(QPalette::Window, newFileHighlightColor(row_widget));
        row_widget->setPalette(row_palette);
    }

    auto* close_button = new QToolButton(row_widget);
    close_button->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    close_button->setToolTip("Remove from Project");
    close_button->setAutoRaise(true);
    close_button->setFixedSize(22, 22);

    row_layout->addWidget(name_label);
    row_layout->addStretch();
    row_layout->addWidget(close_button);

    setItemWidget(item, 0, row_widget);

    connect(
        close_button,
        &QToolButton::clicked,
        this,
        [this, path]()
        {
            closeFile(path);
        }
    );

    return item;
}

void ProjectTreePanel::addFileToTree(const QString& path)
{
    QFileInfo file(path);

    if (file.exists() && file.isFile())
    {
        createFileItem(nullptr, file, QString());
    }
}

void ProjectTreePanel::loadProjectState()
{
    QSettings settings("Tando", "ShaderViewer");

    open_directories_ =
        settings.value("paths/open_directories").toStringList();

    if (open_directories_.isEmpty())
    {
        open_directories_.append(QDir::currentPath());
    }

    open_files_ =
        settings.value("paths/open_files").toStringList();
}

void ProjectTreePanel::saveProjectState()
{
    QSettings settings("Tando", "ShaderViewer");
    settings.setValue("paths/open_directories", open_directories_);
    settings.setValue("paths/open_files", open_files_);
}
