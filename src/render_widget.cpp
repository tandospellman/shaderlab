#include "render_widget.h"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTextStream>
#include <QSizePolicy>
#include <QJsonDocument>


namespace
{
const char* default_vertex_shader = R"(
#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main()
{
    v_uv = a_uv;

    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";
}

RenderWidget::RenderWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , shader_path_("shaders/example.frag")
    , mouse_position_(0.0f, 0.0f)
    , accumulated_time_ms_(0)
    , time_paused_(false)
    , vao_(0)
    , vbo_(0)
    , ebo_(0)
    , shader_program_(0)
    , gl_ready_(false)
{
    setSizePolicy(
    QSizePolicy::Expanding,
    QSizePolicy::Expanding
);

    setMouseTracking(true);

    elapsed_timer_.start();

    connect(
        &frame_timer_,
        &QTimer::timeout,
        this,
        QOverload<>::of(&RenderWidget::update)
    );

    frame_timer_.start(16);
}
RenderWidget::~RenderWidget()
{
    if (!isValid())
    {
        return;
    }

    makeCurrent();

    glDeleteVertexArrays(1, &vao_);

    glDeleteBuffers(1, &vbo_);

    glDeleteBuffers(1, &ebo_);

    clearTextures();

    glDeleteProgram(shader_program_);

    doneCurrent();
}

QDateTime RenderWidget::shaderLastModified() const
{
    return shader_last_modified_;
}

QString RenderWidget::shaderPath() const
{
    return shader_path_;
}

const std::vector<std::unique_ptr<ParamBase>>& RenderWidget::params() const
{
    return params_;
}

bool RenderWidget::setShaderPath(const QString& path)
{
    shader_path_ = path;

    QFileInfo info(shader_path_);

    if (info.exists())
    {
        shader_last_modified_ = info.lastModified();
    }

    if (!gl_ready_)
    {
        return true;
    }

    return reloadShader();
}
bool RenderWidget::reloadShader()
{
    QFileInfo info(shader_path_);

    // Nothing to reload if the file doesn't exist.
    if (!info.exists())
    {
        return false;
    }

    // Don't try to compile before OpenGL is initialized.
    if (!gl_ready_)
    {
        return false;
    }

    GLuint new_program = 0;

    std::vector<std::unique_ptr<ParamBase>> new_params;

    QString error_log;

    if (!createShaderProgram(
            new_program,
            new_params,
            error_log))
    {
        emit shaderStatusChanged(error_log);

        return false;
    }

    if (shader_program_ != 0)
    {
        glDeleteProgram(shader_program_);
    }

    shader_program_ = new_program;

    params_ = std::move(new_params);

    loadUniformFile();

    shader_last_modified_ = info.lastModified();

    emit paramsChanged();

    emit shaderStatusChanged(
        QString("Shader reloaded: %1")
            .arg(info.fileName())
    );

    update();

    return true;
}

QJsonObject RenderWidget::savePreset() const
{
    QJsonObject root;

    QJsonObject values;

    for (const auto& param : params_)
    {
        values[param->name()] = param->toJson();
    }

    root["shader"] = shader_path_;

    root["values"] = values;

    return root;
}

void RenderWidget::loadPreset(const QJsonObject& preset)
{
    QJsonObject values = preset["values"].toObject();

    for (const auto& param : params_)
    {
        if (values.contains(param->name()))
        {
            param->fromJson(values[param->name()]);
        }
    }

    emit paramsChanged();

    update();
}

bool RenderWidget::isTimePaused() const
{
    return time_paused_;
}

float RenderWidget::shaderTimeSeconds() const
{
    qint64 total_ms = accumulated_time_ms_;

    if (!time_paused_ && elapsed_timer_.isValid())
    {
        total_ms += elapsed_timer_.elapsed();
    }

    return static_cast<float>(total_ms) / 1000.0f;
}

void RenderWidget::setTimePaused(bool paused)
{
    if (time_paused_ == paused)
    {
        return;
    }

    if (paused)
    {
        if (elapsed_timer_.isValid())
        {
            accumulated_time_ms_ += elapsed_timer_.elapsed();
        }

        time_paused_ = true;
    }
    else
    {
        elapsed_timer_.restart();
        time_paused_ = false;
    }

    update();
}

void RenderWidget::resetShaderTime()
{
    accumulated_time_ms_ = 0;
    elapsed_timer_.restart();
    update();
}

bool RenderWidget::saveScreenshot(const QString& path)
{
    if (path.isEmpty())
    {
        return false;
    }

    const QImage image = grabFramebuffer();

    if (image.isNull())
    {
        return false;
    }

    return image.save(path);
}

void RenderWidget::initializeGL()
{
    initializeOpenGLFunctions();

    gl_ready_ = true;

    glViewport(0, 0, width(), height());

    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

    createFullscreenQuad();

    reloadShader();
}

void RenderWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void RenderWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF position = event->position();

