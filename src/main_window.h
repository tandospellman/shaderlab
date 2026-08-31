#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QDateTime>
#include <QDockWidget>
#include <QLabel>
#include <QLocale>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QString>
#include <QTimer>

class ParamForm;
class ProjectTreePanel;
class QToolBar;
class RenderWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void openShader();
    void reloadShader();
    void savePreset();
    void loadPreset();

    void openDirectory();
    void pasteFileFromClipboard();
    void saveScreenshot();
    void resetShaderTime();

    void openProjectFilePath(const QString& path);

    void onShaderStatusChanged(const QString& message);
    void onParamsChanged();

    void resetUniforms();
    void setUniformDockLocked(bool locked);

    void saveUniformFile();

    void loadUniformFile();

    void checkCurrentShaderForChanges();

private:
    void setupUi();
    void setupMenus();
    void setupToolbar();
    void setupDocks();

    void loadSettings();
    void saveSettings();

    void updateShaderInfo();
    void updateWindowTitle();

    void setToolbarLocked(bool locked);

private:
    RenderWidget* render_widget_;
    ParamForm* param_form_;

    QDockWidget* project_dock_;
    QDockWidget* uniform_dock_;
    QDockWidget* log_dock_;

    ProjectTreePanel* project_tree_panel_;
    QPlainTextEdit* log_output_;

    QLabel* shader_path_label_;
    QLabel* modified_time_label_;
    QLabel* version_label_;
    QLabel* auto_reload_status_label_;

    QToolBar* main_toolbar_;

    QAction* open_shader_action_;
    QAction* open_directory_action_;
    QAction* paste_file_action_;
    QAction* screenshot_action_;
    QAction* pause_time_action_;
    QAction* reset_time_action_;
    QAction* reload_shader_action_;
    QAction* save_preset_action_;
    QAction* load_preset_action_;
    QAction* exit_action_;
    QAction* lock_toolbar_action_;

    QAction* save_uniform_file_action_;

    QAction* load_uniform_file_action_;

    QAction* reset_uniforms_action_;
    QAction* lock_uniform_dock_action_;
    QAction* clear_log_action_;

    QString last_folder_;
    QString current_shader_;

    int shader_version_;

    QLocale locale_;

    QTimer shader_watch_timer_;

    QAction* auto_reload_action_;
    QDateTime current_shader_modified_time_;
};
