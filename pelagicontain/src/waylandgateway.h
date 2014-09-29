/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */
#pragma once

#include <string>
#include <unistd.h>
#include "gateway.h"
#include "systemcallinterface.h"

class WaylandGateway: public Gateway {

	LOG_DECLARE_CLASS_CONTEXT("Wayl", "Wayland gateway");

public:

	static constexpr const char* ID = "wayland";

	WaylandGateway() {
	}

	~WaylandGateway() {
	}

	std::string id() override {
		return ID;
	}

	static constexpr const char* WAYLAND_RUNTIME_DIR_VARIABLE_NAME= "XDG_RUNTIME_DIR";
	static constexpr const char* SOCKET_FILE_NAME = "wayland-0";

	bool setConfig(const std::string &config) override {
		JSonParser parser(config);
		m_enabled = false;
		parser.readBoolean("enabled", m_enabled);
		return true;
	}

	bool activate() override {
		if (m_enabled) {
			const char* dir = getenv(WAYLAND_RUNTIME_DIR_VARIABLE_NAME);
			if (dir != nullptr) {
				log_info() << "enabling Wayland gateway. Socket dir:" << dir;
				std::string d = logging::StringBuilder() << dir << "/" << SOCKET_FILE_NAME;
				std::string path = getContainer().bindMountFileInContainer(d,  SOCKET_FILE_NAME, false);
				setEnvironmentVariable(WAYLAND_RUNTIME_DIR_VARIABLE_NAME, parentPath(path));
			} else {
				log_error() << "Should enable wayland gateway, but " << WAYLAND_RUNTIME_DIR_VARIABLE_NAME << " is not defined";
				return false;
			}
		}

		return true;
	}

	bool teardown() override {
		return true;
	}

private:

	bool m_enabled = false;
};

