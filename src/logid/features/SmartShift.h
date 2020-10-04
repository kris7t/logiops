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
#ifndef LOGID_FEATURE_SMARTSHIFT_H
#define LOGID_FEATURE_SMARTSHIFT_H

#include "../backend/hidpp20/features/SmartShift.h"
#include "DeviceFeature.h"
#include "../ipc/IPCInterface.h"

namespace logid {
namespace features
{
    class SmartShift : public DeviceFeature
    {
    public:
        explicit SmartShift(Device* dev);
        virtual void configure();
        virtual void listen();

        backend::hidpp20::SmartShift::SmartshiftStatus getStatus();
        void setStatus(backend::hidpp20::SmartShift::SmartshiftStatus status);
        void saveConfig(libconfig::Setting& root);

        class Config : public DeviceFeature::Config
        {
        public:
            explicit Config(Device* dev);
            backend::hidpp20::SmartShift::SmartshiftStatus getSettings() const;
            void mergeSettings(
                    const backend::hidpp20::SmartShift::SmartshiftStatus& o);

            void save(libconfig::Setting& root);
        protected:
            backend::hidpp20::SmartShift::SmartshiftStatus _status;
        };

        class IPC : public ipc::IPCInterface
        {
        public:
            IPC(SmartShift* smart_shift);
        private:
            SmartShift* _smartshift;
        };
    private:
        Config _config;
        std::shared_ptr<backend::hidpp20::SmartShift> _smartshift;
        std::shared_ptr<IPC> _ipc_interface;
    };
}}

#endif //LOGID_FEATURE_SMARTSHIFT_H
