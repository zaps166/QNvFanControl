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

#pragma once

#include <QtGlobal>

#include <initializer_list>
#include <functional>
#include <memory>

struct _XDisplay;
class QTimer;

class Nvidia
{
    Q_DISABLE_COPY(Nvidia)

public:
    enum class FanControl
    {
        Error = -1,
        Auto,
        Manual,
    };

    struct TemperatureSpeed
    {
        qint32 temperature;
        qint32 fanLevel;
    };

    using ErrorCallback = std::function<void()>;

    Nvidia(const bool verbose);
    ~Nvidia();

    bool isOk() const;

    qint32 getTemperature(const qint32 gpu = 0) const;
    qint32 getFanLevel(const qint32 gpu = 0) const;

    FanControl getFanControl(const qint32 gpu = 0) const;
    bool setFanAutoControl(const qint32 gpu = 0);
    bool setFanManualControl(const qint32 gpu = 0);

    bool setFanLevel(const qint32 value, const qint32 gpu = 0);

    // Temperature[0-99], Speed[0-100], unique temperatures, sorted
    bool setCurvePoints(const std::initializer_list<TemperatureSpeed> &points);

    bool setFanOffOffset(const qint32 fanOffOffset);

    void setErrorCallback(const ErrorCallback &errorCallback);

    bool start(const qint32 interval = 2500); //GPU 0 only
    void stop();

private:
    void timeout();

    void handleError();

    const bool m_verbose;
    ErrorCallback m_errorCallback;
    std::unique_ptr<qint32[]> m_fanLevels;
    qint32 m_temperatureFanTurnOn = 0;
    qint32 m_fanOffOffset = 10;
    std::unique_ptr<QTimer> m_timer;
    bool m_timerChangedToManual = false;
    _XDisplay *m_dpy;
};
