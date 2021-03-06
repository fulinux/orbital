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

#ifndef ORBITAL_LAYER_H
#define ORBITAL_LAYER_H

#include <vector>
#include <memory>

struct weston_layer;

namespace Orbital {

class View;
struct Wrapper;

class Layer
{
public:
    explicit Layer(weston_layer *layer);
    explicit Layer(Layer *parent = nullptr);
    Layer(const Layer &) = delete;
    Layer(Layer &&l);
    ~Layer();

    void setParent(Layer *parent);

    void addView(View *view);
    void raiseOnTop(View *view);
    void lower(View *view);

    View *topView() const;

    void setMask(int x, int y, int w, int h);
    void unsetMask();
    void setAcceptInput(bool accept);
    bool acceptInput() const { return m_acceptInput; }

    static Layer *fromLayer(weston_layer *layer);

    Layer &operator=(const Layer &) = delete;
    Layer &operator=(Layer &&) = delete;

private:
    void addChild(Layer *l);
    void removeChild(Layer *l);

    std::unique_ptr<Wrapper> m_layer;
    Layer *m_parent;
    std::vector<Layer *> m_children;
    bool m_acceptInput;
};

}

#endif

