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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <linux/input.h>

#include <wayland-server.h>

#include <weston/compositor.h>
// #include <weston/config-parser.h>

#include "shell.h"
#include "shellsurface.h"
#include "effect.h"
#include "desktop-shell.h"

Shell::Shell(struct weston_compositor *ec)
            : m_compositor(ec)
            , m_blackSurface(nullptr)
            , m_grabSurface(nullptr)
{
    srandom(weston_compositor_get_time());
}

Shell::~Shell()
{
    printf("dtor\n");
}

static void
black_surface_configure(struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height)
{
}

void Shell::init()
{
    m_compositor->shell_interface.shell = this;
    m_compositor->shell_interface.create_shell_surface = [](void *shell, struct weston_surface *surface, const struct weston_shell_client *client) {
        return (struct shell_surface *)static_cast<Shell *>(shell)->createShellSurface(surface, client);
    };
    //     ec->shell_interface.set_toplevel = set_toplevel;
    //     ec->shell_interface.set_transient = set_transient;
    //     ec->shell_interface.set_fullscreen = set_fullscreen;
    //     ec->shell_interface.move = surface_move;
    //     ec->shell_interface.resize = surface_resize;

    struct wl_global *global = wl_display_add_global(m_compositor->wl_display, &wl_shell_interface, this,
                                                     [](struct wl_client *client, void *data, uint32_t version, uint32_t id) {
                                                         static_cast<Shell *>(data)->bind(client, version, id);
                                                     });

    if (!global)
        return;

    weston_layer_init(&m_layer, &m_compositor->cursor_layer.link);
    weston_layer_init(&m_backgroundLayer, &m_layer.link);


    m_blackSurface = weston_surface_create(m_compositor);

    struct weston_output *out = container_of(m_compositor->output_list.next, struct weston_output, link);

    int x = 0, y = 0;
    int w = out->width, h = out->height;

    m_blackSurface->configure = black_surface_configure;
    m_blackSurface->configure_private = 0;
    weston_surface_configure(m_blackSurface, x, y, w, h);
    weston_surface_set_color(m_blackSurface, 0.0, 0.0, 0.0, 1);
    pixman_region32_fini(&m_blackSurface->opaque);
    pixman_region32_init_rect(&m_blackSurface->opaque, 0, 0, w, h);
    pixman_region32_fini(&m_blackSurface->input);
    pixman_region32_init_rect(&m_blackSurface->input, 0, 0, w, h);

    wl_list_insert(&m_backgroundLayer.surface_list, &m_blackSurface->layer_link);

    struct wl_event_loop *loop = wl_display_get_event_loop(m_compositor->wl_display);
    wl_event_loop_add_idle(loop, [](void *data) { static_cast<Shell *>(data)->launchShellProcess(); }, this);

    weston_compositor_add_button_binding(compositor(), BTN_LEFT, (weston_keyboard_modifier)0,
                                         [](struct wl_seat *seat, uint32_t time, uint32_t button, void *data) {
                                             static_cast<Shell *>(data)->activateSurface(seat, time, button); }, this);
}

