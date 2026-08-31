#include "uniform_inspector.h"

#include <QHBoxLayout>

UniformInspector::UniformInspector(QWidget* parent)
    : QWidget(parent)
{
    form_layout_ = new QFormLayout(this);
}

QDoubleSpinBox* UniformInspector::createSpinbox(double value)
{
    QDoubleSpinBox* spinbox = new QDoubleSpinBox(this);

    spinbox->setRange(-1000.0, 1000.0);
    spinbox->setSingleStep(0.1);
    spinbox->setDecimals(3);
    spinbox->setValue(value);

    return spinbox;
}

void UniformInspector::rebuild(
    const std::vector<ShaderUniform>& uniforms,
    const QMap<QString, float>& float_values,
    const QMap<QString, QVector2D>& vec2_values,
    const QMap<QString, QVector3D>& vec3_values
)
{
    while (form_layout_->rowCount() > 0)
    {
        form_layout_->removeRow(0);
    }

    for (const ShaderUniform& uniform : uniforms)
    {
        if (uniform.name == "u_time" || uniform.name == "u_resolution")
            continue;

        if (uniform.type == GL_FLOAT)
        {
            QDoubleSpinBox* spinbox =
                createSpinbox(float_values.value(uniform.name, 1.0f));

            connect(
                spinbox,
                &QDoubleSpinBox::valueChanged,
                this,
                [this, uniform](double value)
                {
                    emit floatUniformChanged(
                        uniform.name,
                        static_cast<float>(value)
                    );
                }
            );

            form_layout_->addRow(uniform.name, spinbox);
        }

        if (uniform.type == GL_FLOAT_VEC2)
        {
            QVector2D value =
                vec2_values.value(uniform.name, QVector2D(0.5f, 0.5f));

            QWidget* row = new QWidget(this);

            QHBoxLayout* layout = new QHBoxLayout(row);

            layout->setContentsMargins(0, 0, 0, 0);

            QDoubleSpinBox* x_spinbox = createSpinbox(value.x());

            QDoubleSpinBox* y_spinbox = createSpinbox(value.y());

            layout->addWidget(x_spinbox);

            layout->addWidget(y_spinbox);

            auto emit_value =
                [this, uniform, x_spinbox, y_spinbox]()
                {
                    emit vec2UniformChanged(
                        uniform.name,
                        QVector2D(
                            static_cast<float>(x_spinbox->value()),
                            static_cast<float>(y_spinbox->value())
                        )
                    );
                };

            connect(
                x_spinbox,
                &QDoubleSpinBox::valueChanged,
                this,
                emit_value
            );

            connect(
                y_spinbox,
                &QDoubleSpinBox::valueChanged,
                this,
                emit_value
            );

            form_layout_->addRow(uniform.name, row);
        }

        if (uniform.type == GL_FLOAT_VEC3)
        {
            QVector3D value =
                vec3_values.value(uniform.name, QVector3D(1.0f, 1.0f, 1.0f));

            QWidget* row = new QWidget(this);

            QHBoxLayout* layout = new QHBoxLayout(row);

            layout->setContentsMargins(0, 0, 0, 0);

            QDoubleSpinBox* x_spinbox = createSpinbox(value.x());

            QDoubleSpinBox* y_spinbox = createSpinbox(value.y());

            QDoubleSpinBox* z_spinbox = createSpinbox(value.z());

            layout->addWidget(x_spinbox);

            layout->addWidget(y_spinbox);

            layout->addWidget(z_spinbox);

            auto emit_value =
                [this, uniform, x_spinbox, y_spinbox, z_spinbox]()
                {
                    emit vec3UniformChanged(
                        uniform.name,
                        QVector3D(
                            static_cast<float>(x_spinbox->value()),
                            static_cast<float>(y_spinbox->value()),
                            static_cast<float>(z_spinbox->value())
                        )
                    );
                };

            connect(
                x_spinbox,
                &QDoubleSpinBox::valueChanged,
                this,
                emit_value
            );

            connect(
                y_spinbox,
                &QDoubleSpinBox::valueChanged,
                this,
                emit_value
            );

            connect(
                z_spinbox,
                &QDoubleSpinBox::valueChanged,
                this,
                emit_value
            );

            form_layout_->addRow(uniform.name, row);
        }
    }
}