/*
    QNvFanControl - QNvFanControl - control nvidia fan
    Copyright (C) 2017  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Notify.hpp"

#include <libnotify/notify.h>

#include <QCoreApplication>

void Notify::init()
{
    notify_init(QCoreApplication::applicationName().toUtf8().constData());
}

void Notify::notify(const char *title, const char *message, const char *icon)
{
    NotifyNotification *notification = notify_notification_new(title, message, icon);
    notify_notification_show(notification, nullptr);
    g_object_unref(G_OBJECT(notification));
}

void Notify::uninit()
{
    notify_uninit();
}