void Shell::configureSurface(ShellSurface *surface, int32_t sx, int32_t sy, int32_t width, int32_t height)
{
    if (width == 0) {
        return;
    }

    bool changedType = false;
    if (surface->m_type != surface->m_pendingType && surface->m_pendingType != ShellSurface::Type::None) {
        changedType = true;
        surface->m_type = surface->m_pendingType;
        surface->m_pendingType = ShellSurface::Type::None;
    }

    if (!surface->isMapped()) {
        switch (surface->m_type) {
            case ShellSurface::Type::TopLevel:
                surface->map(10 + random() % 400, 10 + random() % 400, width, height);
                break;
            default:
                surface->map(surface->m_surface->geometry.x + sx, surface->m_surface->geometry.y + sy, width, height);
        }

        if (surface->m_type == ShellSurface::Type::TopLevel) {
            struct weston_seat *seat;
            wl_list_for_each(seat, &m_compositor->seat_list, link) {
                weston_surface_activate(surface->m_surface, seat);
            }

            m_surfaces.push_back(surface);
            for (Effect *e: m_effects) {
                e->addSurface(surface);
            }
        }
    } else if (changedType || sx != 0 || sy != 0 || surface->width() != width || surface->height() != height) {
        float from_x, from_y;
        float to_x, to_y;

        struct weston_surface *es = surface->m_surface;
        weston_surface_to_global_float(es, 0, 0, &from_x, &from_y);
        weston_surface_to_global_float(es, sx, sy, &to_x, &to_y);
        weston_surface_configure(es, surface->x() + to_x - from_x, surface->y() + to_y - from_y, width, height);
//         configure(shell, es,
//                   es->geometry.x + to_x - from_x,
//                   es->geometry.y + to_y - from_y,
//                   width, height);
    }


}

static void shell_surface_configure(struct weston_surface *surf, int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    ShellSurface *shsurf = Shell::getShellSurface(surf);
    if (shsurf) {
        shsurf->shell()->configureSurface(shsurf, sx, sy, w, h);
    }
}

ShellSurface *Shell::createShellSurface(struct weston_surface *surface, const struct weston_shell_client *client)
{
    ShellSurface *shsurf = new ShellSurface(this, surface);

    surface->configure = shell_surface_configure;
    surface->configure_private = shsurf;
//     shsurf->client = client;
    return shsurf;
}

static const struct weston_shell_client shell_client = {
//     send_configure
};

ShellSurface *Shell::getShellSurface(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                                     struct wl_resource *surface_resource)
{
    struct weston_surface *surface = static_cast<weston_surface *>(surface_resource->data);

    ShellSurface *shsurf = getShellSurface(surface);
    if (shsurf) {
        wl_resource_post_error(surface_resource,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "desktop_shell::get_shell_surface already requested");
        return shsurf;
    }

    shsurf = createShellSurface(surface, &shell_client);
    if (!shsurf) {
        wl_resource_post_error(surface_resource,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "surface->configure already set");
        return nullptr;
    }

    shsurf->init(id);

    wl_client_add_resource(client, shsurf->wl_resource());
    wl_list_insert(&m_layer.surface_list, &surface->layer_link);

    return shsurf;
}

ShellSurface *Shell::getShellSurface(struct weston_surface *surf)
{
    if (surf->configure == shell_surface_configure) {
        return static_cast<ShellSurface *>(surf->configure_private);
    }

    return nullptr;
}

void Shell::removeShellSurface(ShellSurface *surface)
{
    for (Effect *e: m_effects) {
        e->removeSurface(surface);
    }
}

void Shell::bindEffect(Effect *effect, uint32_t key, enum weston_keyboard_modifier modifier)
{
    m_effects.push_back(effect);
    for (ShellSurface *s: m_surfaces) {
        effect->addSurface(s);
    }
    weston_compositor_add_key_binding(compositor(), key, modifier,
                                      [](struct wl_seat *seat, uint32_t time, uint32_t key, void *data) {
                                          static_cast<Effect *>(data)->run(seat, time, key); }, effect);
}

void Shell::activateSurface(ShellSurface *shsurf, struct weston_seat *seat)
{
    weston_surface_activate(shsurf->m_surface, seat);
    weston_surface_restack(shsurf->m_surface, &m_layer.surface_list);
}

void Shell::activateSurface(struct wl_seat *seat, uint32_t time, uint32_t button)
{
    struct weston_surface *focus = (struct weston_surface *) seat->pointer->focus;
    if (!focus)
        return;

//     struct weston_surface *upper;
//     if (is_black_surface(focus, &upper))
//         focus = upper;

//     if (get_shell_surface_type(focus) == SHELL_SURFACE_NONE)
//         return;

    if (seat->pointer->grab == &seat->pointer->default_grab) {
        ShellSurface *shsurf = getShellSurface(focus);
        if (shsurf) {
            activateSurface(shsurf, (struct weston_seat *)seat);
        }
//         activate(shell, focus, (struct weston_seat *)seat)
    };
}

