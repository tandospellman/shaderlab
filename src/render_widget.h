#pragma once

#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QJsonObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QString>
#include <QTimer>
#include <QVector2D>

#include <memory>
#include <vector>

#include "param.h"

class QMouseEvent;

class RenderWidget
    : public QOpenGLWidget
    , protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit RenderWidget(QWidget* parent = nullptr);
    ~RenderWidget() override;

    QString shaderPath() const;
    QDateTime shaderLastModified() const;

    const std::vector<std::unique_ptr<ParamBase>>& params() const;

    QJsonObject savePreset() const;
    void loadPreset(const QJsonObject& preset);

    QString uniformSavePath() const;

    bool saveUniformFile() const;
    bool loadUniformFile();

    bool isTimePaused() const;
    float shaderTimeSeconds() const;
    bool saveScreenshot(const QString& path);

public slots:
    bool setShaderPath(const QString& path);
    bool reloadShader();

    void resetUniforms();
    void setTimePaused(bool paused);
    void resetShaderTime();

signals:
    void shaderStatusChanged(const QString& message);
    void paramsChanged();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool createFullscreenQuad();

    bool createShaderProgram(
        GLuint& new_program,
        std::vector<std::unique_ptr<ParamBase>>& new_params,
        QString& error_log
    );

    QString loadFragmentShaderSource() const;

    void uploadBuiltinUniforms();
    void uploadParams();

    GLuint textureForParam(
        const QString& name,
        const QString& path
    );

    void clearTextures();

    bool isBuiltinUniform(const QString& name) const;
    QString normalizeUniformName(const QString& name) const;

    std::unique_ptr<ParamBase> createParamFromUniform(
        const QString& name,
        GLenum type,
        GLint array_size
    ) const;

    template<typename T>
    const Param<T>* findParam(
        const std::vector<std::unique_ptr<ParamBase>>& params,
        const QString& name,
        GLenum type,
        GLint array_size
    ) const
    {
        for (const auto& param : params)
        {
            if (
                param->name() == name &&
                param->glType() == type &&
                param->arraySize() == array_size
            )
            {
                return dynamic_cast<const Param<T>*>(param.get());
            }
        }

        return nullptr;
    }

private:
    struct TextureState
    {
        GLuint id = 0;
        QString path;
    };

    QTimer frame_timer_;
    QElapsedTimer elapsed_timer_;

    QString shader_path_;
    QDateTime shader_last_modified_;
    QVector2D mouse_position_;

    qint64 accumulated_time_ms_;
    bool time_paused_;

    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;
    GLuint shader_program_;

    bool gl_ready_;

    std::vector<std::unique_ptr<ParamBase>> params_;
    QHash<QString, TextureState> textures_;
};
