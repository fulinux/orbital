
#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>

class QQmlEngine;
class QQmlComponent;
class QQuickView;
class QWindow;

struct wl_display;
struct wl_registry;
struct wl_surface;
struct wl_registry_listener;

struct desktop_shell;
struct desktop_shell_listener;
struct desktop_shell_binding;

class ProcessLauncher;

class Binding : public QObject
{
    Q_OBJECT
public:
    ~Binding();

signals:
    void triggered();

private:
    desktop_shell_binding *bind;
    friend class Client;
};

class Client : public QObject
{
    Q_OBJECT
public:
    Client();
    ~Client();

    Q_INVOKABLE Binding *addKeyBinding(uint32_t key, uint32_t modifiers);

private slots:
    void create();

private:
    static void handleGlobal(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void configure(void *data, desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height);
    static void handlePrepareLockSurface(void *data, desktop_shell *desktop_shell);
    static void handleGrabCursor(void *data, desktop_shell *desktop_shell, uint32_t cursor);

    static const wl_registry_listener s_registryListener;
    static const desktop_shell_listener s_shellListener;

    wl_display *m_display;
    wl_registry *m_registry;
    int m_fd;
    desktop_shell *m_shell;
    ProcessLauncher *m_launcher;
    QWindow *m_grabWindow;
    QList<Binding *> m_bindings;

    QQuickView *m_backgroundView;
    QList<QQuickView *> m_panelViews;
    QQuickView *m_volumeView;
};

#endif