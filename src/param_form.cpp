#include "param_form.h"

#include <QLayoutItem>
#include <QSizePolicy>

namespace
{
QString uniformTypeName(GLenum gl_type)
{
    switch (gl_type)
    {
        case GL_INT: return "int";
        case GL_FLOAT: return "float";
        case GL_FLOAT_VEC2: return "vec2";
        case GL_FLOAT_VEC3: return "vec3";
        case GL_FLOAT_VEC4: return "vec4";
        case GL_SAMPLER_2D: return "sampler2D";
        default: return "unknown";
    }
}

QString uniformLabel(const ParamBase& param)
{
    QString type_name = uniformTypeName(param.glType());

    if (param.arraySize() > 1)
    {
        type_name += QString("[%1]").arg(param.arraySize());
    }

    return QString("%1  ·  %2").arg(param.name(), type_name);
}
}

ParamForm::ParamForm(QWidget* parent)
    : QWidget(parent)
    , layout_(new QFormLayout(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout_->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout_->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout_->setFormAlignment(Qt::AlignTop);
    layout_->setContentsMargins(12, 12, 12, 12);
    layout_->setHorizontalSpacing(10);
    layout_->setVerticalSpacing(10);
}

void ParamForm::rebuild(const std::vector<std::unique_ptr<ParamBase>>& params)
{
    clearLayout();

    for (const auto& param : params)
    {
        QWidget* editor = param->createEditor(this);
        editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout_->addRow(uniformLabel(*param), editor);

        connect(
            param.get(),
            &ParamBase::valueChanged,
            this,
            &ParamForm::paramsChanged,
            Qt::UniqueConnection
        );
    }
}

void ParamForm::clearLayout()
{
    while (layout_->rowCount() > 0)
    {
        layout_->removeRow(0);
    }
}
