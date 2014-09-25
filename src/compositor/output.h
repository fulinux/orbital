/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ORBITAL_OUTPUT_H
#define ORBITAL_OUTPUT_H

#include <QObject>
#include <QRect>

struct wl_resource;
struct weston_output;

namespace Orbital {

class Compositor;
class Workspace;
class View;
class Layer;
class WorkspaceView;
class Root;
class Animation;
class Pager;
class Surface;
struct Listener;

class Output : public QObject
{
    Q_OBJECT
public:
    explicit Output(weston_output *out);
    ~Output();

    Workspace *currentWorkspace() const;

    void setBackground(Surface *surface);
    void setPanel(Surface *surface, int pos);
    void setOverlay(Surface *surface);

    void repaint();

    int id() const;
    int x() const;
    int y() const;
    int width() const;
    int height() const;
    QRect geometry() const;
    QRect availableGeometry() const;
    wl_resource *resource(wl_client *client) const;
    View *rootView() const;
    QString name() const;

    static Output *fromOutput(weston_output *out);
    static Output *fromResource(wl_resource *res);

signals:
    void moved();

private:
    void onMoved();

    Compositor *m_compositor;
    weston_output *m_output;
    Listener *m_listener;
    Layer *m_panelsLayer;
    Root *m_transformRoot;
    View *m_background;
    QList<View *> m_panels;
    QList<View *> m_overlays;
    Workspace *m_currentWs;
    Surface *m_backgroundSurface;

    friend View;
    friend Animation;
    friend Pager;
};

}

#endif
