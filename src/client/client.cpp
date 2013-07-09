
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickView>
#include <QScreen>
#include <QDebug>
#include <QTimer>

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-desktop-shell-client-protocol.h"

#include "client.h"
#include "processlauncher.h"

QTimer timer;

Client::Client()
      : QObject()
{
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
    Q_ASSERT(m_display);

    m_fd = wl_display_get_fd(m_display);
    Q_ASSERT(m_fd > -1);
    qDebug() << "Wayland display socket:" << m_fd;

    m_registry = wl_display_get_registry(m_display);
    Q_ASSERT(m_registry);

    wl_registry_add_listener(m_registry, &s_registryListener, this);
}

Client::~Client()
{
    delete m_grabWindow;
    delete m_backgroundView;
    qDeleteAll(m_panelViews);
}

void Client::create()
{
    m_launcher = new ProcessLauncher(this);
    m_grabWindow = new QWindow;
    m_grabWindow->resize(1, 1);
    m_grabWindow->create();
    m_grabWindow->show();

    wl_surface *grabSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_grabWindow));
    desktop_shell_set_grab_surface(m_shell, grabSurface);

    QString path(QCoreApplication::applicationDirPath() + QLatin1String("/../src/client/"));

    QScreen *screen = QGuiApplication::screens().first();
    wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen));

    m_backgroundView = new QQuickView;
    m_backgroundView->setSource(path + QLatin1String("background.qml"));

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setAlphaBufferSize(8);

//     m_backgroundView->setFormat(format);
    m_backgroundView->setFlags(Qt::BypassWindowManagerHint);
    m_backgroundView->setScreen(screen);
    m_backgroundView->show();
    m_backgroundView->create();

    wl_surface *backgroundSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", m_backgroundView));
    desktop_shell_set_background(m_shell, output, backgroundSurface);

    qDebug() << "Background for screen" << screen->name() << "with geometry" << m_backgroundView->geometry();



    QQuickView *panelView = new QQuickView;
    panelView = new QQuickView;
    panelView->engine()->rootContext()->setContextProperty("ProcessLauncher", m_launcher);
    panelView->setSource(path + QLatin1String("panel.qml"));
    //     panelView->setFormat(format);
    panelView->setFlags(Qt::BypassWindowManagerHint);
    panelView->setScreen(screen);
    panelView->show();
    panelView->create();

    wl_surface *panelSurface = static_cast<struct wl_surface *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", panelView));
    desktop_shell_set_panel(m_shell, output, panelSurface);

    m_panelViews << panelView;
}

void Client::handleGlobal(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Q_UNUSED(version);

    Client *object = static_cast<Client *>(data);

    if (strcmp(interface, "desktop_shell") == 0) {
        // Bind interface and register listener
        object->m_shell = static_cast<desktop_shell *>(wl_registry_bind(registry, id, &desktop_shell_interface, version));
        desktop_shell_add_listener(object->m_shell, &s_shellListener, data);

        QMetaObject::invokeMethod(object, "create");
    }
}

const wl_registry_listener Client::s_registryListener = {
    Client::handleGlobal
};

void Client::configure(void *data, desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height)
{
    qDebug()<<"configure";
    Client *object = static_cast<Client *>(data);

    object->m_backgroundView->resize(width, height);
}

void Client::handlePrepareLockSurface(void *data, desktop_shell *desktop_shell)
{
}

void Client::handleGrabCursor(void *data, desktop_shell *desktop_shell, uint32_t cursor)
{
    Client *object = static_cast<Client *>(data);

    QCursor qcursor;
    switch (cursor) {
        case DESKTOP_SHELL_CURSOR_NONE:
            break;
        case DESKTOP_SHELL_CURSOR_BUSY:
            qcursor.setShape(Qt::BusyCursor);
            break;
        case DESKTOP_SHELL_CURSOR_MOVE:
            qcursor.setShape(Qt::DragMoveCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_TOP:
            qcursor.setShape(Qt::SizeVerCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM:
            qcursor.setShape(Qt::SizeVerCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_LEFT:
            qcursor.setShape(Qt::SizeHorCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_RIGHT:
            qcursor.setShape(Qt::SizeHorCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_TOP_LEFT:
            qcursor.setShape(Qt::SizeFDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_TOP_RIGHT:
            qcursor.setShape(Qt::SizeBDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM_LEFT:
            qcursor.setShape(Qt::SizeBDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM_RIGHT:
            qcursor.setShape(Qt::SizeFDiagCursor);
            break;
        case DESKTOP_SHELL_CURSOR_ARROW:
        default:
            break;
    }

    object->m_grabWindow->setCursor(qcursor);
}

const desktop_shell_listener Client::s_shellListener = {
    Client::configure,
    Client::handlePrepareLockSurface,
    Client::handleGrabCursor
};

#include "client.moc"
