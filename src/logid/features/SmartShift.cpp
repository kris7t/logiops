/*
 * Copyright 2019-2020 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "SmartShift.h"
#include "../Device.h"
#include "../util/log.h"

using namespace logid::features;
using namespace logid::backend;

SmartShift::SmartShift(Device* device) : DeviceFeature(device), _config
    (device)
{
    try {
        _smartshift = std::make_shared<hidpp20::SmartShift>(&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _ipc_interface = std::make_shared<IPC>(this);
    ipc::registerAuto(_ipc_interface.get());
}

void SmartShift::configure()
{
    _smartshift->setStatus(_config.getSettings());
}

void SmartShift::listen()
{
}

hidpp20::SmartShift::SmartshiftStatus SmartShift::getStatus()
{
    return _smartshift->getStatus();
}

void SmartShift::setStatus(backend::hidpp20::SmartShift::SmartshiftStatus
    status)
{
    _smartshift->setStatus(status);
}

void SmartShift::saveConfig(libconfig::Setting& root)
{
    _config.save(root);
}

SmartShift::Config::Config(Device *dev) : DeviceFeature::Config(dev), _status()
{
    try {
        auto& config_root = dev->config().getSetting("smartshift");
        if(!config_root.isGroup()) {
            logPrintf(WARN, "Line %d: smartshift must be an object",
                    config_root.getSourceLine());
            return;
        }
        _status.setActive = config_root.lookupValue("on", _status.active);
        int tmp;
        _status.setAutoDisengage = config_root.lookupValue("threshold", tmp);
        if(_status.setAutoDisengage)
            _status.autoDisengage = tmp;
        _status.setDefaultAutoDisengage = config_root.lookupValue
                ("default_threshold", tmp);
        if(_status.setDefaultAutoDisengage)
            _status.defaultAutoDisengage = tmp;
    } catch(libconfig::SettingNotFoundException& e) {
        // SmartShift not configured, use default
    }
}

hidpp20::SmartShift::SmartshiftStatus SmartShift::Config::getSettings() const
{
    return _status;
}

void SmartShift::Config::mergeSettings(
        const hidpp20::SmartShift::SmartshiftStatus& o)
{
    _status.setActive |= o.setActive;
    if(o.setActive)
        _status.active = o.active;

    _status.setAutoDisengage |= o.setAutoDisengage;
    if(o.setAutoDisengage)
        _status.autoDisengage = o.autoDisengage;

    _status.setDefaultAutoDisengage |= o.setDefaultAutoDisengage;
    if(o.setDefaultAutoDisengage)
        _status.defaultAutoDisengage = o.defaultAutoDisengage;
}

void SmartShift::Config::save(libconfig::Setting& root)
{
    if(root.exists("smartshift"))
        root.remove("smartshift");

    auto& ss_setting = root.add("smartshift", libconfig::Setting::TypeGroup);

    if(_status.setActive)
        ss_setting.add("on", libconfig::Setting::TypeBoolean) = _status.active;
    if(_status.setAutoDisengage)
        ss_setting.add("threshold", libconfig::Setting::TypeInt) =
                _status.autoDisengage;
    if(_status.setDefaultAutoDisengage)
        ss_setting.add("default_threshold", libconfig::Setting::TypeInt) =
                _status.defaultAutoDisengage;
}

SmartShift::IPC::IPC(SmartShift *smart_shift) : ipc::IPCInterface(
        smart_shift->device()->ipc().node() + "/smartshift", "SmartShift"),
        _smartshift (smart_shift)
{
    auto get_status = std::make_shared<ipc::IPCFunction>();
    get_status->response.emplace_back("active",
                                      ipc::IPCVariant::TypeInfo::Boolean);
    get_status->response.emplace_back("threshold",
                                      ipc::IPCVariant::TypeInfo::Byte);
    get_status->response.emplace_back("defaultThreshold",
                                      ipc::IPCVariant::TypeInfo::Byte);
    get_status->function = [this](const ipc::IPCFunctionArgs& args)->
            ipc::IPCFunctionArgs {
        (void)(args);
        ipc::IPCFunctionArgs response;
        auto status = _smartshift->getStatus();
        response.emplace_back(status.active);
        response.emplace_back(status.autoDisengage);
        response.emplace_back(status.defaultAutoDisengage);
        return response;
    };

    auto set_status = std::make_shared<ipc::IPCFunction>();
    set_status->args.emplace_back("active", "(bb)");
    set_status->args.emplace_back("threshold", "(by)");
    set_status->args.emplace_back("defaultThreshold", "(by)");
    set_status->function = [this](const ipc::IPCFunctionArgs& args)->
            ipc::IPCFunctionArgs {
        hidpp20::SmartShift::SmartshiftStatus status{};
        status.setActive = args[0][0];
        status.active = args[0][1];
        status.setAutoDisengage = args[1][0];
        status.autoDisengage = args[1][1];
        status.setDefaultAutoDisengage = args[2][0];
        status.defaultAutoDisengage = args[2][1];

        _smartshift->_config.mergeSettings(status);
        _smartshift->setStatus(status);

        return {};
    };

    _functions.emplace("getStatus", get_status);
    _functions.emplace("setStatus", set_status);
}