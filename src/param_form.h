#pragma once

#include <QFormLayout>
#include <QWidget>

#include <memory>
#include <vector>

#include "param.h"

class ParamForm final : public QWidget
{
    Q_OBJECT

public:
    explicit ParamForm(QWidget* parent = nullptr);

    void rebuild(
        const std::vector<std::unique_ptr<ParamBase>>& params
    );

signals:
    void paramsChanged();

private:
    void clearLayout();

private:
    QFormLayout* layout_;
};