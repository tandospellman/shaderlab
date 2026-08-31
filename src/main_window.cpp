#include "main_window.h"

#include "param_form.h"
#include "project_tree_panel.h"
#include "render_widget.h"
#include "shader_file_utils.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , render_widget_(nullptr)
    , param_form_(nullptr)
    , project_dock_(nullptr)
    , uniform_dock_(nullptr)
    , log_dock_(nullptr)
    , project_tree_panel_(nullptr)
    , log_output_(nullptr)
    , shader_path_label_(nullptr)
    , modified_time_label_(nullptr)
    , version_label_(nullptr)
    , auto_reload_status_label_(nullptr)
    , main_toolbar_(nullptr)
    , open_shader_action_(nullptr)
    , open_directory_action_(nullptr)
    , paste_file_action_(nullptr)
    , screenshot_action_(nullptr)
    , pause_time_action_(nullptr)
    , reset_time_action_(nullptr)
    , reload_shader_action_(nullptr)
    , save_preset_action_(nullptr)
    , load_preset_action_(nullptr)
    , exit_action_(nullptr)
    , lock_toolbar_action_(nullptr)
    , last_folder_("shaders")
    , shader_version_(0)
    , locale_(QLocale::system())
    , reset_uniforms_action_(nullptr)
    , lock_uniform_dock_action_(nullptr)
    , save_uniform_file_action_(nullptr)
    , load_uniform_file_action_(nullptr)
    , clear_log_action_(nullptr)
    , auto_reload_action_(nullptr)
{
    setupUi();
    setupDocks();
    setupMenus();
    setupToolbar();

    connect(render_widget_, &RenderWidget::shaderStatusChanged,
            this, &MainWindow::onShaderStatusChanged);
    connect(render_widget_, &RenderWidget::paramsChanged,
            this, &MainWindow::onParamsChanged);
    connect(param_form_, &ParamForm::paramsChanged,
            render_widget_, QOverload<>::of(&RenderWidget::update));
    connect(&shader_watch_timer_, &QTimer::timeout,
            this, &MainWindow::checkCurrentShaderForChanges);

    shader_watch_timer_.start(1000);
    loadSettings();
    updateShaderInfo();
    updateWindowTitle();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    resize(1440, 900);
    setDockNestingEnabled(true);

    render_widget_ = new RenderWidget(this);

    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(render_widget_, 1);
    setCentralWidget(central);

    shader_path_label_ = new QLabel(this);
    modified_time_label_ = new QLabel(this);
    version_label_ = new QLabel(this);
    auto_reload_status_label_ = new QLabel("● Auto Reload", this);

    shader_path_label_->setObjectName("shaderName");
    auto_reload_status_label_->setObjectName("autoReloadEnabled");

    statusBar()->addPermanentWidget(shader_path_label_, 1);
    statusBar()->addPermanentWidget(modified_time_label_);
    statusBar()->addPermanentWidget(version_label_);
    statusBar()->addPermanentWidget(auto_reload_status_label_);
    statusBar()->showMessage("Ready");
}

void MainWindow::setupDocks()
{
    project_dock_ = new QDockWidget("PROJECT", this);
    project_dock_->setObjectName("project_dock");
    project_tree_panel_ = new ProjectTreePanel(project_dock_);
    project_dock_->setWidget(project_tree_panel_);
    project_dock_->setMinimumWidth(210);
    addDockWidget(Qt::LeftDockWidgetArea, project_dock_);

    connect(project_tree_panel_, &ProjectTreePanel::fileActivationRequested,
            this, &MainWindow::openProjectFilePath);

    uniform_dock_ = new QDockWidget("UNIFORMS", this);
    uniform_dock_->setObjectName("uniform_dock");
    param_form_ = new ParamForm(uniform_dock_);
    uniform_dock_->setWidget(param_form_);
    uniform_dock_->setMinimumWidth(230);
    addDockWidget(Qt::RightDockWidgetArea, uniform_dock_);

    log_dock_ = new QDockWidget("CONSOLE", this);
    log_dock_->setObjectName("log_dock");
    log_output_ = new QPlainTextEdit(log_dock_);
    log_output_->setReadOnly(true);
    log_output_->setPlaceholderText("Shader compiler output will appear here...");
    log_dock_->setWidget(log_output_);
    log_dock_->setMinimumHeight(120);
    addDockWidget(Qt::BottomDockWidgetArea, log_dock_);

    resizeDocks({project_dock_, uniform_dock_}, {230, 260}, Qt::Horizontal);
    resizeDocks({log_dock_}, {160}, Qt::Vertical);
}

