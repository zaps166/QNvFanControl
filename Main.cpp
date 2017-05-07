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

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

#include "Notify.hpp"
#include "Nvidia.hpp"

#include <csignal>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QNvFanControl");
    QCoreApplication::setApplicationVersion("0.0.1");

    QCommandLineParser parser;
    const QList<QCommandLineOption> options {
        {{"v",  "verbose"}, "Set verbose mode"},
    };
    parser.addOptions(options);
    parser.addHelpOption();
    parser.process(app);

    bool started = false;

    Notify::init();

    Nvidia nvidia(parser.isSet(options[0]));
    if (nvidia.isOk())
    {
        nvidia.setCurvePoints({
            {55, 20},
            {70, 70},
            {95, 90}
        });
        nvidia.setErrorCallback([] {
            QCoreApplication::exit(-2);
        });
        started = nvidia.start();
    }

    int ret = -1;
    if (started)
    {
        constexpr auto signalCallBack = [](int) {
            QCoreApplication::quit();
        };
        signal(SIGINT,  signalCallBack);
        signal(SIGTERM, signalCallBack);
        Notify::notify("Fan control",
                       "Service started!\n",
                       "dialog-information");
        ret = app.exec();
        if (ret == 0)
        {
            Notify::notify("Fan control",
                        "Service stopped!\n",
                        "dialog-information");
        }
        else
        {
            Notify::notify("Fan control error",
                           "Service stopped!",
                           "dialog-error");
        }
    }
    else
    {
        Notify::notify("Fan control",
                       "Can't start service,\n"
                       "be sure that you have set \"Coolbits 4\" in X11 configuration!",
                       "dialog-error");
    }

    Notify::uninit();

    return ret;
}
