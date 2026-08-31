#pragma once

#include <QStringList>

class QFileInfo;

QStringList shaderFileFilters();


bool isShaderFile(const QFileInfo& info);
