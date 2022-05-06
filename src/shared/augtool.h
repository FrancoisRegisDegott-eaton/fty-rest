/*
 *
 * Copyright (C) 2015 - 2020 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/// @file   augtool.h
/// @author Michal Hrusecky <MichalHrusecky@Eaton.com>
/// @brief  Not yet documented file
#pragma once
#include <fty/process.h>
#include <functional>
#include <mutex>
#include <string>


/// Simple class abstraction over augtool
class augtool
{
public:
    /// Singleton get_instance method (w/ privileges escalation option)
    static augtool* get_instance(bool sudoer);

    /// Method returning parsed output of augtool
    ///
    /// If there is more than two lines, omits first and the last one. Returns all values concatenated using settings in
    /// optional parameters.
    ///
    /// @param cmd what to execute
    /// @param key_value if true each line is expected in form 'key = value' and only value is outputed
    /// @param sep used to separate individual values
    /// @param filter values for which it returns true are omitted
    std::string get_cmd_out(
        const std::string&               cmd,
        bool                             key_value = true,
        const std::string&               sep       = "",
        std::function<bool(std::string)> filter    = [](const std::string) -> bool { return false; }
    );

    /// Return string directly as returned from augtool
    std::string get_cmd_out_raw(const std::string& cmd);

    /// Runs command without returning anything
    void run_cmd(const std::string& cmd)
    {
        get_cmd_out_raw(cmd);
    }

    /// Saves current state
    void save()
    {
        run_cmd("save");
    }

protected:
    std::mutex    m_cmd_mutex; //!< Shared mutex, to protect cmd overlap
    fty::Process* m_process{nullptr}; //!< Subprocess itself

    /// Ensures we are in reasonably clean state
    augtool(bool sudoer) noexcept;
    bool init(bool sudoer) noexcept;
    bool initialized();

    void load();
};