    mouse_position_.setX(static_cast<float>(position.x()));
    mouse_position_.setY(static_cast<float>(height() - position.y()));

    update();
    QOpenGLWidget::mouseMoveEvent(event);
}

void RenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (shader_program_ == 0)
    {
        return;
    }

    glUseProgram(shader_program_);

    uploadBuiltinUniforms();

    uploadParams();

    glBindVertexArray(vao_);

    glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        nullptr
    );
}

void RenderWidget::uploadBuiltinUniforms()
{
    GLint time_location =
        glGetUniformLocation(shader_program_, "u_time");

    if (time_location >= 0)
    {
        const float time = shaderTimeSeconds();

        glUniform1f(time_location, time);
    }

    GLint resolution_location =
        glGetUniformLocation(shader_program_, "u_resolution");

    if (resolution_location >= 0)
    {
        glUniform2f(
            resolution_location,
            static_cast<float>(width()),
            static_cast<float>(height())
        );
    }

    GLint mouse_location =
        glGetUniformLocation(shader_program_, "u_mouse");

    if (mouse_location >= 0)
    {
        glUniform2f(
            mouse_location,
            mouse_position_.x(),
            mouse_position_.y()
        );
    }
}

void RenderWidget::uploadParams()
{
    int texture_unit = 0;

    for (const auto& param : params_)
    {
        GLint location =
            glGetUniformLocation(
                shader_program_,
                param->name().toUtf8().constData()
            );

        if (location < 0)
        {
            continue;
        }

        if (auto* value = dynamic_cast<Param<QString>*>(param.get()))
        {
            if (param->glType() != GL_SAMPLER_2D)
            {
                continue;
            }

            const GLuint texture_id =
                textureForParam(
                    param->name(),
                    value->value()
                );

            glActiveTexture(GL_TEXTURE0 + texture_unit);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glUniform1i(location, texture_unit);

            ++texture_unit;
        }
        else if (auto* value = dynamic_cast<Param<int>*>(param.get()))
        {
            glUniform1i(location, value->value());
        }
        else if (auto* value = dynamic_cast<Param<float>*>(param.get()))
        {
            glUniform1f(location, value->value());
        }
        else if (auto* value = dynamic_cast<Param<QVector2D>*>(param.get()))
        {
            glUniform2f(
                location,
                value->value().x(),
                value->value().y()
            );
        }
        else if (auto* value = dynamic_cast<Param<QVector3D>*>(param.get()))
        {
            glUniform3f(
                location,
                value->value().x(),
                value->value().y(),
                value->value().z()
            );
        }
        else if (auto* value = dynamic_cast<Param<QVector4D>*>(param.get()))
        {
            glUniform4f(
                location,
                value->value().x(),
                value->value().y(),
                value->value().z(),
                value->value().w()
            );
        }
        else if (auto* value = dynamic_cast<Param<std::vector<float>>*>(param.get()))
        {
            const std::vector<float>& items = value->value();

            if (items.empty())
            {
                continue;
            }

            if (param->glType() == GL_FLOAT)
            {
                glUniform1fv(
                    location,
                    param->arraySize(),
                    items.data()
                );
            }
            else if (param->glType() == GL_FLOAT_VEC2)
            {
                glUniform2fv(
                    location,
                    param->arraySize(),
                    items.data()
                );
            }
            else if (param->glType() == GL_FLOAT_VEC3)
            {
                glUniform3fv(
                    location,
                    param->arraySize(),
                    items.data()
                );
            }
            else if (param->glType() == GL_FLOAT_VEC4)
            {
                glUniform4fv(
                    location,
                    param->arraySize(),
                    items.data()
                );
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
}

GLuint RenderWidget::textureForParam(
    const QString& name,
    const QString& path)
{
    TextureState& state = textures_[name];

    if (path.isEmpty())
    {
        if (state.id != 0)
        {
            glDeleteTextures(1, &state.id);
            state.id = 0;
        }

        state.path.clear();
        return 0;
    }

    if (state.id != 0 && state.path == path)
    {
        return state.id;
    }

    QImage image(path);

    if (image.isNull())
    {
        if (state.id != 0)
        {
            glDeleteTextures(1, &state.id);
            state.id = 0;
        }

        state.path = path;
        return 0;
    }

    image = image
        .convertToFormat(QImage::Format_RGBA8888)
        .flipped(Qt::Vertical);

    if (state.id != 0)
    {
        glDeleteTextures(1, &state.id);
        state.id = 0;
    }

    glGenTextures(1, &state.id);
    glBindTexture(GL_TEXTURE_2D, state.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        image.width(),
        image.height(),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.constBits()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    state.path = path;

    return state.id;
}

void RenderWidget::clearTextures()
{
    for (auto it = textures_.begin(); it != textures_.end(); ++it)
    {
        if (it->id != 0)
        {
            glDeleteTextures(1, &it->id);
            it->id = 0;
        }
    }

    textures_.clear();
}

bool RenderWidget::createFullscreenQuad()
{
    const float vertices[] =
    {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    const unsigned int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &vao_);

    glGenBuffers(1, &vbo_);

    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))
    );

    glEnableVertexAttribArray(1);

    return true;
}

QString RenderWidget::loadFragmentShaderSource() const
{
    QFile file(shader_path_);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }

    QTextStream stream(&file);

    return stream.readAll();
}

