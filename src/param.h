#pragma once

#include <QAbstractSpinBox>
#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSizePolicy>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QVBoxLayout>
#include <QWidget>
#include <QLocale>
#include <QOpenGLFunctions_3_3_Core>

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class ParamBase : public QObject
{
    Q_OBJECT

public:
    explicit ParamBase(QString name, GLenum gl_type, GLint array_size, QObject* parent = nullptr)
        : QObject(parent)
        , name_(std::move(name))
        , gl_type_(gl_type)
        , array_size_(array_size)
    {
    }

    ~ParamBase() override = default;

    const QString& name() const
    {
        return name_;
    }

    GLenum glType() const
    {
        return gl_type_;
    }

    GLint arraySize() const
    {
        return array_size_;
    }

    virtual QWidget* createEditor(QWidget* parent) = 0;

    virtual QJsonValue toJson() const = 0;

    virtual void fromJson(const QJsonValue& value) = 0;

signals:
    void valueChanged();

private:
    QString name_;
    GLenum gl_type_;
    GLint array_size_;
};

template<typename T>
struct EditorFactory;

template<typename T>
class Param final : public ParamBase
{
public:
    Param(QString name, T value, GLenum gl_type, GLint array_size, QObject* parent = nullptr)
        : ParamBase(std::move(name), gl_type, array_size, parent)
        , value_(std::move(value))
    {
    }

    const T& value() const
    {
        return value_;
    }

    void setValue(const T& value)
    {
        if (value_ == value)
        {
            return;
        }

        value_ = value;
        emit valueChanged();
    }

    void setValue(T&& value)
    {
        if (value_ == value)
        {
            return;
        }

        value_ = std::move(value);
        emit valueChanged();
    }

    QWidget* createEditor(QWidget* parent) override
    {
        return EditorFactory<T>::create(*this, parent);
    }

    QJsonValue toJson() const override
    {
        if constexpr (std::is_same_v<T, int>)
        {
            return value_;
        }
        else if constexpr (std::is_same_v<T, QString>)
        {
            return value_;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return value_;
        }
        else if constexpr (std::is_same_v<T, QVector2D>)
        {
            return QJsonArray{value_.x(), value_.y()};
        }
        else if constexpr (std::is_same_v<T, QVector3D>)
        {
            return QJsonArray{value_.x(), value_.y(), value_.z()};
        }
        else if constexpr (std::is_same_v<T, QVector4D>)
        {
            return QJsonArray{value_.x(), value_.y(), value_.z(), value_.w()};
        }
        else if constexpr (std::is_same_v<T, std::vector<float>>)
        {
            QJsonArray array;

            for (float item : value_)
            {
                array.append(item);
            }

            return array;
        }
        else
        {
            return {};
        }
    }

    void fromJson(const QJsonValue& value) override
    {
        if constexpr (std::is_same_v<T, int>)
        {
            setValue(value.toInt());
        }
        else if constexpr (std::is_same_v<T, QString>)
        {
            setValue(value.toString());
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            setValue(static_cast<float>(value.toDouble()));
        }
        else if constexpr (std::is_same_v<T, QVector2D>)
        {
            QJsonArray array = value.toArray();

            if (array.size() == 2)
            {
                setValue(QVector2D(
                    static_cast<float>(array[0].toDouble()),
                    static_cast<float>(array[1].toDouble())
                ));
            }
        }
        else if constexpr (std::is_same_v<T, QVector3D>)
        {
            QJsonArray array = value.toArray();

            if (array.size() == 3)
            {
                setValue(QVector3D(
                    static_cast<float>(array[0].toDouble()),
                    static_cast<float>(array[1].toDouble()),
                    static_cast<float>(array[2].toDouble())
                ));
            }
        }
        else if constexpr (std::is_same_v<T, QVector4D>)
        {
            QJsonArray array = value.toArray();

            if (array.size() == 4)
            {
                setValue(QVector4D(
                    static_cast<float>(array[0].toDouble()),
                    static_cast<float>(array[1].toDouble()),
                    static_cast<float>(array[2].toDouble()),
                    static_cast<float>(array[3].toDouble())
                ));
            }
        }
        else if constexpr (std::is_same_v<T, std::vector<float>>)
        {
            QJsonArray array = value.toArray();

            std::vector<float> values;

            values.reserve(array.size());

            for (const QJsonValue& item : array)
            {
                values.push_back(static_cast<float>(item.toDouble()));
            }

            setValue(std::move(values));
        }
    }

private:
    T value_;
};

inline QDoubleSpinBox* createFloatSpinbox(QWidget* parent, double value)
{
    auto* spinbox = new QDoubleSpinBox(parent);

    spinbox->setRange(-1000.0, 1000.0);
    spinbox->setDecimals(3);
    spinbox->setSingleStep(0.1);
    spinbox->setValue(value);
    spinbox->setMinimumWidth(0);
    spinbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    spinbox->setLocale(QLocale::c());

    return spinbox;
}

