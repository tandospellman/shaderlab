#include "shader_file_utils.h"

#include <QFileInfo>

QStringList shaderFileFilters()
{
    return {
        "*.frag",
        "*.glsl",
        "*.vert",
        "*.png",
        "*.jpg",
        "*.jpeg"
    };
}

bool isShaderFile(const QFileInfo& info)
{
    QString suffix = info.suffix().toLower();

    return
        suffix == "frag" ||
        suffix == "glsl" ||
        suffix == "vert";
}