bool RenderWidget::isBuiltinUniform(const QString& name) const
{
    return
        name == "u_time" ||
        name == "u_resolution" ||
        name == "u_delta_time" ||
        name == "u_mouse" ||
        name == "u_frame_index";
}

QString RenderWidget::normalizeUniformName(const QString& name) const
{
    QString result = name;

    if (result.endsWith("[0]"))
    {
        result.chop(3);
    }

    return result;
}

std::unique_ptr<ParamBase> RenderWidget::createParamFromUniform(
    const QString& name,
    GLenum type,
    GLint array_size
) const
{
    if (type == GL_SAMPLER_2D)
    {
        QString value;

        if (const auto* old_param =
            findParam<QString>(
                params_,
                name,
                type,
                array_size
            ))
        {
            value = old_param->value();
        }

        return std::make_unique<Param<QString>>(
            name,
            value,
            type,
            array_size
        );
    }

    if (array_size > 1)
    {
        int component_count = 1;

        if (type == GL_FLOAT_VEC2)
        {
            component_count = 2;
        }
        else if (type == GL_FLOAT_VEC3)
        {
            component_count = 3;
        }
        else if (type == GL_FLOAT_VEC4)
        {
            component_count = 4;
        }

        std::vector<float> values(
            static_cast<size_t>(array_size * component_count),
            1.0f
        );

        if (const auto* old_param =
            findParam<std::vector<float>>(
                params_,
                name,
                type,
                array_size
            ))
        {
            values = old_param->value();
        }

        return std::make_unique<Param<std::vector<float>>>(
            name,
            std::move(values),
            type,
            array_size
        );
    }

    if (type == GL_INT)
    {
        int value = 1;

        if (const auto* old_param =
            findParam<int>(
                params_,
                name,
                type,
                array_size
            ))
        {
            value = old_param->value();
        }

        return std::make_unique<Param<int>>(
            name,
            value,
            type,
            array_size
        );
    }

    if (type == GL_FLOAT)
    {
        float value = 1.0f;

        if (const auto* old_param =
            findParam<float>(
                params_,
                name,
                type,
                array_size
            ))
        {
            value = old_param->value();
        }

        return std::make_unique<Param<float>>(
            name,
            value,
            type,
            array_size
        );
    }

    if (type == GL_FLOAT_VEC2)
    {
        QVector2D value(0.5f, 0.5f);

        if (const auto* old_param =
            findParam<QVector2D>(
                params_,
                name,
                type,
                array_size
            ))
        {
            value = old_param->value();
        }

        return std::make_unique<Param<QVector2D>>(
            name,
            value,
            type,
            array_size
        );
    }

    if (type == GL_FLOAT_VEC3)
    {
        QVector3D value(1.0f, 1.0f, 1.0f);

        if (const auto* old_param =
            findParam<QVector3D>(
                params_,
                name,
                type,
                array_size
            ))
        {
            value = old_param->value();
        }

        return std::make_unique<Param<QVector3D>>(
            name,
            value,
            type,
            array_size
        );
    }

    if (type == GL_FLOAT_VEC4)
    {
        QVector4D value(1.0f, 1.0f, 1.0f, 1.0f);

        if (const auto* old_param =
            findParam<QVector4D>(
                params_,
                name,
                type,
                array_size
            ))
        {
            value = old_param->value();
        }

        return std::make_unique<Param<QVector4D>>(
            name,
            value,
            type,
            array_size
        );
    }

    return nullptr;
}