struct MoveGrab : public ShellGrab {
    struct ShellSurface *shsurf;
    struct wl_listener shsurf_destroy_listener;
    wl_fixed_t dx, dy;
};

static void noop_grab_focus(struct wl_pointer_grab *grab, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    grab->focus = NULL;
}

void Shell::move_grab_motion(struct wl_pointer_grab *grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    ShellGrab *shgrab = container_of(grab, ShellGrab, grab);
    MoveGrab *move = static_cast<MoveGrab *>(shgrab);
    struct wl_pointer *pointer = grab->pointer;
    ShellSurface *shsurf = move->shsurf;
    int dx = wl_fixed_to_int(pointer->x + move->dx);
    int dy = wl_fixed_to_int(pointer->y + move->dy);

    if (!shsurf)
        return;

    struct weston_surface *es = shsurf->m_surface;

    weston_surface_configure(es, dx, dy, es->geometry.width, es->geometry.height);

    weston_compositor_schedule_repaint(es->compositor);
}

void Shell::move_grab_button(struct wl_pointer_grab *grab, uint32_t time, uint32_t button, uint32_t state_w)
{
    ShellGrab *shell_grab = container_of(grab, ShellGrab, grab);
    struct wl_pointer *pointer = grab->pointer;
    enum wl_pointer_button_state state = (wl_pointer_button_state)state_w;

    if (pointer->button_count == 0 && state == WL_POINTER_BUTTON_STATE_RELEASED) {
        Shell::endGrab(shell_grab);
        delete shell_grab;
    }
}

const struct wl_pointer_grab_interface Shell::m_move_grab_interface = {
    noop_grab_focus,
    Shell::move_grab_motion,
    Shell::move_grab_button,
};

void Shell::startGrab(ShellGrab *grab, const struct wl_pointer_grab_interface *interface,
                      struct wl_pointer *pointer, enum desktop_shell_cursor cursor)
{
//     popup_grab_end(pointer);

    grab->shell = this;
    grab->grab.interface = interface;
//     grab->shsurf = shsurf;
//     grab->shsurf_destroy_listener.notify = destroy_shell_grab_shsurf;
//     wl_signal_add(&shsurf->resource.destroy_signal, &grab->shsurf_destroy_listener);

    grab->pointer = pointer;
//     grab->grab.focus = &shsurf->m_surface->surface;

    wl_pointer_start_grab(pointer, &grab->grab);
    desktop_shell_send_grab_cursor(m_child.desktop_shell, cursor);
    wl_pointer_set_focus(pointer, &m_grabSurface->surface, wl_fixed_from_int(0), wl_fixed_from_int(0));
}

void Shell::endGrab(ShellGrab *grab)
{
//     if (grab->shsurf)
//         wl_list_remove(&grab->shsurf_destroy_listener.link);

    wl_pointer_end_grab(grab->pointer);
}

void Shell::moveSurface(ShellSurface *shsurf, struct weston_seat *ws)
{
    MoveGrab *move = new MoveGrab;
    if (!move)
        return;

    move->dx = wl_fixed_from_double(shsurf->m_surface->geometry.x) - ws->seat.pointer->grab_x;
    move->dy = wl_fixed_from_double(shsurf->m_surface->geometry.y) - ws->seat.pointer->grab_y;
    move->shsurf = shsurf;
    move->grab.focus = &shsurf->m_surface->surface;

    startGrab(move, &m_move_grab_interface, ws->seat.pointer, DESKTOP_SHELL_CURSOR_MOVE);
}