void MainWindow::setupMenus()
{
    open_shader_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "Open Shader", this);
    open_shader_action_->setShortcut(QKeySequence::Open);

    open_directory_action_ = new QAction(style()->standardIcon(QStyle::SP_DirOpenIcon), "Open Directory", this);
    open_directory_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));

    paste_file_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Paste File from Clipboard", this);
    paste_file_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));

    screenshot_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save Screenshot", this);
    screenshot_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));

    pause_time_action_ = new QAction(style()->standardIcon(QStyle::SP_MediaPause), "Pause Time", this);
    pause_time_action_->setCheckable(true);
    pause_time_action_->setShortcut(QKeySequence(Qt::Key_Space));

    reset_time_action_ = new QAction(style()->standardIcon(QStyle::SP_BrowserReload), "Reset Time", this);

    reload_shader_action_ = new QAction(style()->standardIcon(QStyle::SP_BrowserReload), "Reload Shader", this);
    reload_shader_action_->setShortcut(QKeySequence(Qt::Key_F5));

    auto_reload_action_ = new QAction("Auto Reload", this);
    auto_reload_action_->setCheckable(true);
    auto_reload_action_->setChecked(true);
    auto_reload_action_->setToolTip("Automatically reload the shader when its file changes");

    save_preset_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save Preset", this);
    save_preset_action_->setShortcut(QKeySequence::Save);

    load_preset_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "Load Preset", this);
    load_preset_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));

    save_uniform_file_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save Uniform File", this);
    save_uniform_file_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    load_uniform_file_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "Load Uniform File", this);
    load_uniform_file_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));

    reset_uniforms_action_ = new QAction(style()->standardIcon(QStyle::SP_BrowserReload), "Reset Uniforms", this);
    reset_uniforms_action_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));

    lock_uniform_dock_action_ = new QAction("Lock Uniform Dock", this);
    lock_uniform_dock_action_->setCheckable(true);

    lock_toolbar_action_ = new QAction("Lock Toolbar", this);
    lock_toolbar_action_->setCheckable(true);

    clear_log_action_ = new QAction(style()->standardIcon(QStyle::SP_DialogResetButton), "Clear Console", this);
    exit_action_ = new QAction("Exit", this);
    exit_action_->setShortcut(QKeySequence::Quit);

    connect(open_shader_action_, &QAction::triggered, this, &MainWindow::openShader);
    connect(open_directory_action_, &QAction::triggered, this, &MainWindow::openDirectory);
    connect(paste_file_action_, &QAction::triggered, this, &MainWindow::pasteFileFromClipboard);
    connect(screenshot_action_, &QAction::triggered, this, &MainWindow::saveScreenshot);
    connect(reload_shader_action_, &QAction::triggered, this, &MainWindow::reloadShader);
    connect(reset_time_action_, &QAction::triggered, this, &MainWindow::resetShaderTime);
    connect(save_preset_action_, &QAction::triggered, this, &MainWindow::savePreset);
    connect(load_preset_action_, &QAction::triggered, this, &MainWindow::loadPreset);
    connect(save_uniform_file_action_, &QAction::triggered, this, &MainWindow::saveUniformFile);
    connect(load_uniform_file_action_, &QAction::triggered, this, &MainWindow::loadUniformFile);
    connect(reset_uniforms_action_, &QAction::triggered, this, &MainWindow::resetUniforms);
    connect(lock_uniform_dock_action_, &QAction::toggled, this, &MainWindow::setUniformDockLocked);
    connect(lock_toolbar_action_, &QAction::toggled, this, &MainWindow::setToolbarLocked);
    connect(exit_action_, &QAction::triggered, this, &MainWindow::close);

    connect(clear_log_action_, &QAction::triggered, this, [this]() { log_output_->clear(); });

    connect(
        pause_time_action_,
        &QAction::toggled,
        this,
        [this](bool paused)
        {
            render_widget_->setTimePaused(paused);
            pause_time_action_->setText(paused ? "Resume Time" : "Pause Time");
            pause_time_action_->setIcon(
                style()->standardIcon(paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause)
            );
            statusBar()->showMessage(paused ? "Shader time paused" : "Shader time resumed", 2000);
        }
    );

    connect(
        auto_reload_action_,
        &QAction::toggled,
        this,
        [this](bool checked)
        {
            auto_reload_status_label_->setText(checked ? "● Auto Reload" : "○ Auto Reload");
            auto_reload_status_label_->setObjectName(checked ? "autoReloadEnabled" : "");
            auto_reload_status_label_->style()->unpolish(auto_reload_status_label_);
            auto_reload_status_label_->style()->polish(auto_reload_status_label_);
        }
    );

    QMenu* file_menu = menuBar()->addMenu("File");
    file_menu->addAction(open_shader_action_);
    file_menu->addAction(open_directory_action_);
    file_menu->addAction(paste_file_action_);
    file_menu->addSeparator();
    file_menu->addAction(screenshot_action_);
    file_menu->addSeparator();
    file_menu->addAction(save_preset_action_);
    file_menu->addAction(load_preset_action_);
    file_menu->addSeparator();
    file_menu->addAction(exit_action_);

    QMenu* shader_menu = menuBar()->addMenu("Shader");
    shader_menu->addAction(reload_shader_action_);
    shader_menu->addAction(auto_reload_action_);
    shader_menu->addSeparator();
    shader_menu->addAction(pause_time_action_);
    shader_menu->addAction(reset_time_action_);

    QMenu* uniforms_menu = menuBar()->addMenu("Uniforms");
    uniforms_menu->addAction(save_uniform_file_action_);
    uniforms_menu->addAction(load_uniform_file_action_);
    uniforms_menu->addSeparator();
    uniforms_menu->addAction(reset_uniforms_action_);

    QMenu* view_menu = menuBar()->addMenu("View");
    view_menu->addAction(project_dock_->toggleViewAction());
    view_menu->addAction(uniform_dock_->toggleViewAction());
    view_menu->addAction(log_dock_->toggleViewAction());
    view_menu->addSeparator();
    view_menu->addAction(clear_log_action_);
    view_menu->addSeparator();
    view_menu->addAction(lock_toolbar_action_);
    view_menu->addAction(lock_uniform_dock_action_);
}

