/*
    QNvFanControl - control nvidia fan
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

#include "Nvidia.hpp"

#include <QTimer>
#include <QDebug>

#include <algorithm>

#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

Nvidia::Nvidia(const bool verbose) :
    m_verbose(verbose),
    m_timer(new QTimer),
    m_dpy(XOpenDisplay(nullptr))
{
    QObject::connect(m_timer.get(), &QTimer::timeout, std::bind(&Nvidia::timeout, this));
}
Nvidia::~Nvidia()
{
    if (m_dpy)
    {
        if (m_timerChangedToManual)
            setFanAutoControl();
        XCloseDisplay(m_dpy);
    }
}

bool Nvidia::isOk() const
{
    return (m_dpy != nullptr && getFanControl() != FanControl::Error);
}

qint32 Nvidia::getTemperature(const qint32 gpu) const
{
    int value = 0;
    const Bool ret = XNVCTRLQueryTargetAttribute(
                m_dpy,
                NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                gpu,
                0,
                NV_CTRL_THERMAL_SENSOR_READING,
                &value);
    return (ret == True) ? qMax(0, value) : -1;
}
qint32 Nvidia::getFanLevel(const qint32 gpu) const
{
    int value = 0;
    const Bool ret = XNVCTRLQueryTargetAttribute(
                m_dpy,
                NV_CTRL_TARGET_TYPE_COOLER,
                gpu,
                0,
                NV_CTRL_THERMAL_COOLER_LEVEL,
                &value);
    return (ret == True) ? qBound<qint32>(0, value, 100) : -1;
}

Nvidia::FanControl Nvidia::getFanControl(const qint32 gpu) const
{
    int value = 0;
    const Bool ret = XNVCTRLQueryTargetAttribute(
                m_dpy,
                NV_CTRL_TARGET_TYPE_GPU,
                gpu,
                0,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                &value);
    if (ret == True)
    {
        return (value == NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE)
                ? FanControl::Manual
                : FanControl::Auto;
    }
    return FanControl::Error;
}
bool Nvidia::setFanAutoControl(const qint32 gpu)
{
    const Bool ret = XNVCTRLSetTargetAttributeAndGetStatus(
                m_dpy,
                NV_CTRL_TARGET_TYPE_GPU,
                gpu,
                0,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE);
    if (ret == true)
    {
        m_timerChangedToManual = false;
        return true;
    }
    return false;
}
bool Nvidia::setFanManualControl(const qint32 gpu)
{
    const Bool ret = XNVCTRLSetTargetAttributeAndGetStatus(
                m_dpy,
                NV_CTRL_TARGET_TYPE_GPU,
                gpu,
                0,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE);
    if (ret == true)
    {
        m_timerChangedToManual = false;
        return true;
    }
    return false;
}

bool Nvidia::setFanLevel(const qint32 value, const qint32 gpu)
{
    const Bool ret = XNVCTRLSetTargetAttributeAndGetStatus(
                m_dpy,
                NV_CTRL_TARGET_TYPE_COOLER,
                gpu,
                0,
                NV_CTRL_THERMAL_COOLER_LEVEL,
                value);
    return (ret == True);
}

bool Nvidia::setCurvePoints(const std::initializer_list<TemperatureSpeed> &points)
{
    constexpr size_t len = 100;

    constexpr auto compare = [](const TemperatureSpeed &a, const TemperatureSpeed &b) {
        return a.temperature < b.temperature;
    };

    if (points.size() < 2 || points.size() > len)
        return false;
    if (!std::is_sorted(points.begin(), points.end(), compare))
        return false;
    if (std::adjacent_find(points.begin(), points.end(), compare) == points.end())
        return false;
    if (points.begin()->temperature < 0)
        return false;

    const auto lerp = [](const qint32 v0, const qint32 v1, const double t) {
        return qRound(v0 + t * (v1 - v0));
    };

    auto it = points.begin();
    auto itEnd = points.end();

    qint32 prevFanLevel = 0;
    qint32 currFanLevel = 0;

    qint32 nextTemperature = it->temperature;

    double lerpPos = 0.0;
    double lerpStep = 0.0;

    m_fanLevels.reset(new qint32[len]);
    m_temperatureFanTurnOn = -1;

    for (size_t temperature = 0; temperature < len; ++temperature)
    {
        if (it != itEnd && temperature == (size_t)nextTemperature)
        {
            if (m_temperatureFanTurnOn < 0)
                m_temperatureFanTurnOn = temperature;
            prevFanLevel = it->fanLevel;
            if (++it != itEnd)
            {
                currFanLevel = it->fanLevel;

                nextTemperature = it->temperature;

                lerpPos = 0.0;
                lerpStep = 1.0 / (nextTemperature - temperature);
            }
            else
            {
                lerpPos = 1.0;
                lerpStep = 0.0;
            }
        }
        else
        {
            lerpPos += lerpStep;
        }
        m_fanLevels[temperature] = lerp(prevFanLevel, currFanLevel, lerpPos);
        if (m_verbose)
            qDebug().nospace() << "Temperature: " << temperature << "℃, level: " << m_fanLevels[temperature] << "%";
    }

    return true;
}

bool Nvidia::setFanOffOffset(const qint32 fanOffOffset)
{
    if (fanOffOffset <= 0 || fanOffOffset > 10)
        return false;
    m_fanOffOffset = fanOffOffset;
    return true;
}

void Nvidia::setErrorCallback(const Nvidia::ErrorCallback &errorCallback)
{
    m_errorCallback = errorCallback;
}

bool Nvidia::start(const qint32 interval)
{
    if (m_fanLevels == nullptr || m_temperatureFanTurnOn < 0)
        return false;
    m_timer->start(interval);
    QTimer::singleShot(0, std::bind(&Nvidia::timeout, this));
    return true;
}
void Nvidia::stop()
{
    m_timer->stop();
}

void Nvidia::timeout()
{
    switch (getFanControl())
    {
        case FanControl::Error:
            return handleError();
        case FanControl::Auto:
            if (!setFanManualControl())
                return handleError();
            setFanLevel(0);
            m_timerChangedToManual = true;
            break;
        default:
            break;
    }

    const qint32 temperature = getTemperature();
    const qint32 level = getFanLevel();
    const qint32 desiredLevel = m_fanLevels[temperature];

    if (temperature < 0 || level < 0)
        return handleError();

    if (desiredLevel != level)
    {
        if (level == 0 || desiredLevel != 0 || (temperature - m_temperatureFanTurnOn) < -m_fanOffOffset)
        {
            if (!setFanLevel(desiredLevel))
                return handleError();
            if (m_verbose)
                qDebug().nospace() << "Change fan speed level: " << level << "% -> " << desiredLevel << "%";
        }
    }
}

void Nvidia::handleError()
{
    m_timer->stop();
    if (m_timerChangedToManual && setFanAutoControl())
        m_timerChangedToManual = false;
    if (m_errorCallback)
        m_errorCallback();
}