inline bool isColorParamName(const QString& name)
{
    QString lower_name = name.toLower();

    return
        lower_name.contains("rgb") ||
        lower_name.contains("color") ||
        lower_name.contains("colour") ||
        lower_name.contains("tint") ||
        lower_name.contains("albedo");
}

template<>
struct EditorFactory<int>
{
    static QWidget* create(Param<int>& param, QWidget* parent)
    {
        auto* spinbox = new QSpinBox(parent);

        spinbox->setRange(-1000000, 1000000);
        spinbox->setSingleStep(1);
        spinbox->setValue(param.value());
        spinbox->setMinimumWidth(0);
        spinbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QObject::connect(
            spinbox,
            &QSpinBox::valueChanged,
            &param,
            [&param](int value)
            {
                param.setValue(value);
            }
        );

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            spinbox,
            [&param, spinbox]()
            {
                QSignalBlocker blocker(spinbox);
                spinbox->setValue(param.value());
            }
        );

        return spinbox;
    }
};

template<>
struct EditorFactory<QString>
{
    static QWidget* create(Param<QString>& param, QWidget* parent)
    {
        auto* row = new QWidget(parent);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto* path_edit = new QLineEdit(param.value(), row);
        path_edit->setPlaceholderText("Choose texture...");
        path_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* browse_button = new QPushButton("...", row);
        browse_button->setToolTip("Choose texture file");
        browse_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        layout->addWidget(path_edit);
        layout->addWidget(browse_button);

        QObject::connect(
            path_edit,
            &QLineEdit::editingFinished,
            &param,
            [&param, path_edit]()
            {
                param.setValue(path_edit->text().trimmed());
            }
        );

        QObject::connect(
            browse_button,
            &QPushButton::clicked,
            &param,
            [&param, path_edit, row]()
            {
                QString start_path = param.value();

                if (!start_path.isEmpty())
                {
                    QFileInfo info(start_path);
                    start_path = info.exists() ? info.absolutePath() : start_path;
                }

                const QString path = QFileDialog::getOpenFileName(
                    row,
                    "Choose Texture",
                    start_path,
                    "Images (*.png *.jpg *.jpeg *.bmp *.tga *.webp);;All Files (*)"
                );

                if (path.isEmpty())
                {
                    return;
                }

                path_edit->setText(path);
                param.setValue(path);
            }
        );

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            row,
            [&param, path_edit]()
            {
                QSignalBlocker blocker(path_edit);
                path_edit->setText(param.value());
            }
        );

        return row;
    }
};

template<>
struct EditorFactory<float>
{
    static QWidget* create(Param<float>& param, QWidget* parent)
    {
        auto* spinbox = createFloatSpinbox(parent, param.value());

        QObject::connect(
            spinbox,
            &QDoubleSpinBox::valueChanged,
            &param,
            [&param](double value)
            {
                param.setValue(static_cast<float>(value));
            }
        );

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            spinbox,
            [&param, spinbox]()
            {
                QSignalBlocker blocker(spinbox);
                spinbox->setValue(param.value());
            }
        );

        return spinbox;
    }
};

template<>
struct EditorFactory<QVector2D>
{
    static QWidget* create(Param<QVector2D>& param, QWidget* parent)
    {
        auto* row = new QWidget(parent);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto* x_spinbox = createFloatSpinbox(row, param.value().x());
        auto* y_spinbox = createFloatSpinbox(row, param.value().y());

        layout->addWidget(x_spinbox);
        layout->addWidget(y_spinbox);

        auto update_model =
            [&param, x_spinbox, y_spinbox]()
            {
                param.setValue(QVector2D(
                    static_cast<float>(x_spinbox->value()),
                    static_cast<float>(y_spinbox->value())
                ));
            };

        QObject::connect(x_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);
        QObject::connect(y_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            row,
            [&param, x_spinbox, y_spinbox]()
            {
                QSignalBlocker block_x(x_spinbox);
                QSignalBlocker block_y(y_spinbox);

                x_spinbox->setValue(param.value().x());
                y_spinbox->setValue(param.value().y());
            }
        );

        return row;
    }
};