void MainWindow::setupToolbar()
{
    main_toolbar_ = addToolBar("ShaderLab Toolbar");
    main_toolbar_->setObjectName("main_toolbar");
    main_toolbar_->setMovable(true);
    main_toolbar_->setFloatable(false);
    main_toolbar_->setIconSize(QSize(18, 18));

    main_toolbar_->addAction(open_directory_action_);
    main_toolbar_->addAction(open_shader_action_);
    main_toolbar_->addSeparator();
    main_toolbar_->addAction(reload_shader_action_);
    main_toolbar_->addAction(auto_reload_action_);
    main_toolbar_->addSeparator();
    main_toolbar_->addAction(pause_time_action_);
    main_toolbar_->addAction(reset_time_action_);
    main_toolbar_->addSeparator();
    main_toolbar_->addAction(screenshot_action_);
}

void MainWindow::loadSettings()
{
    QSettings settings("Tando", "ShaderViewer");

    last_folder_ = settings.value("paths/last_folder", "shaders").toString();
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/state").toByteArray());

    const bool toolbar_locked = settings.value("toolbar/locked", false).toBool();
    lock_toolbar_action_->setChecked(toolbar_locked);
    setToolbarLocked(toolbar_locked);

    const bool uniform_dock_locked = settings.value("uniform_dock/locked", false).toBool();
    lock_uniform_dock_action_->setChecked(uniform_dock_locked);
    setUniformDockLocked(uniform_dock_locked);

    const QString last_shader_path = settings.value("shader/path").toString();

    if (!last_shader_path.isEmpty())
    {
        QFileInfo info(last_shader_path);

        if (info.exists())
        {
            render_widget_->setShaderPath(last_shader_path);
            current_shader_ = info.absoluteFilePath();
            last_folder_ = info.absolutePath();
            shader_version_ = 0;
            project_tree_panel_->addFile(info.absoluteFilePath());
            project_tree_panel_->setActiveShaderPath(info.absoluteFilePath());
            current_shader_modified_time_ = info.lastModified();
        }
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("Tando", "ShaderViewer");
    settings.setValue("paths/last_folder", last_folder_);
    settings.setValue("shader/path", render_widget_->shaderPath());
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    settings.setValue("uniform_dock/locked", lock_uniform_dock_action_->isChecked());
    settings.setValue("toolbar/locked", lock_toolbar_action_->isChecked());
}

void MainWindow::saveScreenshot()
{
    QFileInfo shader_info(render_widget_->shaderPath());
    QString base_name = shader_info.completeBaseName();

    if (base_name.isEmpty())
    {
        base_name = "shader";
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        "Save Shader Screenshot",
        last_folder_ + "/" + base_name + ".png",
        "PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;BMP Image (*.bmp)"
    );

    if (path.isEmpty())
    {
        return;
    }

    if (QFileInfo(path).suffix().isEmpty())
    {
        path += ".png";
    }

    if (!render_widget_->saveScreenshot(path))
    {
        onShaderStatusChanged("Failed to save screenshot.");
        return;
    }

    last_folder_ = QFileInfo(path).absolutePath();
    onShaderStatusChanged("Screenshot saved: " + path);
    saveSettings();
}

void MainWindow::resetShaderTime()
{
    render_widget_->resetShaderTime();
    onShaderStatusChanged("Shader time reset to 0.");
}

void MainWindow::openShader()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this,
        "Open Fragment Shader",
        last_folder_,
        "GLSL Files (*.frag *.glsl *.vert)"
    );

    if (paths.isEmpty())
    {
        return;
    }

    for (const QString& path : paths)
    {
        QFileInfo info(path);

        if (!info.exists())
        {
            continue;
        }

        last_folder_ = info.absolutePath();
        project_tree_panel_->addFile(info.absoluteFilePath());
    }

    saveSettings();
    openProjectFilePath(QFileInfo(paths.first()).absoluteFilePath());
}