static void
configure_static_surface(struct weston_surface *es, struct weston_layer *layer, int32_t width, int32_t height)
{
    struct weston_surface *s, *next;

    if (width == 0)
        return;

    wl_list_for_each_safe(s, next, &layer->surface_list, layer_link) {
        if (s->output == es->output && s != es) {
            weston_surface_unmap(s);
            s->configure = NULL;
        }
    }

    weston_surface_configure(es, es->output->x, es->output->y, width, height);

    if (wl_list_empty(&es->layer_link)) {
        wl_list_insert(&layer->surface_list, &es->layer_link);
        weston_compositor_schedule_repaint(es->compositor);
    }
}

void Shell::backgroundConfigure(struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height)
{
    configure_static_surface(es, &m_backgroundLayer, width, height);
}

void Shell::setBackgroundSurface(struct weston_surface *surface, struct weston_output *output)
{
    surface->configure = [](struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height) {
        static_cast<Shell *>(es->configure_private)->backgroundConfigure(es, sx, sy, width, height); };
    surface->configure_private = this;
    surface->output = output;
}

void Shell::setGrabSurface(struct weston_surface *surface)
{
    m_grabSurface = surface;
}

const struct wl_shell_interface Shell::shell_implementation = {
    [](struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource) {
        static_cast<Shell *>(resource->data)->getShellSurface(client, resource, id, surface_resource);
    }
};

void Shell::bind(struct wl_client *client, uint32_t version, uint32_t id)
{
    wl_client_add_object(client, &wl_shell_interface, &shell_implementation, id, this);
}

static void
desktop_shell_sigchld(struct weston_process *process, int status)
{
//     uint32_t time;
//     struct desktop_shell *shell =
//     container_of(process, struct desktop_shell, child.process);
//
//     shell->child.process.pid = 0;
//     shell->child.client = NULL; /* already destroyed by wayland */
//
//     /* if desktop-shell dies more than 5 times in 30 seconds, give up */
//     time = weston_compositor_get_time();
//     if (time - shell->child.deathstamp > 30000) {
//         shell->child.deathstamp = time;
//         shell->child.deathcount = 0;
//     }
//
//     shell->child.deathcount++;
//     if (shell->child.deathcount > 5) {
//         weston_log("weston-desktop-shell died, giving up.\n");
//         return;
//     }
//
//     weston_log("weston-desktop-shell died, respawning...\n");
//     launch_desktop_shell_process(shell);
}

void Shell::launchShellProcess()
{
#define LIBEXECDIR "/usr/local/libexec"
    const char *shell_exe = LIBEXECDIR "/weston-desktop-shell";

    m_child.client = weston_client_launch(m_compositor,
                                          &m_child.process,
                                          shell_exe,
                                          desktop_shell_sigchld);

    printf("launch\n");

    if (!m_child.client)
        weston_log("not able to start %s\n", shell_exe);
}


