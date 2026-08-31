#pragma once

#include <QString>
#include <QOpenGLFunctions_3_3_Core>

struct ShaderUniform
{
    QString name;
    GLenum type;
    GLint location;
    GLint size;
};