void MainWindow::openDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        "Open Project Directory",
        last_folder_
    );

    if (directory.isEmpty())
    {
        return;
    }

    QFileInfo info(directory);

    if (!info.exists() || !info.isDir())
    {
        return;
    }

    const QString absolute_path = info.absoluteFilePath();
    project_tree_panel_->addDirectory(absolute_path);
    last_folder_ = absolute_path;
    saveSettings();
}

void MainWindow::pasteFileFromClipboard()
{
    const QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mime_data = clipboard->mimeData();

    if (mime_data == nullptr)
    {
        return;
    }

    QString source_path;

    if (mime_data->hasUrls())
    {
        for (const QUrl& url : mime_data->urls())
        {
            const QString local_path = url.toLocalFile();
            QFileInfo info(local_path);

            if (info.exists() && info.isFile())
            {
                source_path = info.absoluteFilePath();
                break;
            }
        }
    }

    if (source_path.isEmpty() && mime_data->hasText())
    {
        const QStringList lines = mime_data->text().trimmed().split('\n', Qt::SkipEmptyParts);

        for (const QString& line : lines)
        {
            QFileInfo info(line.trimmed());

            if (info.exists() && info.isFile())
            {
                source_path = info.absoluteFilePath();
                break;
            }
        }
    }

    if (!source_path.isEmpty())
    {
        QFileInfo source_info(source_path);
        const QString destination = QFileDialog::getSaveFileName(
            this,
            "Save Clipboard File As",
            QDir(last_folder_).filePath(source_info.fileName()),
            "All Files (*)"
        );

        if (destination.isEmpty())
        {
            return;
        }

        const QString absolute_destination = QFileInfo(destination).absoluteFilePath();

        if (absolute_destination != source_info.absoluteFilePath())
        {
            if (QFileInfo::exists(absolute_destination))
            {
                const auto result = QMessageBox::question(
                    this,
                    "Replace File",
                    "The destination file already exists. Replace it?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No
                );

                if (result != QMessageBox::Yes)
                {
                    return;
                }

                if (!QFile::remove(absolute_destination))
                {
                    onShaderStatusChanged("Failed to replace destination file.");
                    return;
                }
            }

            if (!QFile::copy(source_info.absoluteFilePath(), absolute_destination))
            {
                onShaderStatusChanged("Failed to copy clipboard file.");
                return;
            }
        }

        last_folder_ = QFileInfo(absolute_destination).absolutePath();
        project_tree_panel_->addFile(absolute_destination);

        if (isShaderFile(QFileInfo(absolute_destination)))
        {
            openProjectFilePath(absolute_destination);
        }

        saveSettings();
        onShaderStatusChanged("Clipboard file saved: " + absolute_destination);
        return;
    }

    if (!mime_data->hasText())
    {
        onShaderStatusChanged("Clipboard does not contain a file or text.");
        return;
    }

    const QString destination = QFileDialog::getSaveFileName(
        this,
        "Save Clipboard Text As",
        QDir(last_folder_).filePath("clipboard.frag"),
        "GLSL Files (*.frag *.glsl *.vert);;Text Files (*.txt);;All Files (*)"
    );

    if (destination.isEmpty())
    {
        return;
    }

    QFile file(destination);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        onShaderStatusChanged("Failed to save clipboard text.");
        return;
    }

    file.write(mime_data->text().toUtf8());
    file.close();

    const QString absolute_destination = QFileInfo(destination).absoluteFilePath();
    last_folder_ = QFileInfo(absolute_destination).absolutePath();
    project_tree_panel_->addFile(absolute_destination);

    if (isShaderFile(QFileInfo(absolute_destination)))
    {
        openProjectFilePath(absolute_destination);
    }

    saveSettings();
    onShaderStatusChanged("Clipboard text saved: " + absolute_destination);
}