WL_EXPORT int
module_init(struct weston_compositor *ec,
        int *argc, char *argv[], const char *config_file)
{
    Shell *shell = Shell::load<DesktopShell>(ec);
    if (!shell) {
        return -1;
    }
//     struct weston_seat *seat;
//     struct desktop_shell *shell;
//     struct wl_notification_daemon *notification;
//     struct workspace **pws;
//     unsigned int i;
//     struct wl_event_loop *loop;
//
//     shell = malloc(sizeof *shell);
//     if (shell == NULL)
//         return -1;
//     memset(shell, 0, sizeof *shell);
//     shell->compositor = ec;
//
//     notification = malloc(sizeof *notification);
//     if (notification == NULL)
//         return -1;
//     memset(notification, 0, sizeof *notification);
//     notification->compositor = ec;
//
//     shell->destroy_listener.notify = shell_destroy;
//     wl_signal_add(&ec->destroy_signal, &shell->destroy_listener);
//     shell->idle_listener.notify = idle_handler;
//     wl_signal_add(&ec->idle_signal, &shell->idle_listener);
//     shell->wake_listener.notify = wake_handler;
//     wl_signal_add(&ec->wake_signal, &shell->wake_listener);
//     shell->show_input_panel_listener.notify = show_input_panels;
//     wl_signal_add(&ec->show_input_panel_signal, &shell->show_input_panel_listener);
//     shell->hide_input_panel_listener.notify = hide_input_panels;
//     wl_signal_add(&ec->hide_input_panel_signal, &shell->hide_input_panel_listener);
//     ec->ping_handler = ping_handler;
//     ec->shell_interface.shell = shell;
//     ec->shell_interface.create_shell_surface = create_shell_surface;
//     ec->shell_interface.set_toplevel = set_toplevel;
//     ec->shell_interface.set_transient = set_transient;
//     ec->shell_interface.set_fullscreen = set_fullscreen;
//     ec->shell_interface.move = surface_move;
//     ec->shell_interface.resize = surface_resize;
//
//     wl_list_init(&shell->input_panel.surfaces);
//
//     weston_layer_init(&notification->layer, &ec->cursor_layer.link);
//     weston_layer_init(&shell->fullscreen_layer, &notification->layer.link);
//     weston_layer_init(&shell->panel_layer, &shell->fullscreen_layer.link);
//     weston_layer_init(&shell->background_layer, &shell->panel_layer.link);
//     weston_layer_init(&shell->lock_layer, NULL);
//     weston_layer_init(&shell->input_panel_layer, NULL);
//
//     wl_array_init(&shell->workspaces.array);
//     wl_list_init(&shell->workspaces.client_list);
//
//     shell_configuration(shell, notification, config_file);
//
//     for (i = 0; i < shell->workspaces.num; i++) {
//         pws = wl_array_add(&shell->workspaces.array, sizeof *pws);
//         if (pws == NULL)
//             return -1;
//
//         *pws = workspace_create();
//         if (*pws == NULL)
//             return -1;
//     }
//     activate_workspace(shell, 0);
//
//     wl_list_init(&shell->workspaces.anim_sticky_list);
//     wl_list_init(&shell->workspaces.animation.link);
//     shell->workspaces.animation.frame = animate_workspace_change_frame;
//
//     struct wl_global *global = wl_display_add_global(ec->wl_display, &wl_shell_interface, shell,
//                                [](struct wl_client *client, void *data, uint32_t version, uint32_t id) {
//                                    static_cast<Shell *>(data)->bind(client, version, id);
//                                });
//
//     if (!global)
//         return -1;
//
//     if (wl_display_add_global(ec->wl_display,
//                   &desktop_shell_interface,
//                   shell, bind_desktop_shell) == NULL)
//         return -1;
//
//     if (wl_display_add_global(ec->wl_display,
//                   &wl_notification_daemon_interface,
//                   notification, bind_notification_daemon) == NULL)
//         return -1;
//
//     if (wl_display_add_global(ec->wl_display, &screensaver_interface,
//                   shell, bind_screensaver) == NULL)
//         return -1;
//
//     if (wl_display_add_global(ec->wl_display, &input_panel_interface,
//                   shell, bind_input_panel) == NULL)
//         return -1;
//
//     if (wl_display_add_global(ec->wl_display, &workspace_manager_interface,
//                   shell, bind_workspace_manager) == NULL)
//         return -1;
//
//     shell->child.deathstamp = weston_compositor_get_time();
//
//     loop = wl_display_get_event_loop(ec->wl_display);
//     wl_event_loop_add_idle(loop, launch_desktop_shell_process, shell);
//
//     shell->screensaver.timer =
//         wl_event_loop_add_timer(loop, 0, shell);
//
//     wl_list_for_each(seat, &ec->seat_list, link)
//         create_pointer_focus_listener(seat);
//
//     shell_add_bindings(ec, shell);
//
//     shell_fade(shell, FADE_IN);

    return 0;
}