bool RenderWidget::createShaderProgram(
    GLuint& new_program,
    std::vector<std::unique_ptr<ParamBase>>& new_params,
    QString& error_log
)
{
    QString fragment_source =
        loadFragmentShaderSource();

    if (fragment_source.isEmpty())
    {
        error_log =
            "Failed to load shader file: " + shader_path_;

        return false;
    }

    GLint success = 0;

    GLuint vertex_shader =
        glCreateShader(GL_VERTEX_SHADER);

    const char* vertex_source =
        default_vertex_shader;

    glShaderSource(
        vertex_shader,
        1,
        &vertex_source,
        nullptr
    );

    glCompileShader(vertex_shader);

    glGetShaderiv(
        vertex_shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char log[2048];

        glGetShaderInfoLog(
            vertex_shader,
            2048,
            nullptr,
            log
        );

        error_log =
            QString("Vertex shader error:\n%1").arg(log);

        glDeleteShader(vertex_shader);

        return false;
    }

    GLuint fragment_shader =
        glCreateShader(GL_FRAGMENT_SHADER);

    QByteArray fragment_bytes =
        fragment_source.toUtf8();

    const char* fragment_source_cstr =
        fragment_bytes.constData();

    glShaderSource(
        fragment_shader,
        1,
        &fragment_source_cstr,
        nullptr
    );

    glCompileShader(fragment_shader);

    glGetShaderiv(
        fragment_shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char log[2048];

        glGetShaderInfoLog(
            fragment_shader,
            2048,
            nullptr,
            log
        );

        error_log =
            QString("Fragment shader error:\n%1").arg(log);

        glDeleteShader(vertex_shader);

        glDeleteShader(fragment_shader);

        return false;
    }

    new_program =
        glCreateProgram();

    glAttachShader(new_program, vertex_shader);

    glAttachShader(new_program, fragment_shader);

    glLinkProgram(new_program);

    glGetProgramiv(
        new_program,
        GL_LINK_STATUS,
        &success
    );

    glDeleteShader(vertex_shader);

    glDeleteShader(fragment_shader);

    if (!success)
    {
        char log[2048];

        glGetProgramInfoLog(
            new_program,
            2048,
            nullptr,
            log
        );

        error_log =
            QString("Shader link error:\n%1").arg(log);

        glDeleteProgram(new_program);

        new_program = 0;

        return false;
    }

    GLint uniform_count = 0;

    glGetProgramiv(
        new_program,
        GL_ACTIVE_UNIFORMS,
        &uniform_count
    );

    for (GLint i = 0; i < uniform_count; ++i)
    {
        GLchar raw_name[256];

        GLenum type;

        GLint size;

        glGetActiveUniform(
            new_program,
            i,
            sizeof(raw_name),
            nullptr,
            &size,
            &type,
            raw_name
        );

        QString name =
            normalizeUniformName(raw_name);

        if (isBuiltinUniform(name))
        {
            continue;
        }

        std::unique_ptr<ParamBase> param =
            createParamFromUniform(
                name,
                type,
                size
            );

        if (param)
        {
            new_params.push_back(std::move(param));
        }
    }

    return true;
}

void RenderWidget::resetUniforms()
{
    if (!gl_ready_)
    {
        return;
    }

    params_.clear();

    reloadShader();

    emit paramsChanged();

    update();
}
QString RenderWidget::uniformSavePath() const
{
    if (shader_path_.isEmpty())
    {
        return {};
    }

    return shader_path_ + ".vusf";
}

bool RenderWidget::saveUniformFile() const
{
    QString path = uniformSavePath();

    if (path.isEmpty())
    {
        return false;
    }

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QJsonObject root;

    root["version"] = 1;
    root["shader"] = QFileInfo(shader_path_).fileName();

    QJsonObject values;

    for (const auto& param : params_)
    {
        values[param->name()] = param->toJson();
    }

    root["values"] = values;

    QJsonDocument document(root);

    file.write(document.toJson(QJsonDocument::Indented));

    return true;
}

bool RenderWidget::loadUniformFile()
{
    QString path = uniformSavePath();

    if (path.isEmpty())
    {
        return false;
    }

    QFileInfo info(path);

    if (!info.exists())
    {
        return false;
    }

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QJsonDocument document =
        QJsonDocument::fromJson(file.readAll());

    if (!document.isObject())
    {
        return false;
    }

    QJsonObject root =
        document.object();

    QJsonObject values =
        root["values"].toObject();

    for (const auto& param : params_)
    {
        if (values.contains(param->name()))
        {
            param->fromJson(values[param->name()]);
        }
    }

    emit paramsChanged();

    update();

    return true;
}