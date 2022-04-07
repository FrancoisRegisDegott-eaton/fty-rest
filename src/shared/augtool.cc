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

/*!
 * \file augtool.cc
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \brief Not yet documented file
 */
#include <fty/string-utils.h>
#include <fty_log.h>
#include <vector>
#include <set>
#include <string>
#include <functional>
#include <mutex>
#include <thread>
#include <chrono>

#include "shared/augtool.h"

// execute cmd, returns raw output
std::string augtool::get_cmd_out_raw(const std::string& cmd)
{
    std::lock_guard<std::mutex> lock(m_cmd_mutex);

    std::string ret; // empty, default

    if (m_process) {
        auto command{cmd};
        if (command.empty() || (command.back() != '\n')) {
            command += "\n";
        }

        auto writeSuccess = m_process->write(command);
        ret = m_process->readAllStandardOutput(500);
        if (!writeSuccess) {
            ret.clear();
        }
    }
    return ret;
}

// execute cmd, returns post-processed output
std::string augtool::get_cmd_out(
    const std::string& cmd,
    bool key_value, // process only '<key> = <value>' output
    const std::string& sep, // list separator
    std::function<bool(std::string)> filter // returns true to exclude value
)
{
    logDebug("augtool: '{}', key_value: {}, sep: '{}'", cmd, key_value, sep);

    // execute the command
    std::string cmdOutput = get_cmd_out_raw(cmd);

    // split output into lines
    std::vector<std::string> lines = fty::split(cmdOutput, "\n");

    std::string ret; // empty, default

    // expect at least 3 lines
    if (lines.size() >= 3) {
        // remove first and last prompts lines ("match...", "augtool>")
        lines.erase(lines.begin());
        lines.pop_back();

        // built ret
        for (auto line : lines) {
            auto pos = line.find_first_of("=");
            if (pos != std::string::npos) {
                // extract value (right of '=' leaving 1st space)
                line = line.substr(pos + 2);
            }
            else {
                if (key_value) { // key_value only?
                    continue; // ignore not '<key> = <value>' line
                }
            }

            // apply exclusion filter
            if (filter(line)) {
                continue; // ignore line
            }

            if (line.empty()) { // inconsistent?
                logDebug("adding an empty line!");
            }

            ret += (ret.empty() ? "" : sep) + line;
        }
    }

    logDebug("augtool: '{}', ret: '{}'", cmd, ret);
    return ret;
}

// returns process instance
augtool* augtool::get_instance(bool sudoer)
{
    static augtool instance_sudoer(true); // privileges escalation
    static augtool instance_nopriv(false); // no escalation
    
    logInfo("Augtool get_instance(), sudoer: {}", sudoer);

    //get the correct instance ()
    auto instance = sudoer ? &instance_sudoer : &instance_nopriv;
    
    //Run the init
    logInfo("Check that Augeas is initialise");
    
    if(!instance->init(sudoer)) {
        logError("Augeas could not be initilized");
        return nullptr;
    }
    
    return instance;
}

augtool::augtool(bool sudoer) noexcept
{
    init(sudoer);
}

bool augtool::init(bool sudoer) noexcept
{
    try {
        if (m_process) { // once
            return true;
        }

        m_process = (sudoer)
            ? new fty::Process("sudo",    {"augtool", "-S", "-I/usr/share/fty/lenses", "-e"})
            : new fty::Process("augtool", {           "-S", "-I/usr/share/fty/lenses", "-e"});

        if (!m_process) {
            throw std::runtime_error("new failed");
        }

        if (!m_process->exists()) {
            if (auto ret = m_process->run()) {
                std::string output = get_cmd_out_raw("help");
                if (output.find("match") == output.npos) {
                    throw std::runtime_error("unexpected help payload");
                }
            }
            else {
                throw std::runtime_error("run failed (" + ret.error() + ")");
            }
        }

        load();

        logDebug("augtool init succeeded (sudoer: {})", sudoer);
        return true;
    }
    catch (const std::exception& e) {
        logFatal("augtool init (sudoer: {}) caught exception (e: {})", sudoer, e.what());
    }

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }
    return false;
}

bool augtool::initialized()
{
    return (m_process != nullptr);
}

void augtool::load()
{
    static std::mutex load_mutex;
    std::lock_guard<std::mutex> lock(load_mutex);

    run_cmd("");
    run_cmd("load");
}
