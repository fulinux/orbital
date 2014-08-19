/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ORBITAL_INTERFACE_H
#define ORBITAL_INTERFACE_H

#include <type_traits>

#include <QObject>

struct wl_interface;
struct wl_client;
struct wl_global;

namespace Orbital {

class Interface;
class Compositor;

class Object : public QObject
{
    Q_OBJECT
public:
    explicit Object(QObject *p = nullptr);
    virtual ~Object();

    void addInterface(Interface *iface);

    template <class T>
    T *findInterface() const;

private:
    QList<Interface *> m_ifaces;
};

class Interface : public QObject
{
    Q_OBJECT
public:
    explicit Interface(QObject *p = nullptr);
    virtual ~Interface() {}

    Object *object() const { return m_obj; }

protected:
    virtual void added() {}

private:
    Object *m_obj;

    friend class Object;
};

class Global
{
public:
    Global(Compositor *c, const wl_interface *i, uint32_t version);
    virtual ~Global();

protected:
    virtual void bind(wl_client *client, uint32_t version, uint32_t id) = 0;

private:
    wl_global *m_global;
};


template <class T>
T *Object::findInterface() const
{
    static_assert(std::is_base_of<Interface, T>::value, "T is not derived from Interface.");
    for (Interface *iface: m_ifaces) {
        if (T *t = qobject_cast<T *>(iface)) {
            return t;
        }
    }
    return nullptr;
}

}

#endif
