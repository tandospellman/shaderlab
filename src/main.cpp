#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>

#include "main_window.h"
#include "theme.h"

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    QApplication::setApplicationName("ShaderLab");
    QApplication::setOrganizationName("Tando");

    applyShaderLabTheme(app);

    MainWindow main_window;
    main_window.show();

    return app.exec();
}