void MainWindow::reloadShader()
{
    if (!render_widget_->reloadShader())
    {
        return;
    }

    shader_version_++;

    QFileInfo info(render_widget_->shaderPath());

    if (info.exists())
    {
        current_shader_ = info.absoluteFilePath();
        current_shader_modified_time_ = info.lastModified();
        project_tree_panel_->setActiveShaderPath(current_shader_);
    }

    updateShaderInfo();
    updateWindowTitle();
}

void MainWindow::savePreset()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Save Shader Preset", last_folder_ + "/preset.json", "JSON Files (*.json)");

    if (path.isEmpty())
    {
        return;
    }

    QFileInfo info(path);
    last_folder_ = info.absolutePath();
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly))
    {
        onShaderStatusChanged("Failed to save preset.");
        return;
    }

    QJsonDocument document(render_widget_->savePreset());
    file.write(document.toJson(QJsonDocument::Indented));
    onShaderStatusChanged("Preset saved: " + path);
    saveSettings();
}

void MainWindow::loadPreset()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Load Shader Preset", last_folder_, "JSON Files (*.json)");

    if (path.isEmpty())
    {
        return;
    }

    QFileInfo info(path);
    last_folder_ = info.absolutePath();
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
    {
        onShaderStatusChanged("Failed to load preset.");
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson(file.readAll());

    if (!document.isObject())
    {
        onShaderStatusChanged("Invalid preset file.");
        return;
    }

    render_widget_->loadPreset(document.object());
    onShaderStatusChanged("Preset loaded: " + path);
    saveSettings();
}

