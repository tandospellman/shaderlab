#pragma once

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QMap>
#include <QVector2D>
#include <QVector3D>
#include <QWidget>

#include <vector>

#include "shader_uniform.h"

class UniformInspector : public QWidget
{
    Q_OBJECT

public:
    explicit UniformInspector(QWidget* parent = nullptr);

    void rebuild(
        const std::vector<ShaderUniform>& uniforms,
        const QMap<QString, float>& float_values,
        const QMap<QString, QVector2D>& vec2_values,
        const QMap<QString, QVector3D>& vec3_values
    );

signals:
    void floatUniformChanged(const QString& name, float value);

    void vec2UniformChanged(const QString& name, QVector2D value);

    void vec3UniformChanged(const QString& name, QVector3D value);

private:
    QDoubleSpinBox* createSpinbox(double value);

private:
    QFormLayout* form_layout_;
};