template<>
struct EditorFactory<QVector3D>
{
    static QWidget* create(Param<QVector3D>& param, QWidget* parent)
    {
        auto* row = new QWidget(parent);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto* x_spinbox = createFloatSpinbox(row, param.value().x());
        auto* y_spinbox = createFloatSpinbox(row, param.value().y());
        auto* z_spinbox = createFloatSpinbox(row, param.value().z());

        layout->addWidget(x_spinbox);
        layout->addWidget(y_spinbox);
        layout->addWidget(z_spinbox);

        auto update_model =
            [&param, x_spinbox, y_spinbox, z_spinbox]()
            {
                param.setValue(QVector3D(
                    static_cast<float>(x_spinbox->value()),
                    static_cast<float>(y_spinbox->value()),
                    static_cast<float>(z_spinbox->value())
                ));
            };

        QObject::connect(x_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);
        QObject::connect(y_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);
        QObject::connect(z_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);

        if (isColorParamName(param.name()))
        {
            auto* color_button = new QPushButton("...", row);

            color_button->setToolTip("Pick color");
            color_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            layout->addWidget(color_button);

            QObject::connect(
                color_button,
                &QPushButton::clicked,
                &param,
                [&param, x_spinbox, y_spinbox, z_spinbox, row]()
                {
                    QColor current_color;

                    current_color.setRgbF(
                        std::clamp(param.value().x(), 0.0f, 1.0f),
                        std::clamp(param.value().y(), 0.0f, 1.0f),
                        std::clamp(param.value().z(), 0.0f, 1.0f)
                    );

                    QColor selected_color =
                        QColorDialog::getColor(
                            current_color,
                            row,
                            "Choose Color"
                        );

                    if (!selected_color.isValid())
                    {
                        return;
                    }

                    QVector3D new_value(
                        static_cast<float>(selected_color.redF()),
                        static_cast<float>(selected_color.greenF()),
                        static_cast<float>(selected_color.blueF())
                    );

                    param.setValue(new_value);
                }
            );
        }

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            row,
            [&param, x_spinbox, y_spinbox, z_spinbox]()
            {
                QSignalBlocker block_x(x_spinbox);
                QSignalBlocker block_y(y_spinbox);
                QSignalBlocker block_z(z_spinbox);

                x_spinbox->setValue(param.value().x());
                y_spinbox->setValue(param.value().y());
                z_spinbox->setValue(param.value().z());
            }
        );

        return row;
    }
};

template<>
struct EditorFactory<QVector4D>
{
    static QWidget* create(Param<QVector4D>& param, QWidget* parent)
    {
        auto* row = new QWidget(parent);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto* x_spinbox = createFloatSpinbox(row, param.value().x());
        auto* y_spinbox = createFloatSpinbox(row, param.value().y());
        auto* z_spinbox = createFloatSpinbox(row, param.value().z());
        auto* w_spinbox = createFloatSpinbox(row, param.value().w());

        layout->addWidget(x_spinbox);
        layout->addWidget(y_spinbox);
        layout->addWidget(z_spinbox);
        layout->addWidget(w_spinbox);

        auto update_model =
            [&param, x_spinbox, y_spinbox, z_spinbox, w_spinbox]()
            {
                param.setValue(QVector4D(
                    static_cast<float>(x_spinbox->value()),
                    static_cast<float>(y_spinbox->value()),
                    static_cast<float>(z_spinbox->value()),
                    static_cast<float>(w_spinbox->value())
                ));
            };

        QObject::connect(x_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);
        QObject::connect(y_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);
        QObject::connect(z_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);
        QObject::connect(w_spinbox, &QDoubleSpinBox::valueChanged, &param, update_model);

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            row,
            [&param, x_spinbox, y_spinbox, z_spinbox, w_spinbox]()
            {
                QSignalBlocker block_x(x_spinbox);
                QSignalBlocker block_y(y_spinbox);
                QSignalBlocker block_z(z_spinbox);
                QSignalBlocker block_w(w_spinbox);

                x_spinbox->setValue(param.value().x());
                y_spinbox->setValue(param.value().y());
                z_spinbox->setValue(param.value().z());
                w_spinbox->setValue(param.value().w());
            }
        );

        return row;
    }
};

template<>
struct EditorFactory<std::vector<float>>
{
    static QWidget* create(Param<std::vector<float>>& param, QWidget* parent)
    {
        auto* container = new QWidget(parent);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        std::vector<QDoubleSpinBox*> spinboxes;

        const std::vector<float>& values = param.value();

        spinboxes.reserve(values.size());

        for (int i = 0; i < static_cast<int>(values.size()); ++i)
        {
            auto* row = new QWidget(container);
            row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            auto* row_layout = new QHBoxLayout(row);
            row_layout->setContentsMargins(0, 0, 0, 0);
            row_layout->setSpacing(4);

            auto* label = new QLabel(QString::number(i), row);

            label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            auto* spinbox =
                createFloatSpinbox(
                    row,
                    values[static_cast<size_t>(i)]
                );

            row_layout->addWidget(label);
            row_layout->addWidget(spinbox);

            layout->addWidget(row);

            spinboxes.push_back(spinbox);
        }

        for (auto* spinbox : spinboxes)
        {
            QObject::connect(
                spinbox,
                &QDoubleSpinBox::valueChanged,
                &param,
                [&param, spinboxes](double)
                {
                    std::vector<float> new_values;

                    new_values.reserve(spinboxes.size());

                    for (auto* item : spinboxes)
                    {
                        new_values.push_back(
                            static_cast<float>(item->value())
                        );
                    }

                    param.setValue(std::move(new_values));
                }
            );
        }

        QObject::connect(
            &param,
            &ParamBase::valueChanged,
            container,
            [&param, spinboxes]()
            {
                const std::vector<float>& values = param.value();

                for (int i = 0; i < static_cast<int>(spinboxes.size()); ++i)
                {
                    QSignalBlocker blocker(spinboxes[static_cast<size_t>(i)]);

                    spinboxes[static_cast<size_t>(i)]->setValue(
                        values[static_cast<size_t>(i)]
                    );
                }
            }
        );

        return container;
    }
};