void MainWindow::openProjectFilePath(const QString& path)
{
    QFileInfo info(path);

    if (!info.exists() || !info.isFile() || !isShaderFile(info))
    {
        return;
    }

    if (!render_widget_->setShaderPath(path))
    {
        return;
    }

    current_shader_ = info.absoluteFilePath();
    current_shader_modified_time_ = info.lastModified();
    last_folder_ = info.absolutePath();
    shader_version_++;

    project_tree_panel_->setActiveShaderPath(current_shader_);
    updateShaderInfo();
    updateWindowTitle();
    saveSettings();
}

void MainWindow::onShaderStatusChanged(const QString& message)
{
    if (!message.startsWith("Failed to load shader file"))
    {
        const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
        log_output_->appendPlainText(time + "  " + message);
        statusBar()->showMessage(message, 3000);
    }

    updateShaderInfo();
    updateWindowTitle();
}

void MainWindow::onParamsChanged()
{
    param_form_->rebuild(render_widget_->params());
}

void MainWindow::updateShaderInfo()
{
    QFileInfo info(render_widget_->shaderPath());

    if (info.exists())
    {
        shader_path_label_->setText(info.fileName());
        shader_path_label_->setToolTip(info.absoluteFilePath());
        modified_time_label_->setText(
            "Updated " + locale_.toString(info.lastModified().time(), QLocale::ShortFormat)
        );
    }
    else
    {
        shader_path_label_->setText("No shader loaded");
        modified_time_label_->clear();
    }

    version_label_->setText(QString("v%1").arg(shader_version_));
}

void MainWindow::updateWindowTitle()
{
    QFileInfo info(render_widget_->shaderPath());

    setWindowTitle(
        info.exists()
            ? info.fileName() + " — ShaderLab"
            : "ShaderLab"
    );
}

void MainWindow::setToolbarLocked(bool locked)
{
    if (main_toolbar_ != nullptr)
    {
        main_toolbar_->setMovable(!locked);
    }
}

void MainWindow::resetUniforms()
{
    render_widget_->resetUniforms();
    onShaderStatusChanged("Uniforms reset.");
}

void MainWindow::setUniformDockLocked(bool locked)
{
    uniform_dock_->setFeatures(
        locked
            ? QDockWidget::NoDockWidgetFeatures
            : QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable |
              QDockWidget::DockWidgetClosable
    );
}

void MainWindow::saveUniformFile()
{
    if (render_widget_->saveUniformFile())
    {
        onShaderStatusChanged("Saved uniform file: " + render_widget_->uniformSavePath());
    }
    else
    {
        onShaderStatusChanged("Failed to save uniform file.");
    }
}

void MainWindow::loadUniformFile()
{
    if (render_widget_->loadUniformFile())
    {
        onShaderStatusChanged("Loaded uniform file: " + render_widget_->uniformSavePath());
    }
    else
    {
        onShaderStatusChanged("No uniform file found.");
    }
}

void MainWindow::checkCurrentShaderForChanges()
{
    if (auto_reload_action_ == nullptr || !auto_reload_action_->isChecked())
    {
        return;
    }

    const QString path = render_widget_->shaderPath();

    if (path.isEmpty())
    {
        return;
    }

    QFileInfo info(path);

    if (!info.exists() || !info.isFile())
    {
        return;
    }

    const QDateTime modified = info.lastModified();

    if (!current_shader_modified_time_.isValid())
    {
        current_shader_modified_time_ = modified;
        return;
    }

    if (modified <= current_shader_modified_time_)
    {
        return;
    }

    current_shader_modified_time_ = modified;

    if (!render_widget_->reloadShader())
    {
        return;
    }

    shader_version_++;
    project_tree_panel_->setActiveShaderPath(info.absoluteFilePath());
    updateShaderInfo();
    updateWindowTitle();
    onShaderStatusChanged("Shader automatically reloaded: " + info.fileName());
}
